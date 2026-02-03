/**
    @file hierarchy_tools.h
    MaxMCP - Patcher Hierarchy MCP Tools

    Tools for navigating patcher hierarchy:
    - get_parent_patcher
    - get_subpatchers

    @ingroup maxmcp
*/

#ifndef HIERARCHY_TOOLS_H
#define HIERARCHY_TOOLS_H

#include <nlohmann/json.hpp>

#include <string>

// Forward declarations
struct _maxmcp;
typedef struct _maxmcp t_maxmcp;

namespace HierarchyTools {

using json = nlohmann::json;

/**
 * Get tool schemas for hierarchy tools.
 * Returns schema definitions for tools/list response.
 *
 * Tools included:
 * - get_parent_patcher: Get the parent patcher of a subpatcher
 * - get_subpatchers: List all subpatchers in a patch
 *
 * @return JSON array of tool schema objects
 */
json get_tool_schemas();

/**
 * Execute a hierarchy tool.
 *
 * @param tool Tool name
 * @param params Tool parameters
 * @return JSON result or error, nullptr if tool not handled
 */
json execute(const std::string& tool, const json& params);

}  // namespace HierarchyTools

#endif  // HIERARCHY_TOOLS_H
