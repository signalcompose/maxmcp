/**
    @file screenshot_tools.h
    MaxMCP - Screenshot MCP Tools

    Tools for capturing patcher screenshots:
    - get_patcher_screenshot

    @ingroup maxmcp
*/

#ifndef SCREENSHOT_TOOLS_H
#define SCREENSHOT_TOOLS_H

#include <string>

#include <nlohmann/json.hpp>

// Forward declarations
struct _maxmcp;
typedef struct _maxmcp t_maxmcp;

namespace ScreenshotTools {

using json = nlohmann::json;

/**
 * @brief Get the JSON schemas for all screenshot tools
 *
 * Returns an array of tool definitions for:
 * - get_patcher_screenshot: Capture patcher as base64 PNG image
 *
 * @return JSON array of tool schemas
 */
json get_tool_schemas();

/**
 * @brief Execute a screenshot tool
 *
 * Dispatcher for screenshot tools. Routes the tool name to the
 * appropriate handler function.
 *
 * @param tool The tool name to execute
 * @param params The tool parameters as JSON
 * @return JSON result from tool execution, or null if tool not found
 */
json execute(const std::string& tool, const json& params);

}  // namespace ScreenshotTools

#endif  // SCREENSHOT_TOOLS_H
