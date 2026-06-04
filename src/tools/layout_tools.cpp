/**
    @file layout_tools.cpp
    MaxMCP - Layout Validation MCP Tools Implementation

    Max-facing glue for validate_layout: walks a patcher for object rects and
    patchline geometry, then runs the pure checks in run_layout_checks() (defined
    in layout_checks.cpp, which delegates to src/utils/geometry.h). Read-only —
    never mutates the patch.

    @ingroup maxmcp
*/

#include "layout_tools.h"

#include "tool_common.h"

#include <set>
#include <string>
#include <unordered_map>
#include <utility>

#ifndef MAXMCP_TEST_MODE
#include "ext.h"
#include "ext_obex.h"

#include "jpatcher_api.h"
#endif

#include "maxmcp.h"
#include "utils/console_logger.h"
#include "utils/geometry.h"
#include "utils/io_geometry.h"
#include "utils/patch_helpers.h"
#include "utils/patch_registry.h"

namespace LayoutTools {

using ToolCommon::DeferredResult;

// ============================================================================
// Data Structures for Deferred Callbacks
// ============================================================================

struct t_validate_layout_data {
    t_maxmcp* patch;
    LayoutCheckOptions options;
    std::vector<std::string> scope_varnames;
    DeferredResult* deferred_result;
};

struct t_get_io_position_data {
    t_maxmcp* patch;
    std::string varname;
    geometry::IoSide side;
    DeferredResult* deferred_result;
};

// ============================================================================
// Production Code (Max SDK required)
// ============================================================================

#ifndef MAXMCP_TEST_MODE

// Walk the patcher and collect the objects participating in the checks, filtered
// by mode (presentation only considers objects shown in presentation) and by
// scope (when non-empty). Every box — included or not — is recorded in @p id_of
// (patcher order) so cords can resolve their endpoints; @p included_ids gets the
// ids that passed the filter, used to keep scoped cord checks within the section.
static std::vector<LayoutObject> extract_layout_objects(t_object* patcher,
                                                        const LayoutCheckOptions& options,
                                                        const std::set<std::string>& scope,
                                                        std::unordered_map<t_object*, int>& id_of,
                                                        std::set<int>& included_ids) {
    const bool presentation = options.presentation_mode;
    const bool use_scope = !scope.empty();

    std::vector<LayoutObject> objects;
    int index = 0;
    for (t_object* box = jpatcher_get_firstobject(patcher); box;
         box = jbox_get_nextobject(box), ++index) {
        id_of[box] = index;

        const bool in_presentation = object_attr_getlong(box, gensym("presentation")) != 0;
        if (presentation && !in_presentation) {
            continue;  // presentation mode only considers objects shown there
        }

        const std::string varname = PatchHelpers::get_box_varname(box);
        if (use_scope && scope.find(varname) == scope.end()) {
            continue;
        }

        const std::string maxclass_str = PatchHelpers::get_box_maxclass(box);

        t_rect rect;
        if (presentation) {
            jbox_get_presentation_rect(box, &rect);
        } else {
            jbox_get_patching_rect(box, &rect);
        }

        objects.push_back({index,
                           varname,
                           maxclass_str,
                           {rect.x, rect.y, rect.width, rect.height},
                           in_presentation});
        included_ids.insert(index);
    }
    return objects;
}

// Walk the patcher's patchlines and collect them as LayoutCords, resolving each
// endpoint to its object id via @p id_of. When scoping is active, cords with an
// endpoint outside @p included_ids are skipped so checks stay within the section.
static std::vector<LayoutCord> extract_layout_cords(t_object* patcher,
                                                    const std::unordered_map<t_object*, int>& id_of,
                                                    const std::set<int>& included_ids,
                                                    bool use_scope) {
    std::vector<LayoutCord> cords;
    for (t_object* line = jpatcher_get_firstline(patcher); line;
         line = jpatchline_get_nextline(line)) {
        t_object* box1 = (t_object*)jpatchline_get_box1(line);
        t_object* box2 = (t_object*)jpatchline_get_box2(line);

        auto it1 = id_of.find(box1);
        auto it2 = id_of.find(box2);
        const int src_id = (it1 != id_of.end()) ? it1->second : -1;
        const int dst_id = (it2 != id_of.end()) ? it2->second : -1;

        if (use_scope && (included_ids.find(src_id) == included_ids.end() ||
                          included_ids.find(dst_id) == included_ids.end())) {
            continue;
        }

        double sx, sy, ex, ey;
        jpatchline_get_startpoint(line, &sx, &sy);
        jpatchline_get_endpoint(line, &ex, &ey);

        std::vector<geometry::Point> midpoints;
        long num_midpoints = jpatchline_get_nummidpoints(line);
        if (num_midpoints > 0) {
            long num_values = num_midpoints * 2;
            std::vector<double> coords(num_values, 0.0);
            long count =
                object_attr_getdouble_array(line, gensym("midpoints"), num_values, coords.data());
            for (long i = 0; i + 1 < count; i += 2) {
                midpoints.push_back({coords[i], coords[i + 1]});
            }
        }

        cords.push_back({src_id,
                         PatchHelpers::get_box_varname(box1),
                         jpatchline_get_outletnum(line),
                         dst_id,
                         PatchHelpers::get_box_varname(box2),
                         jpatchline_get_inletnum(line),
                         {sx, sy},
                         {ex, ey},
                         std::move(midpoints)});
    }
    return cords;
}

/**
 * Deferred callback for validate_layout.
 * Executes on the Max main thread via defer(). Extracts object rects and
 * patchline geometry into plain structs, then runs the pure checks.
 */
static void validate_layout_deferred(t_maxmcp* patch, t_symbol* s, long argc, t_atom* argv) {
    VALIDATE_DEFERRED_ARGS("validate_layout_deferred");
    EXTRACT_DEFERRED_DATA_WITH_RESULT(t_validate_layout_data, data, argv);

    t_object* patcher = data->patch->patcher;
    const std::set<std::string> scope(data->scope_varnames.begin(), data->scope_varnames.end());

    std::unordered_map<t_object*, int> id_of;
    std::set<int> included_ids;
    std::vector<LayoutObject> objects =
        extract_layout_objects(patcher, data->options, scope, id_of, included_ids);

    // Cords are not drawn in presentation mode, so only the patching layout needs them.
    std::vector<LayoutCord> cords;
    if (!data->options.presentation_mode) {
        cords = extract_layout_cords(patcher, id_of, included_ids, !scope.empty());
    }

    json result = run_layout_checks(objects, cords, data->options);
    result["patch_id"] = data->patch->patch_id;

    COMPLETE_DEFERRED(data, result);
}

/**
 * Execute validate_layout tool.
 */
static json execute_validate_layout(const json& params) {
    std::string patch_id = params.value("patch_id", "");
    if (patch_id.empty()) {
        return ToolCommon::missing_param_error("patch_id");
    }

    t_maxmcp* patch = PatchRegistry::find_patch(patch_id);
    if (!patch) {
        return ToolCommon::patch_not_found_error(patch_id);
    }

    LayoutCheckOptions options;
    options.epsilon = params.value("epsilon", 2.0);
    options.presentation_mode = (params.value("mode", "patching") == "presentation");

    // Empty/absent "checks" means run every check; otherwise enable only the
    // named ones (presentation_overlap shares the overlap pass).
    if (params.contains("checks") && params["checks"].is_array() && !params["checks"].empty()) {
        std::set<std::string> checks;
        for (const auto& c : params["checks"]) {
            if (c.is_string()) {
                checks.insert(c.get<std::string>());
            }
        }
        options.check_upward = checks.count("upward") > 0;
        options.check_overlap =
            checks.count("overlap") > 0 || checks.count("presentation_overlap") > 0;
        options.check_cord_object = checks.count("cord_object") > 0;
        options.check_cord_cord = checks.count("cord_cord") > 0;
    }

    std::vector<std::string> scope_varnames;
    if (params.contains("scope_varnames") && params["scope_varnames"].is_array()) {
        for (const auto& v : params["scope_varnames"]) {
            if (v.is_string()) {
                scope_varnames.push_back(v.get<std::string>());
            }
        }
    }

    auto* deferred_result = new DeferredResult();
    auto* data =
        new t_validate_layout_data{patch, options, std::move(scope_varnames), deferred_result};

    t_atom a;
    atom_setobj(&a, data);
    defer(patch, (method)validate_layout_deferred, gensym("validate_layout"), 1, &a);

    if (!deferred_result->wait_for(ToolCommon::HEAVY_OPERATION_TIMEOUT)) {
        delete deferred_result;
        return ToolCommon::timeout_error("validating layout");
    }

    json result = deferred_result->result;
    delete deferred_result;
    return {{"result", result}};
}

/**
 * Deferred callback for get_io_position.
 * Executes on the Max main thread via defer(). Reads the box rect, counts, and
 * drawfirstin, then delegates to the pure placement rule in io_geometry.h.
 */
static void get_io_position_deferred(t_maxmcp* patch, t_symbol* s, long argc, t_atom* argv) {
    VALIDATE_DEFERRED_ARGS("get_io_position_deferred");
    EXTRACT_DEFERRED_DATA_WITH_RESULT(t_get_io_position_data, data, argv);

    t_object* box = PatchHelpers::find_box_by_varname(data->patch->patcher, data->varname);
    if (!box) {
        COMPLETE_DEFERRED(data, ToolCommon::object_not_found_error(data->varname));
        return;
    }

    t_rect rect;
    jbox_get_patching_rect(box, &rect);
    const geometry::Rect box_rect{rect.x, rect.y, rect.width, rect.height};

    const bool is_inlet = (data->side == geometry::IoSide::Inlet);
    const long logical_count =
        is_inlet ? PatchHelpers::get_inlet_count(box) : PatchHelpers::get_outlet_count(box);
    // drawfirstin only suppresses an inlet nub; outlets always draw every nub.
    const bool draw_first_in = is_inlet ? (jbox_get_drawfirstin(box) != 0) : true;
    const std::string maxclass = PatchHelpers::get_box_maxclass(box);

    const std::vector<geometry::IoPosition> io = geometry::io_positions(
        box_rect, static_cast<int>(logical_count), data->side, draw_first_in, maxclass);

    json positions = json::array();
    for (const geometry::IoPosition& p : io) {
        positions.push_back({{"index", p.index}, {"x", p.center.x}, {"y", p.center.y}});
    }

    COMPLETE_DEFERRED(data, (json{{"varname", data->varname},
                                  {"side", is_inlet ? "inlet" : "outlet"},
                                  {"count", positions.size()},
                                  {"positions", positions}}));
}

/**
 * Execute get_io_position tool.
 */
static json execute_get_io_position(const json& params) {
    std::string patch_id = params.value("patch_id", "");
    std::string varname = params.value("varname", "");
    std::string side_str = params.value("side", "");

    if (patch_id.empty() || varname.empty() || side_str.empty()) {
        return ToolCommon::make_error(ToolCommon::ErrorCode::INVALID_PARAMS,
                                      "Missing required parameters: patch_id, varname, and side");
    }
    if (side_str != "inlet" && side_str != "outlet") {
        return ToolCommon::make_error(ToolCommon::ErrorCode::INVALID_PARAMS,
                                      "side must be \"inlet\" or \"outlet\"");
    }

    t_maxmcp* patch = PatchRegistry::find_patch(patch_id);
    if (!patch) {
        return ToolCommon::patch_not_found_error(patch_id);
    }

    const geometry::IoSide side =
        (side_str == "inlet") ? geometry::IoSide::Inlet : geometry::IoSide::Outlet;

    auto* deferred_result = new DeferredResult();
    auto* data = new t_get_io_position_data{patch, varname, side, deferred_result};

    t_atom a;
    atom_setobj(&a, data);
    defer(patch, (method)get_io_position_deferred, gensym("get_io_position"), 1, &a);

    if (!deferred_result->wait_for(ToolCommon::DEFAULT_DEFER_TIMEOUT)) {
        delete deferred_result;
        return ToolCommon::timeout_error("getting io position");
    }

    json result = deferred_result->result;
    delete deferred_result;

    if (result.contains("error")) {
        return result;
    }
    return {{"result", result}};
}

#endif  // MAXMCP_TEST_MODE

// ============================================================================
// Tool Schemas
// ============================================================================

json get_tool_schemas() {
    return json::array(
        {{{"name", "validate_layout"},
          {"description",
           "Machine-check a patch's layout geometry (read-only) and return a structured "
           "findings list. Replaces the manual Phase 8 checks of organize-patch: object "
           "overlaps, upward patchcords, patchcords crossing unrelated objects, and "
           "collinear overlapping cords. Call this before saving and fix findings until "
           "'clean' is true."},
          {"inputSchema",
           {{"type", "object"},
            {"properties",
             {{"patch_id", {{"type", "string"}, {"description", "Patch ID to validate"}}},
              {"scope_varnames",
               {{"type", "array"},
                {"items", {{"type", "string"}}},
                {"description", "Optional. Limit checks to these objects (and cords between "
                                "them). Omit to check the whole patch."}}},
              {"mode",
               {{"type", "string"},
                {"enum", json::array({"patching", "presentation"})},
                {"description", "Coordinate space to check. Default 'patching'. 'presentation' "
                                "checks presentation_rect overlaps of objects shown in "
                                "presentation (cord checks are skipped)."}}},
              {"checks",
               {{"type", "array"},
                {"items",
                 {{"type", "string"},
                  {"enum", json::array({"upward", "overlap", "cord_object", "cord_cord",
                                        "presentation_overlap"})}}},
                {"description", "Optional subset of checks to run. Omit to run all."}}},
              {"epsilon",
               {{"type", "number"},
                {"description", "Tolerance in pixels for all checks (default: 2.0)."}}}}},
            {"required", json::array({"patch_id"})}}}},
         {{"name", "get_io_position"},
          {"description",
           "Return the pixel center (x, y) of each inlet (top edge) or outlet (bottom edge) "
           "of an object, computed with Max's calibrated equal-spacing rule. Use instead of "
           "hand-computing nub x positions for width/alignment work. 'index' is the logical "
           "inlet/outlet number (matches connect_max_objects)."},
          {"inputSchema",
           {{"type", "object"},
            {"properties",
             {{"patch_id", {{"type", "string"}, {"description", "Patch ID containing the object"}}},
              {"varname", {{"type", "string"}, {"description", "Object varname to query"}}},
              {"side",
               {{"type", "string"},
                {"enum", json::array({"inlet", "outlet"})},
                {"description", "Which edge to compute: 'inlet' (top) or 'outlet' (bottom)."}}}}},
            {"required", json::array({"patch_id", "varname", "side"})}}}}});
}

// ============================================================================
// Tool Dispatcher
// ============================================================================

json execute(const std::string& tool, const json& params) {
    if (tool == "validate_layout") {
#ifdef MAXMCP_TEST_MODE
        return ToolCommon::test_mode_error();
#else
        return execute_validate_layout(params);
#endif
    }

    if (tool == "get_io_position") {
#ifdef MAXMCP_TEST_MODE
        return ToolCommon::test_mode_error();
#else
        return execute_get_io_position(params);
#endif
    }

    return nullptr;
}

}  // namespace LayoutTools
