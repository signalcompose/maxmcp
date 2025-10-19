/**
    @file maxmcp_server.h
    MaxMCP Server - MCP Server Max External

    Singleton Max external object that manages the MCP server.
    One instance per Max application.

    @ingroup maxmcp
*/

#ifndef MAXMCP_SERVER_H
#define MAXMCP_SERVER_H

#include "ext.h"
#include "ext_obex.h"

/**
 * @brief MaxMCP Server object structure
 *
 * Singleton Max external that manages MCP server lifecycle.
 */
typedef struct _maxmcp_server {
    t_object ob;              ///< Max object header (must be first)
    void* outlet_log;         ///< Outlet for log messages (optional)
} t_maxmcp_server;

// Function prototypes
void* maxmcp_server_new(t_symbol* s, long argc, t_atom* argv);
void maxmcp_server_free(t_maxmcp_server* x);
void maxmcp_server_assist(t_maxmcp_server* x, void* b, long m, long a, char* s);

#endif // MAXMCP_SERVER_H
