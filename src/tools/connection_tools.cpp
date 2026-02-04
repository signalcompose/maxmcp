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

// ============================================================================
// Data Structures for Deferred Callbacks
// ============================================================================

struct t_connect_objects_data {
    t_maxmcp* patch;
    std::string src_varname;
    long outlet;
    std::string dst_varname;
    long inlet;
};

struct t_disconnect_objects_data {
    t_maxmcp* patch;
    std::string src_varname;
    long outlet;
    std::string dst_varname;
    long inlet;
};

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
    EXTRACT_DEFERRED_DATA(t_connect_objects_data, data, argv);

    t_object* patcher = data->patch->patcher;

    // Find source and destination boxes by varname using PatchHelpers
    t_object* src_box = PatchHelpers::find_box_by_varname(patcher, data->src_varname);
    t_object* dst_box = PatchHelpers::find_box_by_varname(patcher, data->dst_varname);

    if (!src_box || !dst_box) {
        ConsoleLogger::log("ERROR: Cannot find source or destination box");
        delete data;
        return;
    }

    t_atom connect_args[4];
    atom_setobj(&connect_args[0], src_box);
    atom_setlong(&connect_args[1], data->outlet);
    atom_setobj(&connect_args[2], dst_box);
    atom_setlong(&connect_args[3], data->inlet);

    // Create connection
    t_atom result;
    object_method_typed(patcher, gensym("connect"), 4, connect_args, &result);

    std::string msg = "Connected: " + data->src_varname + "[" + std::to_string(data->outlet) +
                      "] -> " + data->dst_varname + "[" + std::to_string(data->inlet) + "]";
    ConsoleLogger::log(msg.c_str());

    delete data;
}

/**
 * Deferred callback for disconnecting Max objects.
 * Executes on the Max main thread via defer().
 */
static void disconnect_objects_deferred(t_maxmcp* patch, t_symbol* s, long argc, t_atom* argv) {
    VALIDATE_DEFERRED_ARGS("disconnect_objects_deferred");
    EXTRACT_DEFERRED_DATA(t_disconnect_objects_data, data, argv);

    // Find source and destination boxes using PatchHelpers
    t_object* patcher = data->patch->patcher;
    t_object* src_box = PatchHelpers::find_box_by_varname(patcher, data->src_varname);
    t_object* dst_box = PatchHelpers::find_box_by_varname(patcher, data->dst_varname);

    if (!src_box || !dst_box) {
        ConsoleLogger::log("Source or destination object not found for disconnect");
        delete data;
        return;
    }

    // Find and remove matching patchline
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
        ConsoleLogger::log("Connection not found for disconnect");
    }

    delete data;
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
        return ToolCommon::make_error(-32602, "Missing or invalid required parameters");
    }

    // Find patch
    t_maxmcp* patch = PatchRegistry::find_patch(patch_id);
    if (!patch) {
        return ToolCommon::patch_not_found_error(patch_id);
    }

#ifndef MAXMCP_TEST_MODE
    // Create defer data
    t_connect_objects_data* data =
        new t_connect_objects_data{patch, src_varname, outlet, dst_varname, inlet};

    // Create atom to hold pointer
    t_atom a;
    atom_setobj(&a, data);

    // Defer to main thread
    defer(patch, (method)connect_objects_deferred, gensym("connect_objects"), 1, &a);
#endif

    return {{"result",
             {{"status", "success"},
              {"patch_id", patch_id},
              {"src_varname", src_varname},
              {"outlet", outlet},
              {"dst_varname", dst_varname},
              {"inlet", inlet}}}};
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
        return ToolCommon::make_error(-32602, "Missing or invalid required parameters");
    }

    // Find patch
    t_maxmcp* patch = PatchRegistry::find_patch(patch_id);
    if (!patch) {
        return ToolCommon::patch_not_found_error(patch_id);
    }

#ifndef MAXMCP_TEST_MODE
    // Create defer data
    t_disconnect_objects_data* data =
        new t_disconnect_objects_data{patch, src_varname, outlet, dst_varname, inlet};

    // Create atom to hold pointer
    t_atom a;
    atom_setobj(&a, data);

    // Defer to main thread
    defer(patch, (method)disconnect_objects_deferred, gensym("disconnect_objects"), 1, &a);
#endif

    return {{"result",
             {{"status", "success"},
              {"patch_id", patch_id},
              {"src_varname", src_varname},
              {"outlet", outlet},
              {"dst_varname", dst_varname},
              {"inlet", inlet}}}};
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
