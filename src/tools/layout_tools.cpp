/**
    @file layout_tools.cpp
    MaxMCP - Layout Validation MCP Tools Implementation

    Implements validate_layout: walks a patcher for object rects and patchline
    geometry, then runs the pure checks in run_layout_checks() (which delegate to
    src/utils/geometry.h). Read-only — never mutates the patch.

    @ingroup maxmcp
*/

#include "layout_tools.h"

#include "tool_common.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
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
#include "utils/patch_helpers.h"
#include "utils/patch_registry.h"

namespace LayoutTools {

using ToolCommon::DeferredResult;

// ============================================================================
// Formatting helpers
// ============================================================================

namespace {

// One-decimal coordinate, e.g. "876.8" — matches the precision used in spec
// findings without dumping the full double precision.
std::string fmt1(double v) {
    char buf[32];
    std::snprintf(buf, sizeof(buf), "%.1f", v);
    return buf;
}

// Rounded integer string, for areas and overlap-region bounds.
std::string fmti(double v) {
    return std::to_string(static_cast<long>(std::llround(v)));
}

// A human-readable label for an object: its varname, or maxclass#id when unnamed.
std::string label_for(const LayoutObject& o) {
    if (!o.varname.empty()) {
        return o.varname;
    }
    return o.maxclass + "#" + std::to_string(o.id);
}

// The {src_varname, outlet, dst_varname, inlet} topology block for a cord.
json cord_topology(const LayoutCord& c) {
    return {{"src_varname", c.src_varname},
            {"outlet", c.outlet},
            {"dst_varname", c.dst_varname},
            {"inlet", c.inlet}};
}

// Each append_*_findings helper runs one check, pushes its findings onto
// `findings`, and returns how many it added (for the summary counters). Splitting
// the checks out keeps run_layout_checks a flat orchestrator instead of a deeply
// nested loop nest.

// overlap / presentation_overlap: every overlapping object pair (AABB). The mode
// only changes the label and which counter the caller credits the count to.
int append_overlap_findings(const std::vector<LayoutObject>& objects,
                            const LayoutCheckOptions& options, json& findings) {
    const double eps = options.epsilon;
    const std::string type = options.presentation_mode ? "presentation_overlap" : "overlap";
    const std::string rect_name = options.presentation_mode ? "presentation_rect" : "patching_rect";
    int count = 0;
    for (size_t i = 0; i < objects.size(); ++i) {
        for (size_t j = i + 1; j < objects.size(); ++j) {
            const geometry::Rect& a = objects[i].rect;
            const geometry::Rect& b = objects[j].rect;
            if (!geometry::aabb_overlap(a, b, eps)) {
                continue;
            }
            // aabb_overlap returned true, so the intersection has positive extent.
            const geometry::Rect ov = geometry::aabb_intersection(a, b);
            const double area = ov.area();
            std::string detail = rect_name + " overlap area " + fmti(area) +
                                 " px^2 (x:" + fmti(ov.origin.x) + "-" + fmti(ov.right()) +
                                 " y:" + fmti(ov.origin.y) + "-" + fmti(ov.bottom()) + ")";
            findings.push_back(
                {{"type", type},
                 {"severity", "error"},
                 {"objects", json::array({label_for(objects[i]), label_for(objects[j])})},
                 {"detail", detail}});
            ++count;
        }
    }
    return count;
}

// upward: a cord segment that rises on screen. A straight (no-midpoint) upward
// cord is an error; an upward segment of a folded cord is an intentional detour
// (warning).
int append_upward_findings(const std::vector<LayoutCord>& cords, double eps, json& findings) {
    int count = 0;
    for (const LayoutCord& cord : cords) {
        const std::vector<geometry::Segment> segs = cord.segments();
        const bool has_midpoints = !cord.midpoints.empty();
        for (const geometry::Segment& seg : segs) {
            if (!geometry::segment_is_upward(seg, eps)) {
                continue;
            }
            std::string detail = "start.y " + fmti(seg.a.y) + " > end.y " + fmti(seg.b.y) +
                                 ", num_midpoints " + std::to_string(cord.midpoints.size());
            findings.push_back({{"type", "upward"},
                                {"severity", has_midpoints ? "warning" : "error"},
                                {"cord", cord_topology(cord)},
                                {"detail", detail}});
            ++count;
        }
    }
    return count;
}

// cord_object: a cord segment passing through an unrelated object's rect. The
// cord's own source/destination objects are excluded, and each (cord, object)
// pair is reported at most once.
int append_cord_object_findings(const std::vector<LayoutObject>& objects,
                                const std::vector<LayoutCord>& cords, double eps, json& findings) {
    int count = 0;
    for (const LayoutCord& cord : cords) {
        const std::vector<geometry::Segment> segs = cord.segments();
        std::set<int> already_flagged;
        for (const geometry::Segment& seg : segs) {
            for (const LayoutObject& obj : objects) {
                if (obj.id == cord.src_id || obj.id == cord.dst_id) {
                    continue;  // a cord may legitimately touch its own endpoints
                }
                if (already_flagged.count(obj.id)) {
                    continue;
                }
                geometry::Point entry{0.0, 0.0};
                if (!geometry::segment_intersects_rect(seg, obj.rect, eps, &entry)) {
                    continue;
                }
                already_flagged.insert(obj.id);
                std::string detail = "cord segment passes through object rect; crossing at (" +
                                     fmt1(entry.x) + ", " + fmt1(entry.y) + ")";
                findings.push_back({{"type", "cord_object"},
                                    {"severity", "error"},
                                    {"cord", cord_topology(cord)},
                                    {"object", label_for(obj)},
                                    {"detail", detail}});
                ++count;
            }
        }
    }
    return count;
}

// cord_cord: two cords whose axis-aligned segments collinearly overlap. Each cord
// pair is reported at most once.
int append_cord_cord_findings(const std::vector<LayoutCord>& cords, double eps, json& findings) {
    // Flatten every cord into (cord index, segment) for pairwise testing.
    struct IndexedSegment {
        size_t cord;
        geometry::Segment seg;
    };
    std::vector<IndexedSegment> all_segments;
    for (size_t ci = 0; ci < cords.size(); ++ci) {
        for (const geometry::Segment& seg : cords[ci].segments()) {
            all_segments.push_back({ci, seg});
        }
    }
    std::set<std::pair<size_t, size_t>> flagged_pairs;
    int count = 0;
    for (size_t a = 0; a < all_segments.size(); ++a) {
        for (size_t b = a + 1; b < all_segments.size(); ++b) {
            if (all_segments[a].cord == all_segments[b].cord) {
                continue;  // ignore a cord overlapping itself
            }
            if (!geometry::segments_overlap_collinear(all_segments[a].seg, all_segments[b].seg,
                                                      eps)) {
                continue;
            }
            const size_t ca = std::min(all_segments[a].cord, all_segments[b].cord);
            const size_t cb = std::max(all_segments[a].cord, all_segments[b].cord);
            if (!flagged_pairs.insert({ca, cb}).second) {
                continue;
            }
            findings.push_back(
                {{"type", "cord_cord"},
                 {"severity", "warning"},
                 {"cords", json::array({cord_topology(cords[ca]), cord_topology(cords[cb])})},
                 {"detail", "collinear cord segments overlap"}});
            ++count;
        }
    }
    return count;
}

}  // namespace

// ============================================================================
// Pure check orchestration (Max-API independent)
// ============================================================================

json run_layout_checks(const std::vector<LayoutObject>& objects,
                       const std::vector<LayoutCord>& cords, const LayoutCheckOptions& options) {
    const double eps = options.epsilon;
    json findings = json::array();
    int n_upward = 0, n_overlap = 0, n_cord_object = 0, n_cord_cord = 0, n_presentation = 0;

    // Object overlaps apply in both modes; the mode only decides the label and
    // which summary counter the count lands in.
    if (options.check_overlap) {
        const int overlaps = append_overlap_findings(objects, options, findings);
        (options.presentation_mode ? n_presentation : n_overlap) += overlaps;
    }

    // Cords are not drawn in presentation mode, so cord-based checks run only for
    // the patching layout.
    if (!options.presentation_mode) {
        if (options.check_upward) {
            n_upward += append_upward_findings(cords, eps, findings);
        }
        if (options.check_cord_object) {
            n_cord_object += append_cord_object_findings(objects, cords, eps, findings);
        }
        if (options.check_cord_cord) {
            n_cord_cord += append_cord_cord_findings(cords, eps, findings);
        }
    }

    return {{"clean", findings.empty()},
            {"summary",
             {{"upward", n_upward},
              {"overlap", n_overlap},
              {"cord_object", n_cord_object},
              {"cord_cord", n_cord_cord},
              {"presentation_overlap", n_presentation}}},
            {"findings", findings}};
}

// ============================================================================
// Data Structures for Deferred Callbacks
// ============================================================================

struct t_validate_layout_data {
    t_maxmcp* patch;
    LayoutCheckOptions options;
    std::vector<std::string> scope_varnames;
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
            {"required", json::array({"patch_id"})}}}}});
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

    return nullptr;
}

}  // namespace LayoutTools
