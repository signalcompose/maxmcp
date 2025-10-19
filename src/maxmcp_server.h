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

/**
 * @brief MaxMCP Server object structure
 *
 * Singleton Max external that manages MCP server lifecycle and stdio communication.
 */
typedef struct _maxmcp_server {
    t_object ob;              ///< Max object header (must be first)
    void* outlet_log;         ///< Outlet for log messages (optional)

    // MCP Protocol state
    bool initialized;         ///< MCP initialize handshake completed
    std::string protocol_version; ///< Negotiated protocol version

    // stdio communication
    void* stdin_qelem;        ///< Qelem for stdin processing (main thread)
    std::string input_buffer; ///< Buffer for incomplete stdin lines
    bool running;             ///< Server running state

} t_maxmcp_server;

// Function prototypes
void* maxmcp_server_new(t_symbol* s, long argc, t_atom* argv);
void maxmcp_server_free(t_maxmcp_server* x);
void maxmcp_server_assist(t_maxmcp_server* x, void* b, long m, long a, char* s);

// MCP Protocol handlers
void maxmcp_server_stdin_task(t_maxmcp_server* x);
void maxmcp_server_handle_request(t_maxmcp_server* x, const std::string& request);
void maxmcp_server_send_response(t_maxmcp_server* x, const std::string& response);

#endif // MAXMCP_SERVER_H
