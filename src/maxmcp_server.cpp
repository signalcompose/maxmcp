/**
    @file maxmcp_server.cpp
    MaxMCP Server - MCP Server Max External Implementation

    @ingroup maxmcp
*/

#include "maxmcp_server.h"
#include "mcp_server.h"
#include "websocket_server.h"
#include "utils/console_logger.h"
#include <nlohmann/json.hpp>
#include <iostream>
#include <sstream>
#include <cstdlib>
#include <signal.h>
#include <unistd.h>
#include <sys/wait.h>

using json = nlohmann::json;

// Global class pointer
static t_class* maxmcp_server_class = nullptr;

// Global singleton instance
static t_maxmcp_server* g_server_instance = nullptr;

/**
 * @brief Main entry point for maxmcp.server external
 *
 * Registers the maxmcp.server class with Max.
 *
 * @param r Module reference (unused)
 */
void ext_main(void* r) {
    t_class* c;

    c = class_new("maxmcp.server",
                  (method)maxmcp_server_new,
                  (method)maxmcp_server_free,
                  (long)sizeof(t_maxmcp_server),
                  nullptr,
                  A_GIMME,
                  0);

    class_addmethod(c, (method)maxmcp_server_assist, "assist", A_CANT, 0);
    
    // Bridge management methods
    class_addmethod(c, (method)maxmcp_server_start_bridge, "start_bridge", A_GIMME, 0);
    class_addmethod(c, (method)maxmcp_server_stop_bridge, "stop_bridge", 0);

    // Register attributes
    CLASS_ATTR_LONG(c, "port", 0, t_maxmcp_server, port);
    CLASS_ATTR_LABEL(c, "port", 0, "WebSocket Port");
    CLASS_ATTR_DEFAULT(c, "port", 0, "7400");
    CLASS_ATTR_ACCESSORS(c, "port", nullptr, (method)maxmcp_server_port_set);
    
    CLASS_ATTR_LONG(c, "bridge_port", 0, t_maxmcp_server, bridge_port);
    CLASS_ATTR_LABEL(c, "bridge_port", 0, "Bridge WebSocket Port");
    CLASS_ATTR_DEFAULT(c, "bridge_port", 0, "18081");
    CLASS_ATTR_ACCESSORS(c, "bridge_port", nullptr, (method)maxmcp_server_bridge_port_set);

    CLASS_ATTR_CHAR(c, "debug", 0, t_maxmcp_server, debug);
    CLASS_ATTR_STYLE_LABEL(c, "debug", 0, "onoff", "Debug Mode");
    CLASS_ATTR_DEFAULT(c, "debug", 0, "0");

    class_register(CLASS_BOX, c);
    maxmcp_server_class = c;

    post("MaxMCP Server external loaded (WebSocket mode)");
}

void* maxmcp_server_new(t_symbol* s, long argc, t_atom* argv) {
    // Singleton check
    if (g_server_instance != nullptr) {
        object_error(nullptr, "maxmcp.server already exists! Only one instance allowed");
        return nullptr;
    }

    t_maxmcp_server* x = (t_maxmcp_server*)object_alloc(maxmcp_server_class);

    if (x) {
        // Initialize state
        x->initialized = false;
        x->protocol_version = "";
        x->running = true;

        // Initialize bridge management
        x->bridge_pid = 0;

        // Initialize attributes with defaults
        x->port = 7400;
        x->bridge_port = 18081;
        x->debug = false;

        // Create outlet for JSON-RPC responses (optional, for debugging)
        x->outlet_log = outlet_new(x, nullptr);

        // Process attributes
        attr_args_process(x, argc, argv);

        // Check environment variable for port
        if (const char* env_port = std::getenv("MAXMCP_PORT")) {
            x->port = std::atoi(env_port);
        }

        // Create WebSocket server
        x->ws_server = new WebSocketServer((int)x->port);

        // Set message callback - called when message arrives from client
        x->ws_server->set_message_callback([x](const std::string& client_id, const std::string& message) {
            try {
                if (x->debug) {
                    ConsoleLogger::log(("Processing WebSocket message from " + client_id + " (" + std::to_string(message.length()) + " bytes)").c_str());
                }

                // Route to MCP server (runs in WebSocket thread, but MCPServer is thread-safe)
                std::string response = MCPServer::get_instance()->handle_request_string(message);

                // Send response back via WebSocket
                x->ws_server->send_to_client(client_id, response);

                if (x->debug) {
                    ConsoleLogger::log(("Response sent to " + client_id + " (" + std::to_string(response.length()) + " bytes)").c_str());
                }

            } catch (const std::exception& e) {
                object_error((t_object*)x, "Request processing error: %s", e.what());

                // Send error response
                json error_response = {
                    {"jsonrpc", "2.0"},
                    {"error", {
                        {"code", -32603},
                        {"message", std::string("Internal error: ") + e.what()}
                    }},
                    {"id", nullptr}
                };

                x->ws_server->send_to_client(client_id, error_response.dump());
            }
        });

        // Start WebSocket server
        if (!x->ws_server->start()) {
            object_error((t_object*)x, "Failed to start WebSocket server");
            delete x->ws_server;
            x->ws_server = nullptr;
            return nullptr;
        }

        // Start MCP server
        MCPServer::get_instance()->start();

        // Set global singleton
        g_server_instance = x;

        object_post((t_object*)x, "MaxMCP Server started on port %ld", x->port);
        ConsoleLogger::log(("maxmcp.server: WebSocket server listening on port " + std::to_string(x->port)).c_str());
    }

    return x;
}

void maxmcp_server_free(t_maxmcp_server* x) {
    if (x) {
        // Stop running
        x->running = false;

        // Stop bridge if running
        maxmcp_server_stop_bridge(x);

        // Stop WebSocket server
        if (x->ws_server) {
            x->ws_server->stop();
            delete x->ws_server;
            x->ws_server = nullptr;
        }

        // Stop MCP server
        MCPServer::destroy_instance();

        ConsoleLogger::log("MaxMCP Server destroyed");

        // Clear singleton
        g_server_instance = nullptr;

        object_post((t_object*)x, "MaxMCP Server freed");
    }
}

void maxmcp_server_assist(t_maxmcp_server* x, void* b, long m, long a, char* s) {
    if (m == ASSIST_INLET) {
        snprintf(s, 256, "Control messages");
    } else {
        snprintf(s, 256, "Log output");
    }
}

/**
 * @brief Get global server instance (for client access)
 *
 * Used by client objects to check if server exists.
 *
 * @return Pointer to server instance, or nullptr if not created
 */
t_maxmcp_server* maxmcp_server_get_instance() {
    return g_server_instance;
}


/**
 * @brief Attribute setter for port
 */
t_max_err maxmcp_server_port_set(t_maxmcp_server* x, t_object* attr, long ac, t_atom* av) {
    if (ac && av) {
        x->port = atom_getlong(av);
        ConsoleLogger::log(("Port set to " + std::to_string(x->port)).c_str());
    }
    return MAX_ERR_NONE;
}

/**
 * @brief Attribute setter for bridge_port
 */
t_max_err maxmcp_server_bridge_port_set(t_maxmcp_server* x, t_object* attr, long ac, t_atom* av) {
    if (ac && av) {
        x->bridge_port = atom_getlong(av);
        object_post((t_object*)x, "Bridge port set to %ld", x->bridge_port);
    }
    return MAX_ERR_NONE;
}

/**
 * @brief Start WebSocket bridge process
 *
 * Launches the maxmcp-bridge binary with the specified port.
 *
 * @param x Pointer to maxmcp_server object
 * @param s Symbol (unused)
 * @param argc Argument count (optional: port number)
 * @param argv Argument vector
 */
void maxmcp_server_start_bridge(t_maxmcp_server* x, t_symbol* s, long argc, t_atom* argv) {
    // Check if bridge is already running
    if (x->bridge_pid > 0) {
        // Check if process is still alive
        if (kill(x->bridge_pid, 0) == 0) {
            object_post((t_object*)x, "Bridge already running (PID: %d)", x->bridge_pid);
            return;
        } else {
            // Process died, reset PID
            x->bridge_pid = 0;
        }
    }

    // Get port number (from argument or attribute)
    long port = x->bridge_port;
    if (argc > 0 && atom_gettype(argv) == A_LONG) {
        port = atom_getlong(argv);
    }

    // Construct bridge path (simplified approach)
    char package_path[MAX_PATH_CHARS];
    const char* home = getenv("HOME");
    
    if (home) {
        snprintf(package_path, MAX_PATH_CHARS,
                 "%s/Documents/Max 9/Packages/MaxMCP/support/maxmcp-bridge",
                 home);
    } else {
        object_error((t_object*)x, "Cannot determine HOME directory");
        return;
    }
    
    object_post((t_object*)x, "Bridge path: %s", package_path);

    // TEMPORARY: Disable fork/exec for testing
    object_post((t_object*)x, "Bridge launch disabled for testing (port: %ld)", port);
    ConsoleLogger::log("Bridge launch temporarily disabled for security testing");

    // TODO: Re-enable after security issue is resolved
    /*
    // Fork and exec bridge process
    pid_t pid = fork();

    if (pid < 0) {
        object_error((t_object*)x, "Failed to fork bridge process");
        return;
    }

    if (pid == 0) {
        // Child process: exec bridge
        char port_str[16];
        snprintf(port_str, sizeof(port_str), "%ld", port);

        // Execute bridge with port argument
        execl(package_path, "maxmcp-bridge", "--port", port_str, nullptr);

        // If we get here, exec failed
        perror("execl failed");
        exit(1);
    }

    // Parent process: store PID
    x->bridge_pid = pid;
    object_post((t_object*)x, "Bridge started on port %ld (PID: %d)", port, pid);
    ConsoleLogger::log(("maxmcp-bridge started on port " + std::to_string(port) + " (PID: " + std::to_string(pid) + ")").c_str());
    */
}

/**
 * @brief Stop WebSocket bridge process
 *
 * Terminates the bridge process if running.
 *
 * @param x Pointer to maxmcp_server object
 */
void maxmcp_server_stop_bridge(t_maxmcp_server* x) {
    if (x->bridge_pid > 0) {
        // Try graceful shutdown first (SIGTERM)
        if (kill(x->bridge_pid, SIGTERM) == 0) {
            object_post((t_object*)x, "Stopping bridge (PID: %d)...", x->bridge_pid);

            // Wait up to 2 seconds for graceful shutdown
            for (int i = 0; i < 20; i++) {
                int status;
                pid_t result = waitpid(x->bridge_pid, &status, WNOHANG);
                if (result == x->bridge_pid) {
                    object_post((t_object*)x, "Bridge stopped gracefully");
                    x->bridge_pid = 0;
                    return;
                }
                usleep(100000); // 100ms
            }

            // Force kill if still alive
            object_warn((t_object*)x, "Bridge did not stop gracefully, force killing...");
            kill(x->bridge_pid, SIGKILL);
            waitpid(x->bridge_pid, nullptr, 0);
        }

        x->bridge_pid = 0;
        object_post((t_object*)x, "Bridge stopped");
    }
}

