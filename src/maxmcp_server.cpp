/**
    @file maxmcp_server.cpp
    MaxMCP Server - MCP Server Max External Implementation

    @ingroup maxmcp
*/

#include "maxmcp_server.h"
#include "mcp_server.h"
#include "utils/console_logger.h"

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

void* maxmcp_server_new(t_symbol* s, long argc, t_atom* argv) {
    // Singleton check
    if (g_server_instance != nullptr) {
        object_error(nullptr, "maxmcp.server already exists! Only one instance allowed");
        return nullptr;
    }

    t_maxmcp_server* x = (t_maxmcp_server*)object_alloc(maxmcp_server_class);

    if (x) {
        // Create outlet for log messages (optional)
        x->outlet_log = outlet_new(x, nullptr);

        // Start MCP server
        MCPServer::get_instance()->start();

        ConsoleLogger::log("MaxMCP Server started");

        // Set global singleton
        g_server_instance = x;

        object_post((t_object*)x, "MaxMCP Server initialized (singleton)");
    }

    return x;
}

void maxmcp_server_free(t_maxmcp_server* x) {
    if (x) {
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
