/**
    @file layout_tools.h
    MaxMCP - Layout Validation MCP Tools

    Tools for validating patch layout geometry:
    - validate_layout: machine-check Phase 8 of organize-patch (overlaps, upward
      cords, cord-vs-object crossings, collinear cord overlaps) and return a
      structured findings list. Read-only — never modifies the patch.

    The geometric predicates live in src/utils/geometry.h (Max-API independent,
    unit-tested). The finding orchestration in run_layout_checks() is likewise
    Max-independent so it can be tested without the SDK; only the data extraction
    (walking the patcher for rects and patchlines) needs the Max main thread.

    @ingroup maxmcp
*/

#ifndef LAYOUT_TOOLS_H
#define LAYOUT_TOOLS_H

#include "utils/geometry.h"

#include <string>
#include <vector>

#include <nlohmann/json.hpp>

// Forward declarations
struct _maxmcp;
typedef struct _maxmcp t_maxmcp;

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

// ============================================================================
// MCP tool interface
// ============================================================================

/**
 * @brief Get the JSON schemas for all layout tools.
 *
 * @return JSON array of tool schemas (validate_layout)
 */
json get_tool_schemas();

/**
 * @brief Execute a layout tool.
 *
 * @param tool The tool name to execute
 * @param params The tool parameters as JSON
 * @return JSON result, or null if the tool is not handled by this module
 */
json execute(const std::string& tool, const json& params);

}  // namespace LayoutTools

#endif  // LAYOUT_TOOLS_H
