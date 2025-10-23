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

    // Register attributes
    CLASS_ATTR_LONG(c, "port", 0, t_maxmcp_server, port);
    CLASS_ATTR_LABEL(c, "port", 0, "WebSocket Port");
    CLASS_ATTR_DEFAULT(c, "port", 0, "7400");
    CLASS_ATTR_ACCESSORS(c, "port", nullptr, (method)maxmcp_server_port_set);

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

        // Initialize attributes with defaults
        x->port = 7400;
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

