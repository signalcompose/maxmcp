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

    t_connection_data(t_maxmcp* patch, const std::string& src_varname, long outlet,
                      const std::string& dst_varname, long inlet, DeferredResult* deferred_result)
        : patch(patch), src_varname(src_varname), outlet(outlet), dst_varname(dst_varname),
          inlet(inlet), deferred_result(deferred_result) {}

    virtual ~t_connection_data() = default;
};

struct t_set_midpoints_data : t_connection_data {
    std::vector<double> coords;  // flat [x1, y1, x2, y2, ...]

    t_set_midpoints_data(t_maxmcp* patch, const std::string& src_varname, long outlet,
                         const std::string& dst_varname, long inlet,
                         DeferredResult* deferred_result, std::vector<double> coords)
        : t_connection_data(patch, src_varname, outlet, dst_varname, inlet, deferred_result),
          coords(std::move(coords)) {}
};

struct t_get_patchlines_data {
    t_maxmcp* patch;
    DeferredResult* deferred_result;
};

// ============================================================================
// Production Code (Max SDK required)
// ============================================================================

#ifndef MAXMCP_TEST_MODE

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

/**
 * Deferred callback for getting patchline information.
 * Executes on the Max main thread via defer().
 */
static void get_patchlines_deferred(t_maxmcp* patch, t_symbol* s, long argc, t_atom* argv) {
    VALIDATE_DEFERRED_ARGS("get_patchlines_deferred");
    EXTRACT_DEFERRED_DATA_WITH_RESULT(t_get_patchlines_data, data, argv);

    json patchlines = json::array();
    t_object* patcher = data->patch->patcher;

    for (t_object* line = jpatcher_get_firstline(patcher); line;
         line = jpatchline_get_nextline(line)) {
        t_object* box1 = (t_object*)jpatchline_get_box1(line);
        t_object* box2 = (t_object*)jpatchline_get_box2(line);
        long outlet = jpatchline_get_outletnum(line);
        long inlet = jpatchline_get_inletnum(line);

        auto get_varname = [](t_object* box) -> std::string {
            t_symbol* v = object_attr_getsym(box, gensym("varname"));
            return (v && v->s_name) ? v->s_name : "";
        };

        double sx, sy, ex, ey;
        jpatchline_get_startpoint(line, &sx, &sy);
        jpatchline_get_endpoint(line, &ex, &ey);

        t_jrgba color = {0.0, 0.0, 0.0, 1.0};
        jpatchline_get_color(line, &color);

        long num_midpoints = jpatchline_get_nummidpoints(line);

        json pl = {{"src_varname", get_varname(box1)},
                   {"outlet", outlet},
                   {"dst_varname", get_varname(box2)},
                   {"inlet", inlet},
                   {"start_point", {{"x", sx}, {"y", sy}}},
                   {"end_point", {{"x", ex}, {"y", ey}}},
                   {"num_midpoints", num_midpoints},
                   {"hidden", (bool)jpatchline_get_hidden(line)},
                   {"color",
                    {{"r", color.red}, {"g", color.green}, {"b", color.blue}, {"a", color.alpha}}}};

        if (num_midpoints > 0) {
            // midpoints are stored as [x1, y1, x2, y2, ...] coordinate pairs
            long num_values = num_midpoints * 2;
            std::vector<double> coords(num_values, 0.0);
            long count =
                object_attr_getdouble_array(line, gensym("midpoints"), num_values, coords.data());
            if (count > 0) {
                json midpoints = json::array();
                for (long i = 0; i + 1 < count; i += 2) {
                    midpoints.push_back({{"x", coords[i]}, {"y", coords[i + 1]}});
                }
                pl["midpoints"] = midpoints;
            }
        }

        patchlines.push_back(pl);
    }

    COMPLETE_DEFERRED(data, (json{{"patch_id", data->patch->patch_id},
                                  {"patchlines", patchlines},
                                  {"count", patchlines.size()}}));
}

/**
 * Deferred callback for setting patchline midpoints.
 * Executes on the Max main thread via defer().
 */
static void set_patchline_midpoints_deferred(t_maxmcp* patch, t_symbol* s, long argc,
                                             t_atom* argv) {
    VALIDATE_DEFERRED_ARGS("set_patchline_midpoints_deferred");
    EXTRACT_DEFERRED_DATA_WITH_RESULT(t_set_midpoints_data, data, argv);

    t_object* src_box;
    t_object* dst_box;
    if (!find_connection_boxes(data, "SetMidpoints", src_box, dst_box))
        return;

    t_object* patcher = data->patch->patcher;
    bool found = false;

    for (t_object* line = jpatcher_get_firstline(patcher); line;
         line = jpatchline_get_nextline(line)) {
        t_object* line_box1 = (t_object*)jpatchline_get_box1(line);
        long line_outlet = jpatchline_get_outletnum(line);
        t_object* line_box2 = (t_object*)jpatchline_get_box2(line);
        long line_inlet = jpatchline_get_inletnum(line);

        if (line_box1 != src_box || line_outlet != data->outlet || line_box2 != dst_box ||
            line_inlet != data->inlet) {
            continue;
        }

        long count = (long)data->coords.size();
        if (count > 0) {
            object_attr_setdouble_array(line, gensym("midpoints"), count, data->coords.data());
        } else {
            // No setter API for clearing midpoints; reconnect the patchline
            bool was_hidden = jpatchline_get_hidden(line);
            t_jrgba saved_color;
            jpatchline_get_color(line, &saved_color);

            object_free(line);

            t_atom connect_args[4];
            atom_setobj(&connect_args[0], src_box);
            atom_setlong(&connect_args[1], data->outlet);
            atom_setobj(&connect_args[2], dst_box);
            atom_setlong(&connect_args[3], data->inlet);
            t_atom rv;
            line = (t_object*)object_method_typed(patcher, gensym("connect"), 4, connect_args,
                                                  &rv);
            if (line) {
                jpatchline_set_hidden(line, was_hidden);
                jpatchline_set_color(line, &saved_color);
            }
        }

        jpatcher_set_dirty(patcher, 1);

        // Read back for confirmation
        long new_num = jpatchline_get_nummidpoints(line);
        json midpoints_out = json::array();
        if (new_num > 0) {
            long num_values = new_num * 2;
            std::vector<double> readback(num_values, 0.0);
            long got =
                object_attr_getdouble_array(line, gensym("midpoints"), num_values, readback.data());
            for (long i = 0; i + 1 < got; i += 2) {
                midpoints_out.push_back({{"x", readback[i]}, {"y", readback[i + 1]}});
            }
        }

        std::string msg = "Set midpoints: " + data->src_varname + "[" +
                          std::to_string(data->outlet) + "] -> " + data->dst_varname + "[" +
                          std::to_string(data->inlet) + "] (" + std::to_string(new_num) +
                          " midpoints)";
        ConsoleLogger::log(msg.c_str());

        COMPLETE_DEFERRED(data, (json{{"result",
                                       {{"status", "success"},
                                        {"src_varname", data->src_varname},
                                        {"outlet", data->outlet},
                                        {"dst_varname", data->dst_varname},
                                        {"inlet", data->inlet},
                                        {"num_midpoints", new_num},
                                        {"midpoints", midpoints_out}}}}));
        found = true;
        break;
    }

    if (!found) {
        std::string msg = "Patchline not found: " + data->src_varname + "[" +
                          std::to_string(data->outlet) + "] -> " + data->dst_varname + "[" +
                          std::to_string(data->inlet) + "]";
        ConsoleLogger::log(msg.c_str());
        COMPLETE_DEFERRED(data, ToolCommon::make_error(ToolCommon::ErrorCode::INVALID_PARAMS, msg));
    }
}

// ============================================================================
// Tool Executors
// ============================================================================

/**
 * Execute connect_max_objects tool.
 * Creates a patchcord between two objects.
 */
static json execute_connect_max_objects(const json& params) {
    std::string patch_id = params.value("patch_id", "");
    std::string src_varname = params.value("src_varname", "");
    std::string dst_varname = params.value("dst_varname", "");
    long outlet = params.value("outlet", -1);
    long inlet = params.value("inlet", -1);

    if (patch_id.empty() || src_varname.empty() || dst_varname.empty() || outlet < 0 || inlet < 0) {
        return ToolCommon::make_error(ToolCommon::ErrorCode::INVALID_PARAMS,
                                      "Missing or invalid required parameters");
    }

    t_maxmcp* patch = PatchRegistry::find_patch(patch_id);
    if (!patch) {
        return ToolCommon::patch_not_found_error(patch_id);
    }

    auto* deferred_result = new DeferredResult();
    auto* data =
        new t_connection_data(patch, src_varname, outlet, dst_varname, inlet, deferred_result);

    t_atom a;
    atom_setobj(&a, data);
    defer(patch, (method)connect_objects_deferred, gensym("connect_objects"), 1, &a);

    if (!deferred_result->wait_for(ToolCommon::DEFAULT_DEFER_TIMEOUT)) {
        delete deferred_result;
        return ToolCommon::timeout_error("connecting objects");
    }

    json result = deferred_result->result;
    delete deferred_result;
    return result;
}

/**
 * Execute disconnect_max_objects tool.
 * Removes a patchcord between two objects.
 */
static json execute_disconnect_max_objects(const json& params) {
    std::string patch_id = params.value("patch_id", "");
    std::string src_varname = params.value("src_varname", "");
    std::string dst_varname = params.value("dst_varname", "");
    long outlet = params.value("outlet", -1);
    long inlet = params.value("inlet", -1);

    if (patch_id.empty() || src_varname.empty() || dst_varname.empty() || outlet < 0 || inlet < 0) {
        return ToolCommon::make_error(ToolCommon::ErrorCode::INVALID_PARAMS,
                                      "Missing or invalid required parameters");
    }

    t_maxmcp* patch = PatchRegistry::find_patch(patch_id);
    if (!patch) {
        return ToolCommon::patch_not_found_error(patch_id);
    }

    auto* deferred_result = new DeferredResult();
    auto* data =
        new t_connection_data(patch, src_varname, outlet, dst_varname, inlet, deferred_result);

    t_atom a;
    atom_setobj(&a, data);
    defer(patch, (method)disconnect_objects_deferred, gensym("disconnect_objects"), 1, &a);

    if (!deferred_result->wait_for(ToolCommon::DEFAULT_DEFER_TIMEOUT)) {
        delete deferred_result;
        return ToolCommon::timeout_error("disconnecting objects");
    }

    json result = deferred_result->result;
    delete deferred_result;
    return result;
}

/**
 * Execute get_patchlines tool.
 * Lists all patchlines in a patch with metadata.
 */
static json execute_get_patchlines(const json& params) {
    std::string patch_id = params.value("patch_id", "");
    if (patch_id.empty()) {
        return ToolCommon::missing_param_error("patch_id");
    }

    t_maxmcp* patch = PatchRegistry::find_patch(patch_id);
    if (!patch) {
        return ToolCommon::patch_not_found_error(patch_id);
    }

    auto* deferred_result = new DeferredResult();
    auto* data = new t_get_patchlines_data{patch, deferred_result};

    t_atom a;
    atom_setobj(&a, data);
    defer(patch, (method)get_patchlines_deferred, gensym("get_patchlines"), 1, &a);

    if (!deferred_result->wait_for(ToolCommon::HEAVY_OPERATION_TIMEOUT)) {
        delete deferred_result;
        return ToolCommon::timeout_error("getting patchlines");
    }

    json result = deferred_result->result;
    delete deferred_result;
    return {{"result", result}};
}

/**
 * Execute set_patchline_midpoints tool.
 * Sets midpoint coordinates for a patchcord.
 */
static json execute_set_patchline_midpoints(const json& params) {
    std::string patch_id = params.value("patch_id", "");
    std::string src_varname = params.value("src_varname", "");
    std::string dst_varname = params.value("dst_varname", "");
    long outlet = params.value("outlet", -1);
    long inlet = params.value("inlet", -1);

    if (patch_id.empty() || src_varname.empty() || dst_varname.empty() || outlet < 0 || inlet < 0) {
        return ToolCommon::make_error(ToolCommon::ErrorCode::INVALID_PARAMS,
                                      "Missing or invalid required parameters");
    }

    if (!params.contains("midpoints") || !params["midpoints"].is_array()) {
        return ToolCommon::make_error(ToolCommon::ErrorCode::INVALID_PARAMS,
                                      "Missing or invalid 'midpoints' array");
    }

    std::vector<double> coords;
    for (const auto& pt : params["midpoints"]) {
        if (!pt.is_object() || !pt.contains("x") || !pt.contains("y")) {
            return ToolCommon::make_error(ToolCommon::ErrorCode::INVALID_PARAMS,
                                          "Each midpoint must be an object with 'x' and 'y'");
        }
        try {
            coords.push_back(pt["x"].get<double>());
            coords.push_back(pt["y"].get<double>());
        } catch (const std::exception& e) {
            return ToolCommon::make_error(ToolCommon::ErrorCode::INVALID_PARAMS,
                                          std::string("Midpoint coordinate is not a number: ") +
                                              e.what());
        }
    }

    t_maxmcp* patch = PatchRegistry::find_patch(patch_id);
    if (!patch) {
        return ToolCommon::patch_not_found_error(patch_id);
    }

    auto* deferred_result = new DeferredResult();
    auto* data = new t_set_midpoints_data(patch, src_varname, outlet, dst_varname, inlet,
                                          deferred_result, std::move(coords));

    t_atom a;
    atom_setobj(&a, data);
    defer(patch, (method)set_patchline_midpoints_deferred, gensym("set_patchline_midpoints"), 1,
          &a);

    if (!deferred_result->wait_for(ToolCommon::DEFAULT_DEFER_TIMEOUT)) {
        delete deferred_result;
        return ToolCommon::timeout_error("setting patchline midpoints");
    }

    json result = deferred_result->result;
    delete deferred_result;
    return result;
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
             json::array({"patch_id", "src_varname", "outlet", "dst_varname", "inlet"})}}}},
         {{"name", "get_patchlines"},
          {"description",
           "List all patchlines (connections) in a patch with metadata "
           "including source/destination, coordinates, color, and visibility. "
           "Folded patchcords include midpoint coordinates as an array of {x, y} points"},
          {"inputSchema",
           {{"type", "object"},
            {"properties",
             {{"patch_id", {{"type", "string"}, {"description", "Patch ID to query"}}}}},
            {"required", json::array({"patch_id"})}}}},
         {{"name", "set_patchline_midpoints"},
          {"description",
           "Set midpoint coordinates for a patchcord. Pass an array of {x, y} objects "
           "to add or reposition midpoints (folding the cord). "
           "Pass an empty array to remove all midpoints and straighten the cord."},
          {"inputSchema",
           {{"type", "object"},
            {"properties",
             {{"patch_id",
               {{"type", "string"}, {"description", "Patch ID containing the patchcord"}}},
              {"src_varname", {{"type", "string"}, {"description", "Source object variable name"}}},
              {"outlet", {{"type", "number"}, {"description", "Source outlet index (0-based)"}}},
              {"dst_varname",
               {{"type", "string"}, {"description", "Destination object variable name"}}},
              {"inlet", {{"type", "number"}, {"description", "Destination inlet index (0-based)"}}},
              {"midpoints",
               {{"type", "array"},
                {"description",
                 "Array of midpoint coordinates. Each element must have 'x' and 'y'. "
                 "Pass [] to remove all midpoints."},
                {"items",
                 {{"type", "object"},
                  {"properties",
                   {{"x", {{"type", "number"}, {"description", "X coordinate"}}},
                    {"y", {{"type", "number"}, {"description", "Y coordinate"}}}}},
                  {"required", json::array({"x", "y"})}}}}}}},
            {"required", json::array({"patch_id", "src_varname", "outlet", "dst_varname", "inlet",
                                      "midpoints"})}}}}});
}

// ============================================================================
// Tool Dispatcher
// ============================================================================

json execute(const std::string& tool, const json& params) {
#ifdef MAXMCP_TEST_MODE
    if (tool == "connect_max_objects" || tool == "disconnect_max_objects" ||
        tool == "get_patchlines" || tool == "set_patchline_midpoints") {
        return ToolCommon::test_mode_error();
    }
    return nullptr;
#else
    if (tool == "connect_max_objects") {
        return execute_connect_max_objects(params);
    } else if (tool == "disconnect_max_objects") {
        return execute_disconnect_max_objects(params);
    } else if (tool == "get_patchlines") {
        return execute_get_patchlines(params);
    } else if (tool == "set_patchline_midpoints") {
        return execute_set_patchline_midpoints(params);
    }
    return nullptr;
#endif
}

}  // namespace ConnectionTools
