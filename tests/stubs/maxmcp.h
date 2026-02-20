/**
 * @file maxmcp.h (test stub)
 *
 * Minimal stub of maxmcp.h for unit tests compiled with MAXMCP_TEST_MODE.
 * Provides type definitions that would normally come from the Max SDK,
 * without requiring the actual Max SDK headers.
 *
 * This file is placed in tests/stubs/ and added to the include path
 * BEFORE the real src/ directory, so #include "maxmcp.h" resolves here
 * instead of src/maxmcp.h during test builds.
 */

#ifndef MAXMCP_H
#define MAXMCP_H

#include <atomic>
#include <string>

// Minimal Max SDK type stubs
struct _object {};
typedef struct _object t_object;

struct _symbol {
    char* s_name;
};
typedef struct _symbol t_symbol;

typedef long t_atom_long;
typedef long t_max_err;

struct _atom {};
typedef struct _atom t_atom;

// Forward declaration
class WebSocketServer;

/**
 * @brief MaxMCP object structure (test stub)
 *
 * Contains only the fields needed by tool modules in test mode.
 * Agent-mode fields and Max object header are omitted.
 */
typedef struct _maxmcp {
    t_object* ob;  ///< Placeholder for Max object header

    // Patch mode fields (used by tool modules)
    std::string patch_id;
    std::string display_name;
    std::string patcher_name;
    t_object* patcher;
    t_symbol* alias;
    t_symbol* group;
} t_maxmcp;

#endif  // MAXMCP_H
