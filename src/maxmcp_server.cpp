/**
    @file maxmcp_server.cpp
    MaxMCP Server - MCP Server Max External Implementation

    @ingroup maxmcp
*/

#include "maxmcp_server.h"
#include "mcp_server.h"
#include "utils/console_logger.h"
#include <nlohmann/json.hpp>
#include <iostream>
#include <sstream>
#include <thread>
#include <atomic>

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

    class_register(CLASS_BOX, c);
    maxmcp_server_class = c;

    post("MaxMCP Server external loaded");
}

// Forward declarations
static void maxmcp_server_stdin_qelem_fn(t_maxmcp_server* x);
static std::atomic<bool> g_stdin_thread_running(false);
static std::thread g_stdin_thread;

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
        x->input_buffer = "";

        // Create qelem for stdin processing (on main thread)
        x->stdin_qelem = qelem_new(x, (method)maxmcp_server_stdin_qelem_fn);

        // Create outlet for log messages (optional)
        x->outlet_log = outlet_new(x, nullptr);

        // Start MCP server
        MCPServer::get_instance()->start();

        // Set global singleton
        g_server_instance = x;

        // Start stdin reader thread
        g_stdin_thread_running = true;
        g_stdin_thread = std::thread([x]() {
            while (g_stdin_thread_running && x->running) {
                std::string line;
                if (std::getline(std::cin, line)) {
                    x->input_buffer += line + "\n";
                    // Trigger qelem to process on main thread
                    qelem_set(x->stdin_qelem);
                }
            }
        });

        ConsoleLogger::log("MaxMCP Server started with stdio communication");
        object_post((t_object*)x, "MaxMCP Server initialized (singleton)");
    }

    return x;
}

void maxmcp_server_free(t_maxmcp_server* x) {
    if (x) {
        // Stop stdin thread
        x->running = false;
        g_stdin_thread_running = false;
        if (g_stdin_thread.joinable()) {
            g_stdin_thread.join();
        }

        // Free qelem
        if (x->stdin_qelem) {
            qelem_free(x->stdin_qelem);
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
 * @brief Qelem callback for processing stdin on main thread
 *
 * Processes buffered stdin input line-by-line.
 * Called on Max main thread via qelem.
 *
 * @param x Server instance
 */
static void maxmcp_server_stdin_qelem_fn(t_maxmcp_server* x) {
    if (!x || x->input_buffer.empty()) {
        return;
    }

    // Process all complete lines in buffer
    std::istringstream stream(x->input_buffer);
    std::string line;
    std::string remaining;

    while (std::getline(stream, line)) {
        if (!line.empty()) {
            maxmcp_server_handle_request(x, line);
        }
    }

    // Clear processed buffer
    x->input_buffer.clear();
}

/**
 * @brief Handle incoming MCP request
 *
 * Parses JSON-RPC request and routes to appropriate handler.
 *
 * @param x Server instance
 * @param request JSON-RPC request string
 */
void maxmcp_server_handle_request(t_maxmcp_server* x, const std::string& request) {
    try {
        json req = json::parse(request);

        // Extract JSON-RPC fields
        std::string method = req.value("method", "");
        json params = req.value("params", json::object());
        auto id = req.value("id", json(nullptr));

        ConsoleLogger::log(("MCP Request: " + method).c_str());

        // Route to MCP server
        std::string response = MCPServer::get_instance()->handle_request_string(request);

        // Send response
        maxmcp_server_send_response(x, response);

    } catch (const std::exception& e) {
        object_error((t_object*)x, "JSON parse error: %s", e.what());

        // Send error response if request had an id
        json error_response = {
            {"jsonrpc", "2.0"},
            {"error", {
                {"code", -32700},
                {"message", "Parse error"}
            }},
            {"id", nullptr}
        };
        maxmcp_server_send_response(x, error_response.dump());
    }
}

/**
 * @brief Send MCP response to stdout
 *
 * Writes JSON-RPC response to stdout with newline.
 *
 * @param x Server instance
 * @param response JSON-RPC response string
 */
void maxmcp_server_send_response(t_maxmcp_server* x, const std::string& response) {
    std::cout << response << std::endl;
    std::cout.flush();

    ConsoleLogger::log(("MCP Response sent (" + std::to_string(response.length()) + " bytes)").c_str());
}
