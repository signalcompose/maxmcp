/**
    @file patch_tools.cpp
    MaxMCP - Patch Management Tools Implementation

    @ingroup maxmcp
*/

#include "patch_tools.h"

#include "../utils/console_logger.h"
#include "../utils/patch_registry.h"
#include "tool_common.h"

namespace PatchTools {

// ============================================================================
// Tool Schemas
// ============================================================================

json get_tool_schemas() {
    return json::array({
        // list_active_patches
        {{"name", "list_active_patches"},
         {"description",
          "List all registered MaxMCP client patches. Optionally filter by group name."},
         {"inputSchema",
          {{"type", "object"},
           {"properties",
            {{"group",
              {{"type", "string"},
               {"description",
                "Optional group name to filter patches (e.g., 'synths', 'effects')"}}}}}}}},

        // get_patch_info
        {{"name", "get_patch_info"},
         {"description", "Get detailed information about a specific patch"},
         {"inputSchema",
          {{"type", "object"},
           {"properties",
            {{"patch_id", {{"type", "string"}, {"description", "Patch ID to query"}}}}},
           {"required", json::array({"patch_id"})}}}},

        // get_frontmost_patch
        {{"name", "get_frontmost_patch"},
         {"description", "Get the currently focused/frontmost patch"},
         {"inputSchema", {{"type", "object"}, {"properties", json::object()}}}}
    });
}

// ============================================================================
// Tool Execution
// ============================================================================

json execute(const std::string& tool, const json& params) {
    if (tool == "list_active_patches") {
        // Get list of active patches from global registry
        // Optionally filter by group
        std::string group_filter = params.value("group", "");
        json result = PatchRegistry::list_patches(group_filter);
        ConsoleLogger::log(("MCP: list_active_patches result: " + result.dump()).c_str());
        return result;

    } else if (tool == "get_patch_info") {
        // Get detailed information about a patch
        std::string patch_id = params.value("patch_id", "");

        if (patch_id.empty()) {
            return ToolCommon::missing_param_error("patch_id");
        }

        return PatchRegistry::get_patch_info(patch_id);

    } else if (tool == "get_frontmost_patch") {
        // Get the currently focused patch
        return PatchRegistry::get_frontmost_patch();
    }

    // Unknown tool - return nullptr to signal not handled
    return nullptr;
}

}  // namespace PatchTools
