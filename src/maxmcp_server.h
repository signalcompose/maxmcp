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
#include <string>
#include <atomic>

// Forward declaration
class WebSocketServer;

/**
 * @brief MaxMCP Server object structure
 *
 * Singleton Max external that manages MCP server lifecycle and UDP communication.
 */
typedef struct _maxmcp_server {
    t_object ob;              ///< Max object header (must be first)
    void* outlet_log;         ///< Outlet for log messages (optional)

    // MCP Protocol state
    bool initialized;         ///< MCP initialize handshake completed
    std::string protocol_version; ///< Negotiated protocol version
    std::atomic<bool> running; ///< Server running state

    // WebSocket server
    WebSocketServer* ws_server;    ///< WebSocket server instance

    // Bridge management
    pid_t bridge_pid;         ///< Bridge process ID (0 if not running)
    t_atom_long bridge_port;  ///< Bridge WebSocket port (@bridge_port attribute)

    // Attributes
    t_atom_long port;         ///< WebSocket server port (@port attribute)
    bool debug;               ///< Debug mode (@debug attribute)

} t_maxmcp_server;

// Function prototypes
void* maxmcp_server_new(t_symbol* s, long argc, t_atom* argv);
void maxmcp_server_free(t_maxmcp_server* x);
void maxmcp_server_assist(t_maxmcp_server* x, void* b, long m, long a, char* s);

// Bridge management methods
void maxmcp_server_start_bridge(t_maxmcp_server* x, t_symbol* s, long argc, t_atom* argv);
void maxmcp_server_stop_bridge(t_maxmcp_server* x);

// Attribute accessors
t_max_err maxmcp_server_port_set(t_maxmcp_server* x, t_object* attr, long ac, t_atom* av);
t_max_err maxmcp_server_bridge_port_set(t_maxmcp_server* x, t_object* attr, long ac, t_atom* av);

#endif // MAXMCP_SERVER_H
