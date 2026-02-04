/**
    @file object_tools.h
    MaxMCP - Object Operation MCP Tools

    Tools for creating, modifying, and querying Max objects.

    @ingroup maxmcp
*/

#ifndef OBJECT_TOOLS_H
#define OBJECT_TOOLS_H

#include <string>

#include <nlohmann/json.hpp>

// Forward declarations
struct _maxmcp;
typedef struct _maxmcp t_maxmcp;

namespace ObjectTools {

using json = nlohmann::json;

/**
 * Get tool schemas for object operation tools.
 * Returns schema definitions for tools/list response.
 *
 * Tools included:
 * - add_max_object
 * - remove_max_object
 * - get_objects_in_patch
 * - set_object_attribute
 * - get_object_io_info
 * - get_object_hidden
 * - set_object_hidden
 * - redraw_object
 *
 * @return JSON array of tool schema objects
 */
json get_tool_schemas();

/**
 * Execute an object operation tool.
 *
 * @param tool Tool name
 * @param params Tool parameters
 * @return JSON result or error
 */
json execute(const std::string& tool, const json& params);

}  // namespace ObjectTools

#endif  // OBJECT_TOOLS_H
