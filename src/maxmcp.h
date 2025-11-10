/**
    @file maxmcp.h
    MaxMCP - MCP Server for Max/MSP

    Header file for MaxMCP external object.
    Supports two modes: agent (MCP server) and patch (patch registration).

    @ingroup maxmcp
*/

#ifndef MAXMCP_H
#define MAXMCP_H

#include "ext.h"          // Max SDK standard include
#include "ext_obex.h"     // Required for new style Max object
#include <string>
#include <atomic>

// Forward declaration
class WebSocketServer;

/**
 * @brief MaxMCP unified object structure
 *
 * Supports two modes:
 * - "agent": MCP server mode (WebSocket server, MCP protocol handler)
 * - "patch": Patch registration mode (registers patch with global registry)
 *
 * Mode is determined by @mode attribute:
 *   [maxmcp @mode agent]                              <- Agent mode
 *   [maxmcp @mode patch @alias my-synth @group synth] <- Patch mode (default)
 */
typedef struct _maxmcp {
    t_object ob;              ///< Max object header (must be first)

    // Mode selection
    t_symbol* mode;           ///< Operation mode: "agent" or "patch" (@mode attribute)

    // === AGENT MODE FIELDS ===
    void* outlet_log;         ///< Outlet for log messages (agent mode)
    bool initialized;         ///< MCP initialize handshake completed (agent mode)
    std::string protocol_version; ///< Negotiated protocol version (agent mode)
    std::atomic<bool> running; ///< Server running state (agent mode)
    WebSocketServer* ws_server; ///< WebSocket server instance (agent mode)
    t_atom_long port;         ///< WebSocket server port (agent mode, @port attribute)
    bool debug;               ///< Debug mode (agent mode, @debug attribute)

    // === PATCH MODE FIELDS ===
    std::string patch_id;     ///< Auto-generated unique ID (patch mode)
    std::string display_name; ///< User-friendly name for patch (patch mode)
    std::string patcher_name; ///< Max patcher filename (patch mode)
    t_object* patcher;        ///< Reference to parent patcher object (patch mode)
    t_symbol* alias;          ///< Custom patch ID override (patch mode, @alias attribute)
    t_symbol* group;          ///< Patch group name (patch mode, @group attribute)
} t_maxmcp;

// Function prototypes
void* maxmcp_new(t_symbol* s, long argc, t_atom* argv);
void maxmcp_free(t_maxmcp* x);
void maxmcp_assist(t_maxmcp* x, void* b, long m, long a, char* s);

// Patch mode methods
void maxmcp_notify(t_maxmcp* x, t_symbol* s, t_symbol* msg, void* sender, void* data);

// Agent mode methods
void maxmcp_start(t_maxmcp* x);
void maxmcp_stop(t_maxmcp* x);
void maxmcp_send_ready_bang(t_maxmcp* x, t_symbol* s, long argc, t_atom* argv);

// Attribute accessors
t_max_err maxmcp_port_set(t_maxmcp* x, t_object* attr, long ac, t_atom* av);

#endif // MAXMCP_H
