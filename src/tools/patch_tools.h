/**
    @file patch_tools.h
    MaxMCP - Patch Management Tools

    MCP tools for patch management operations:
    - list_active_patches
    - get_patch_info
    - get_frontmost_patch

    @ingroup maxmcp
*/

#ifndef PATCH_TOOLS_H
#define PATCH_TOOLS_H

#include <nlohmann/json.hpp>

#include <string>

// Forward declarations
struct _maxmcp;
typedef struct _maxmcp t_maxmcp;

namespace PatchTools {

using json = nlohmann::json;

/**
 * @brief Get the JSON schemas for all patch management tools
 *
 * Returns an array of tool definitions for:
 * - list_active_patches: List all registered MaxMCP client patches
 * - get_patch_info: Get detailed information about a specific patch
 * - get_frontmost_patch: Get the currently focused patch
 *
 * @return JSON array of tool schemas
 */
json get_tool_schemas();

/**
 * @brief Execute a patch management tool
 *
 * Dispatcher for patch management tools. Routes the tool name to the
 * appropriate handler function.
 *
 * @param tool The tool name to execute
 * @param params The tool parameters as JSON
 * @return JSON result from tool execution, or error if tool not found
 */
json execute(const std::string& tool, const json& params);

}  // namespace PatchTools

#endif  // PATCH_TOOLS_H
