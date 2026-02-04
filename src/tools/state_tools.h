/**
    @file state_tools.h
    MaxMCP - State MCP Tools

    Tools for patch state queries:
    - get_patch_lock_state
    - set_patch_lock_state
    - get_patch_dirty

    @ingroup maxmcp
*/

#ifndef STATE_TOOLS_H
#define STATE_TOOLS_H

#include <string>

#include <nlohmann/json.hpp>

// Forward declarations
struct _maxmcp;
typedef struct _maxmcp t_maxmcp;

namespace StateTools {

using json = nlohmann::json;

/**
 * Get tool schemas for state tools.
 * Returns schema definitions for tools/list response.
 *
 * Tools included:
 * - get_patch_lock_state: Query lock/edit state
 * - set_patch_lock_state: Set lock/edit state
 * - get_patch_dirty: Check for unsaved changes
 *
 * @return JSON array of tool schema objects
 */
json get_tool_schemas();

/**
 * Execute a state tool.
 *
 * @param tool Tool name
 * @param params Tool parameters
 * @return JSON result or error
 */
json execute(const std::string& tool, const json& params);

}  // namespace StateTools

#endif  // STATE_TOOLS_H
