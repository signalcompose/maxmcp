/**
    @file layout_checks.h
    MaxMCP - Pure layout-check logic (Max-API independent)

    The data structures and finding orchestration behind validate_layout, with no
    Max SDK dependency so they can be unit-tested directly. A patcher is reduced to
    plain LayoutObject / LayoutCord structs (by the SDK glue in layout_tools.cpp),
    then run_layout_checks() applies the geometric predicates from
    src/utils/geometry.h and returns the structured findings list.

    @ingroup maxmcp
*/

#ifndef LAYOUT_CHECKS_H
#define LAYOUT_CHECKS_H

#include "utils/geometry.h"

#include <string>
#include <vector>

#include <nlohmann/json.hpp>

namespace LayoutTools {

using json = nlohmann::json;

// ============================================================================
// Plain data extracted from a patcher (Max-API independent, testable)
// ============================================================================

/**
 * @brief One object's geometry, as needed by the layout checks.
 *
 * @c id is the object's position in patcher order; it identifies cord endpoints
 * unambiguously even when objects share (or lack) a varname. @c rect is the rect
 * for the active mode (patching or presentation).
 */
struct LayoutObject {
    int id;
    std::string varname;
    std::string maxclass;
    geometry::Rect rect;
    bool in_presentation;
};

/**
 * @brief One patchcord's topology and polyline geometry.
 *
 * @c src_id / @c dst_id reference LayoutObject::id so cord-vs-object checks can
 * skip the cord's own endpoints reliably.
 */
struct LayoutCord {
    int src_id;
    std::string src_varname;
    long outlet;
    int dst_id;
    std::string dst_varname;
    long inlet;
    geometry::Point start;
    geometry::Point end;
    std::vector<geometry::Point> midpoints;

    // The cord's polyline (start -> midpoints... -> end) decomposed into segments.
    std::vector<geometry::Segment> segments() const {
        return geometry::polyline_to_segments(start, midpoints, end);
    }
};

/**
 * @brief Which checks to run and with what tolerance.
 */
struct LayoutCheckOptions {
    double epsilon = 2.0;
    bool check_upward = true;
    bool check_overlap = true;
    bool check_cord_object = true;
    bool check_cord_cord = true;
    // When true, overlap findings are reported as "presentation_overlap" and the
    // cord-based checks are skipped (cords are not shown in presentation mode).
    bool presentation_mode = false;
};

// ============================================================================
// Pure check orchestration (Max-API independent, unit-tested)
// ============================================================================

/**
 * @brief Run the enabled layout checks over already-extracted geometry.
 *
 * Pure function: takes plain structs (no Max SDK) and returns the validate_layout
 * response body { clean, summary, findings }. The geometric decisions are
 * delegated to src/utils/geometry.h.
 *
 * @param objects Objects participating in the checks (rects for the active mode)
 * @param cords   Patchcords participating in the checks
 * @param options Enabled checks and tolerance
 * @return JSON object: { "clean": bool, "summary": {...}, "findings": [...] }
 */
json run_layout_checks(const std::vector<LayoutObject>& objects,
                       const std::vector<LayoutCord>& cords, const LayoutCheckOptions& options);

}  // namespace LayoutTools

#endif  // LAYOUT_CHECKS_H
