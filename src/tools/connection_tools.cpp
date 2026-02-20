/**
    @file connection_tools.cpp
    MaxMCP - Connection Operation MCP Tools Implementation

    @ingroup maxmcp
*/

#include "connection_tools.h"

#include "tool_common.h"

#ifndef MAXMCP_TEST_MODE
#include "ext.h"
#include "ext_obex.h"

#include "jpatcher_api.h"
#endif

#include "maxmcp.h"
#include "utils/console_logger.h"
#include "utils/patch_helpers.h"
#include "utils/patch_registry.h"

namespace ConnectionTools {

using DeferredResult = ToolCommon::DeferredResult;

// ============================================================================
// Data Structures for Deferred Callbacks
// ============================================================================

struct t_connection_data {
    t_maxmcp* patch;
    std::string src_varname;
    long outlet;
    std::string dst_varname;
    long inlet;
    DeferredResult* deferred_result;
};

// Find source and destination boxes by varname, completing with error if not found.
// Returns true if both boxes were found, false otherwise (deferred already completed with error).
static bool find_connection_boxes(t_connection_data* data, const std::string& operation,
                                  t_object*& src_box, t_object*& dst_box) {
    t_object* patcher = data->patch->patcher;
    src_box = PatchHelpers::find_box_by_varname(patcher, data->src_varname);
    dst_box = PatchHelpers::find_box_by_varname(patcher, data->dst_varname);

    if (!src_box || !dst_box) {
        std::string msg = operation + " failed: ";
        if (!src_box) {
            msg += "source '" + data->src_varname + "' not found";
        }
        if (!src_box && !dst_box) {
            msg += ", ";
        }
        if (!dst_box) {
            msg += "destination '" + data->dst_varname + "' not found";
        }
        ConsoleLogger::log(msg.c_str());
        COMPLETE_DEFERRED(data, ToolCommon::make_error(ToolCommon::ErrorCode::INVALID_PARAMS, msg));
        return false;
    }
    return true;
}

// ============================================================================
// Deferred Callbacks (execute on Max main thread)
// ============================================================================

#ifndef MAXMCP_TEST_MODE

/**
 * Deferred callback for connecting Max objects.
 * Executes on the Max main thread via defer().
 */
static void connect_objects_deferred(t_maxmcp* patch, t_symbol* s, long argc, t_atom* argv) {
    VALIDATE_DEFERRED_ARGS("connect_objects_deferred");
    EXTRACT_DEFERRED_DATA_WITH_RESULT(t_connection_data, data, argv);

    t_object* src_box;
    t_object* dst_box;
    if (!find_connection_boxes(data, "Connect", src_box, dst_box))
        return;

    t_object* patcher = data->patch->patcher;

    t_atom connect_args[4];
    atom_setobj(&connect_args[0], src_box);
    atom_setlong(&connect_args[1], data->outlet);
    atom_setobj(&connect_args[2], dst_box);
    atom_setlong(&connect_args[3], data->inlet);

    // Create connection
    t_atom result;
    object_method_typed(patcher, gensym("connect"), 4, connect_args, &result);

    // Verify connection was created by searching for matching patchline
    bool verified = false;
    for (t_object* line = jpatcher_get_firstline(patcher); line;
         line = jpatchline_get_nextline(line)) {
        t_object* line_box1 = (t_object*)jpatchline_get_box1(line);
        long line_outlet = jpatchline_get_outletnum(line);
        t_object* line_box2 = (t_object*)jpatchline_get_box2(line);
        long line_inlet = jpatchline_get_inletnum(line);

        if (line_box1 == src_box && line_outlet == data->outlet && line_box2 == dst_box &&
            line_inlet == data->inlet) {
            verified = true;
            break;
        }
    }

    if (!verified) {
        std::string msg = "Connect failed: could not verify connection " + data->src_varname + "[" +
                          std::to_string(data->outlet) + "] -> " + data->dst_varname + "[" +
                          std::to_string(data->inlet) + "]";
        ConsoleLogger::log(msg.c_str());
        COMPLETE_DEFERRED(data, ToolCommon::make_error(ToolCommon::ErrorCode::INTERNAL_ERROR, msg));
        return;
    }

    jpatcher_set_dirty(data->patch->patcher, 1);

    std::string msg = "Connected: " + data->src_varname + "[" + std::to_string(data->outlet) +
                      "] -> " + data->dst_varname + "[" + std::to_string(data->inlet) + "]";
    ConsoleLogger::log(msg.c_str());

    COMPLETE_DEFERRED(data, (json{{"result",
                                   {{"status", "success"},
                                    {"src_varname", data->src_varname},
                                    {"outlet", data->outlet},
                                    {"dst_varname", data->dst_varname},
                                    {"inlet", data->inlet}}}}));
}

/**
 * Deferred callback for disconnecting Max objects.
 * Executes on the Max main thread via defer().
 */
static void disconnect_objects_deferred(t_maxmcp* patch, t_symbol* s, long argc, t_atom* argv) {
    VALIDATE_DEFERRED_ARGS("disconnect_objects_deferred");
    EXTRACT_DEFERRED_DATA_WITH_RESULT(t_connection_data, data, argv);

    t_object* src_box;
    t_object* dst_box;
    if (!find_connection_boxes(data, "Disconnect", src_box, dst_box))
        return;

    // Find and remove matching patchline
    t_object* patcher = data->patch->patcher;
    t_object* line = jpatcher_get_firstline(patcher);
    bool found = false;

    while (line) {
        t_object* line_box1 = (t_object*)jpatchline_get_box1(line);
        long line_outlet = jpatchline_get_outletnum(line);
        t_object* line_box2 = (t_object*)jpatchline_get_box2(line);
        long line_inlet = jpatchline_get_inletnum(line);

        if (line_box1 == src_box && line_outlet == data->outlet && line_box2 == dst_box &&
            line_inlet == data->inlet) {

            object_free(line);
            jpatcher_set_dirty(data->patch->patcher, 1);
            found = true;

            std::string msg = "Disconnected: " + data->src_varname + "[" +
                              std::to_string(data->outlet) + "] -> " + data->dst_varname + "[" +
                              std::to_string(data->inlet) + "]";
            ConsoleLogger::log(msg.c_str());
            break;
        }

        line = jpatchline_get_nextline(line);
    }

    if (!found) {
        std::string msg = "Connection not found: " + data->src_varname + "[" +
                          std::to_string(data->outlet) + "] -> " + data->dst_varname + "[" +
                          std::to_string(data->inlet) + "]";
        ConsoleLogger::log(msg.c_str());
        COMPLETE_DEFERRED(data, ToolCommon::make_error(ToolCommon::ErrorCode::INVALID_PARAMS, msg));
        return;
    }

    COMPLETE_DEFERRED(data, (json{{"result",
                                   {{"status", "success"},
                                    {"src_varname", data->src_varname},
                                    {"outlet", data->outlet},
                                    {"dst_varname", data->dst_varname},
                                    {"inlet", data->inlet}}}}));
}

#endif  // MAXMCP_TEST_MODE

// ============================================================================
// Tool Schemas
// ============================================================================

json get_tool_schemas() {
    return json::array(
        {{{"name", "connect_max_objects"},
          {"description", "Create a patchcord connection between two Max objects"},
          {"inputSchema",
           {{"type", "object"},
            {"properties",
             {{"patch_id",
               {{"type", "string"}, {"description", "Patch ID containing the objects"}}},
              {"src_varname", {{"type", "string"}, {"description", "Source object variable name"}}},
              {"outlet", {{"type", "number"}, {"description", "Source outlet index (0-based)"}}},
              {"dst_varname",
               {{"type", "string"}, {"description", "Destination object variable name"}}},
              {"inlet",
               {{"type", "number"}, {"description", "Destination inlet index (0-based)"}}}}},
            {"required",
             json::array({"patch_id", "src_varname", "outlet", "dst_varname", "inlet"})}}}},
         {{"name", "disconnect_max_objects"},
          {"description", "Remove a patchcord connection between two Max objects"},
          {"inputSchema",
           {{"type", "object"},
            {"properties",
             {{"patch_id",
               {{"type", "string"}, {"description", "Patch ID containing the objects"}}},
              {"src_varname", {{"type", "string"}, {"description", "Source object variable name"}}},
              {"outlet", {{"type", "number"}, {"description", "Source outlet index (0-based)"}}},
              {"dst_varname",
               {{"type", "string"}, {"description", "Destination object variable name"}}},
              {"inlet",
               {{"type", "number"}, {"description", "Destination inlet index (0-based)"}}}}},
            {"required",
             json::array({"patch_id", "src_varname", "outlet", "dst_varname", "inlet"})}}}}});
}

// ============================================================================
// Tool Executors
// ============================================================================

/**
 * Execute connect_max_objects tool.
 * Creates a patchcord between two objects.
 */
static json execute_connect_max_objects(const json& params) {
    // Parse parameters
    std::string patch_id = params.value("patch_id", "");
    std::string src_varname = params.value("src_varname", "");
    std::string dst_varname = params.value("dst_varname", "");
    long outlet = params.value("outlet", -1);
    long inlet = params.value("inlet", -1);

    if (patch_id.empty() || src_varname.empty() || dst_varname.empty() || outlet < 0 || inlet < 0) {
        return ToolCommon::make_error(ToolCommon::ErrorCode::INVALID_PARAMS,
                                      "Missing or invalid required parameters");
    }

    // Find patch
    t_maxmcp* patch = PatchRegistry::find_patch(patch_id);
    if (!patch) {
        return ToolCommon::patch_not_found_error(patch_id);
    }

#ifndef MAXMCP_TEST_MODE
    // Create deferred result for synchronization
    auto* deferred_result = new DeferredResult();

    // Create defer data
    auto* data =
        new t_connection_data{patch, src_varname, outlet, dst_varname, inlet, deferred_result};

    // Create atom to hold pointer
    t_atom a;
    atom_setobj(&a, data);

    // Defer to main thread
    defer(patch, (method)connect_objects_deferred, gensym("connect_objects"), 1, &a);

    // Wait for completion
    if (!deferred_result->wait_for(ToolCommon::DEFAULT_DEFER_TIMEOUT)) {
        delete deferred_result;
        return ToolCommon::timeout_error("connecting objects");
    }

    json result = deferred_result->result;
    delete deferred_result;
    return result;
#else
    return {{"result",
             {{"status", "mock_success"},
              {"warning", "Test mode - no actual connection made"},
              {"patch_id", patch_id},
              {"src_varname", src_varname},
              {"outlet", outlet},
              {"dst_varname", dst_varname},
              {"inlet", inlet}}}};
#endif
}

/**
 * Execute disconnect_max_objects tool.
 * Removes a patchcord between two objects.
 */
static json execute_disconnect_max_objects(const json& params) {
    // Parse parameters
    std::string patch_id = params.value("patch_id", "");
    std::string src_varname = params.value("src_varname", "");
    std::string dst_varname = params.value("dst_varname", "");
    long outlet = params.value("outlet", -1);
    long inlet = params.value("inlet", -1);

    if (patch_id.empty() || src_varname.empty() || dst_varname.empty() || outlet < 0 || inlet < 0) {
        return ToolCommon::make_error(ToolCommon::ErrorCode::INVALID_PARAMS,
                                      "Missing or invalid required parameters");
    }

    // Find patch
    t_maxmcp* patch = PatchRegistry::find_patch(patch_id);
    if (!patch) {
        return ToolCommon::patch_not_found_error(patch_id);
    }

#ifndef MAXMCP_TEST_MODE
    // Create deferred result for synchronization
    auto* deferred_result = new DeferredResult();

    // Create defer data
    auto* data =
        new t_connection_data{patch, src_varname, outlet, dst_varname, inlet, deferred_result};

    // Create atom to hold pointer
    t_atom a;
    atom_setobj(&a, data);

    // Defer to main thread
    defer(patch, (method)disconnect_objects_deferred, gensym("disconnect_objects"), 1, &a);

    // Wait for completion
    if (!deferred_result->wait_for(ToolCommon::DEFAULT_DEFER_TIMEOUT)) {
        delete deferred_result;
        return ToolCommon::timeout_error("disconnecting objects");
    }

    json result = deferred_result->result;
    delete deferred_result;
    return result;
#else
    return {{"result",
             {{"status", "mock_success"},
              {"warning", "Test mode - no actual disconnection made"},
              {"patch_id", patch_id},
              {"src_varname", src_varname},
              {"outlet", outlet},
              {"dst_varname", dst_varname},
              {"inlet", inlet}}}};
#endif
}

// ============================================================================
// Tool Dispatcher
// ============================================================================

json execute(const std::string& tool, const json& params) {
    if (tool == "connect_max_objects") {
        return execute_connect_max_objects(params);
    } else if (tool == "disconnect_max_objects") {
        return execute_disconnect_max_objects(params);
    }

    // Tool not handled by this module - return nullptr to signal routing should continue
    return nullptr;
}

}  // namespace ConnectionTools
