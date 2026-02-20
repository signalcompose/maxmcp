/**
    @file utility_tools.h
    MaxMCP - Utility MCP Tools

    Tools for utility operations:
    - get_console_log
    - get_avoid_rect_position

    @ingroup maxmcp
*/

#ifndef UTILITY_TOOLS_H
#define UTILITY_TOOLS_H

#include <string>

#include <nlohmann/json.hpp>

// Forward declarations
struct _maxmcp;
typedef struct _maxmcp t_maxmcp;

namespace UtilityTools {

using json = nlohmann::json;

/**
 * @brief Get the JSON schemas for all utility tools
 *
 * Returns an array of tool definitions for:
 * - get_console_log: Retrieve recent Max Console messages
 * - get_avoid_rect_position: Find empty position for placing new objects
 *
 * @return JSON array of tool schemas
 */
json get_tool_schemas();

/**
 * @brief Execute a utility tool
 *
 * Dispatcher for utility tools. Routes the tool name to the
 * appropriate handler function.
 *
 * @param tool The tool name to execute
 * @param params The tool parameters as JSON
 * @return JSON result from tool execution, or error if tool not found
 */
json execute(const std::string& tool, const json& params);

}  // namespace UtilityTools

#endif  // UTILITY_TOOLS_H
