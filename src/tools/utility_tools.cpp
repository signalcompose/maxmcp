/**
    @file utility_tools.cpp
    MaxMCP - Utility MCP Tools Implementation

    Implements utility tools:
    - get_console_log: Retrieve recent Max Console messages
    - get_avoid_rect_position: Find empty position for placing new objects

    @ingroup maxmcp
*/

#include "utility_tools.h"

#include "tool_common.h"

#ifndef MAXMCP_TEST_MODE
#include "ext.h"
#include "jpatcher_api.h"
#endif

#include "maxmcp.h"
#include "utils/console_logger.h"
#include "utils/patch_registry.h"

namespace UtilityTools {

using ToolCommon::DeferredResult;

// ============================================================================
// Data Structures for Deferred Callbacks
// ============================================================================

struct t_get_position_data {
    t_maxmcp* patch;
    double width;
    double height;
    DeferredResult* deferred_result;
};

// ============================================================================
// Deferred Callbacks
// ============================================================================

#ifndef MAXMCP_TEST_MODE

/**
 * @brief Deferred callback for finding empty position
 *
 * Executed on main thread via defer(). Scans existing objects and finds
 * position to the right of all existing objects with margin.
 */
static void get_position_deferred(t_maxmcp* patch, t_symbol* s, long argc, t_atom* argv) {
    VALIDATE_DEFERRED_ARGS("get_position_deferred");
    EXTRACT_DEFERRED_DATA_WITH_RESULT(t_get_position_data, data, argv);

    t_object* patcher = data->patch->patcher;
    double max_x = 50.0;   // Default starting position
    double start_y = 50.0;

    // Find bounding box of existing objects
    for (t_object* box = jpatcher_get_firstobject(patcher); box; box = jbox_get_nextobject(box)) {
        t_rect rect;
        jbox_get_patching_rect(box, &rect);

        // Update max_x to be past the rightmost object
        double box_right = rect.x + rect.width;
        if (box_right > max_x) {
            max_x = box_right;
        }
    }

    // Add margin and return position
    double margin = 50.0;
    double new_x = max_x + margin;
    double new_y = start_y;

    COMPLETE_DEFERRED(data, json({
        {"position", json::array({new_x, new_y})},
        {"rationale", "Positioned to the right of existing objects with 50px margin"}
    }));
}

#endif  // MAXMCP_TEST_MODE

// ============================================================================
// Tool Schema Definitions
// ============================================================================

json get_tool_schemas() {
    return json::array({
        // get_console_log
        {{"name", "get_console_log"},
         {"description", "Retrieve recent Max Console messages"},
         {"inputSchema",
          {{"type", "object"},
           {"properties",
            {{"lines",
              {{"type", "number"},
               {"description", "Number of recent lines (default: 50, max: 1000)"}}},
             {"clear",
              {{"type", "boolean"},
               {"description", "Clear log after reading (default: false)"}}}}}}}},

        // get_avoid_rect_position
        {{"name", "get_avoid_rect_position"},
         {"description", "Find an empty position for placing new objects"},
         {"inputSchema",
          {{"type", "object"},
           {"properties",
            {{"patch_id", {{"type", "string"}, {"description", "Patch ID to query"}}},
             {"width", {{"type", "number"}, {"description", "Object width (default: 50)"}}},
             {"height", {{"type", "number"}, {"description", "Object height (default: 20)"}}}}},
           {"required", json::array({"patch_id"})}}}}
    });
}

// ============================================================================
// Tool Executors
// ============================================================================

/**
 * @brief Execute get_console_log tool
 *
 * No defer needed - ConsoleLogger is thread-safe with mutex protection.
 */
static json execute_get_console_log(const json& params) {
    size_t lines = params.value("lines", 50);
    bool clear = params.value("clear", false);

    return ConsoleLogger::get_logs(lines, clear);
}

/**
 * @brief Execute get_avoid_rect_position tool
 */
static json execute_get_avoid_rect_position(const json& params) {
#ifdef MAXMCP_TEST_MODE
    return ToolCommon::make_error(-32603, "Not available in test mode");
#else
    std::string patch_id = params.value("patch_id", "");
    double width = params.value("width", 50.0);
    double height = params.value("height", 20.0);

    if (patch_id.empty()) {
        return ToolCommon::missing_param_error("patch_id");
    }

    t_maxmcp* patch = PatchRegistry::find_patch(patch_id);
    if (!patch) {
        return ToolCommon::patch_not_found_error(patch_id);
    }

    // Allocate DeferredResult on heap
    DeferredResult* deferred_result = new DeferredResult();
    t_get_position_data* data = new t_get_position_data{patch, width, height, deferred_result};

    // Create atom to hold pointer
    t_atom a;
    atom_setobj(&a, data);

    // Defer to main thread
    defer(patch, (method)get_position_deferred, gensym("get_position"), 1, &a);

    // Wait for defer callback to complete
    if (!deferred_result->wait_for(ToolCommon::DEFAULT_DEFER_TIMEOUT)) {
        delete deferred_result;
        return ToolCommon::timeout_error("waiting for position calculation");
    }

    json result = deferred_result->result;
    delete deferred_result;

    return {{"result", result}};
#endif
}

// ============================================================================
// Public Dispatcher
// ============================================================================

json execute(const std::string& tool, const json& params) {
    if (tool == "get_console_log") {
        return execute_get_console_log(params);
    } else if (tool == "get_avoid_rect_position") {
        return execute_get_avoid_rect_position(params);
    }

    // Tool not handled by this module - return nullptr to signal not handled
    return nullptr;
}

}  // namespace UtilityTools
