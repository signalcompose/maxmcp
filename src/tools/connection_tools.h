/**
    @file connection_tools.h
    MaxMCP - Connection Operation MCP Tools

    Tools for creating, removing, and querying patchcord connections between Max objects.

    @ingroup maxmcp
*/

#ifndef CONNECTION_TOOLS_H
#define CONNECTION_TOOLS_H

#include <string>

#include <nlohmann/json.hpp>

// Forward declarations
struct _maxmcp;
typedef struct _maxmcp t_maxmcp;

namespace ConnectionTools {

using json = nlohmann::json;

/**
 * Get tool schemas for connection operation tools.
 * Returns schema definitions for tools/list response.
 *
 * Tools included:
 * - connect_max_objects: Create a patchcord connection between two objects
 * - disconnect_max_objects: Remove a patchcord connection between two objects
 * - get_patchlines: List all patchlines with metadata (coordinates, color, etc.)
 *
 * @return JSON array of tool schema objects
 */
json get_tool_schemas();

/**
 * Execute a connection operation tool.
 *
 * @param tool Tool name
 * @param params Tool parameters
 * @return JSON result or error
 */
json execute(const std::string& tool, const json& params);

}  // namespace ConnectionTools

#endif  // CONNECTION_TOOLS_H
