/**
    @file maxmcp.h
    MaxMCP - MCP Server for Max/MSP

    Header file for MaxMCP external object.

    @ingroup maxmcp
*/

#ifndef MAXMCP_H
#define MAXMCP_H

#include "ext.h"          // Max SDK standard include
#include "ext_obex.h"     // Required for new style Max object
#include <string>

/**
 * @brief MaxMCP client object structure
 *
 * Client object that registers with global patch registry.
 * Each instance represents a patch controlled by Claude Code.
 */
typedef struct _maxmcp {
    t_object ob;              ///< Max object header (must be first)
    std::string patch_id;     ///< Auto-generated unique ID (8-char UUID)
    std::string display_name; ///< User-friendly name for patch
    std::string patcher_name; ///< Max patcher filename
    t_object* patcher;        ///< Reference to parent patcher object
} t_maxmcp;

// Function prototypes
void* maxmcp_new(t_symbol* s, long argc, t_atom* argv);
void maxmcp_free(t_maxmcp* x);
void maxmcp_assist(t_maxmcp* x, void* b, long m, long a, char* s);
void maxmcp_notify(t_maxmcp* x, t_symbol* s, t_symbol* msg, void* sender, void* data);

#endif // MAXMCP_H
