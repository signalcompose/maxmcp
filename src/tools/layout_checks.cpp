/**
    @file layout_checks.cpp
    MaxMCP - Pure layout-check logic implementation

    Implements run_layout_checks() and its per-check helpers over already-extracted
    LayoutObject / LayoutCord structs. Max-API independent: the geometric decisions
    delegate to src/utils/geometry.h, and the SDK glue that walks the patcher lives
    in layout_tools.cpp.

    @ingroup maxmcp
*/

#include "layout_checks.h"

#include "utils/format_util.h"
#include "utils/geometry.h"

#include <algorithm>
#include <set>
#include <string>
#include <utility>
#include <vector>

namespace LayoutTools {

using fmtutil::fmt1;
using fmtutil::fmti;

// ============================================================================
// Formatting helpers
// ============================================================================

namespace {

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

}  // namespace LayoutTools
