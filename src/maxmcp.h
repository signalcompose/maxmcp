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

/**
 * @brief MaxMCP object structure
 *
 * Contains all instance data for a MaxMCP external object.
 */
typedef struct _maxmcp {
    t_object ob;          ///< Max object header (must be first)
} t_maxmcp;

// Function prototypes
void* maxmcp_new(t_symbol* s, long argc, t_atom* argv);
void maxmcp_free(t_maxmcp* x);
void maxmcp_assist(t_maxmcp* x, void* b, long m, long a, char* s);

#endif // MAXMCP_H
