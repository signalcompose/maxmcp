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

#include <algorithm>
#include <cmath>

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
// Geometry: nearest non-overlapping placement (Max-API independent, testable)
// ============================================================================

// Two rectangles conflict if they are closer than `gap` on both axes.
// This is the shared AABB separating-axis test with a negated tolerance: a
// positive clearance `gap` (keep objects apart) is the mirror image of
// geometry::aabb_overlap's positive penetration tolerance (ignore tiny overlaps),
// so the single formula lives in geometry and this stays a thin, intent-named
// wrapper. Public so the same predicate can be reused by unit tests.
bool rects_conflict(const Rect& a, const Rect& b, double gap) {
    return geometry::aabb_overlap(a, b, -gap);
}

namespace {

constexpr double PLACE_MARGIN = 50.0;        // gap used when stacking to the right
constexpr double PLACE_GAP = 8.0;            // minimum visual gap between objects
constexpr double PLACE_GRID = 15.0;          // search step (matches Max default grid)
constexpr double PLACE_MAX_RADIUS = 4000.0;  // give up beyond this many px

// Reuse the shared geometry point so the placement search and the layout tools
// speak the same coordinate type (see src/utils/geometry.h).
using Point = geometry::Point;

// A width x height rect at (x, y) is valid when it stays in the visible area
// and clears every existing rect (keeping PLACE_GAP of separation).
bool position_is_free(double x, double y, double width, double height,
                      const std::vector<Rect>& existing) {
    if (x < 0.0 || y < 0.0) {
        return false;
    }
    const Rect candidate{x, y, width, height};
    for (const Rect& r : existing) {
        if (rects_conflict(candidate, r, PLACE_GAP)) {
            return false;
        }
    }
    return true;
}

double rightmost_edge(const std::vector<Rect>& existing) {
    double edge = 0.0;
    for (const Rect& r : existing) {
        edge = std::max(edge, r.origin.x + r.width);
    }
    return edge;
}

std::string coord_pair(double x, double y) {
    return "(" + std::to_string(static_cast<int>(x)) + ", " + std::to_string(static_cast<int>(y)) +
           ")";
}

// Where to start searching from, plus a human-readable description.
Point search_anchor(const std::vector<Rect>& existing, bool has_near, double near_x, double near_y,
                    std::string& desc) {
    if (has_near) {
        desc = "near " + coord_pair(near_x, near_y);
        return {std::max(0.0, near_x), std::max(0.0, near_y)};
    }
    if (existing.empty()) {
        desc = "default origin (empty patch)";
        return {50.0, 50.0};
    }
    desc = "to the right of existing objects";
    return {rightmost_edge(existing) + PLACE_MARGIN, 50.0};
}

// Nearest (Euclidean) free spot on the square ring at Chebyshev distance
// `radius` from the anchor. Returns false if the ring has no free spot.
bool nearest_free_on_ring(const Point& anchor, double radius, double width, double height,
                          const std::vector<Rect>& existing, Point& out) {
    bool found = false;
    double best_dist2 = 0.0;
    for (double dx = -radius; dx <= radius; dx += PLACE_GRID) {
        for (double dy = -radius; dy <= radius; dy += PLACE_GRID) {
            const bool on_perimeter = std::max(std::fabs(dx), std::fabs(dy)) >= radius;
            if (!on_perimeter ||
                !position_is_free(anchor.x + dx, anchor.y + dy, width, height, existing)) {
                continue;
            }
            const double dist2 = dx * dx + dy * dy;
            if (!found || dist2 < best_dist2) {
                found = true;
                best_dist2 = dist2;
                out = {anchor.x + dx, anchor.y + dy};
            }
        }
    }
    return found;
}

}  // namespace

PlacedPosition find_avoid_rect_position(const std::vector<Rect>& existing, double width,
                                        double height, bool has_near, double near_x,
                                        double near_y) {
    std::string desc;
    const Point anchor = search_anchor(existing, has_near, near_x, near_y, desc);

    if (position_is_free(anchor.x, anchor.y, width, height, existing)) {
        return {anchor, "Placed " + desc};
    }

    Point spot{};
    for (double radius = PLACE_GRID; radius <= PLACE_MAX_RADIUS; radius += PLACE_GRID) {
        if (nearest_free_on_ring(anchor, radius, width, height, existing, spot)) {
            return {spot, "Found nearest free position " + desc};
        }
    }

    // Fallback for an extremely dense patch: the far right is always free.
    return {{rightmost_edge(existing) + PLACE_MARGIN, 50.0}, "Fallback: placed to the far right"};
}

// ============================================================================
// Data Structures for Deferred Callbacks
// ============================================================================

struct t_get_position_data {
    t_maxmcp* patch;
    double width;
    double height;
    bool has_near;
    double near_x;
    double near_y;
    DeferredResult* deferred_result;
};

// ============================================================================
// Deferred Callbacks
// ============================================================================

#ifndef MAXMCP_TEST_MODE

/**
 * @brief Deferred callback for finding an empty position
 *
 * Executed on main thread via defer(). Collects the patching rectangles of all
 * existing objects, then delegates the placement decision to the Max-API
 * independent find_avoid_rect_position() so the geometry stays unit-testable.
 */
static void get_position_deferred(t_maxmcp* patch, t_symbol* s, long argc, t_atom* argv) {
    VALIDATE_DEFERRED_ARGS("get_position_deferred");
    EXTRACT_DEFERRED_DATA_WITH_RESULT(t_get_position_data, data, argv);

    t_object* patcher = data->patch->patcher;

    // Collect the patching rectangle of every existing object.
    std::vector<Rect> existing;
    for (t_object* box = jpatcher_get_firstobject(patcher); box; box = jbox_get_nextobject(box)) {
        t_rect rect;
        jbox_get_patching_rect(box, &rect);
        existing.push_back({rect.x, rect.y, rect.width, rect.height});
    }

    PlacedPosition placed = find_avoid_rect_position(existing, data->width, data->height,
                                                     data->has_near, data->near_x, data->near_y);

    std::string rationale = placed.rationale;
    if (data->width != 50.0 || data->height != 20.0) {
        rationale += " (for object " + std::to_string(static_cast<int>(data->width)) + "x" +
                     std::to_string(static_cast<int>(data->height)) + ")";
    }

    COMPLETE_DEFERRED(data, json({{"position", json::array({placed.position.x, placed.position.y})},
                                  {"width", data->width},
                                  {"height", data->height},
                                  {"rationale", rationale}}));
}

#endif  // MAXMCP_TEST_MODE

// ============================================================================
// Tool Schema Definitions
// ============================================================================

json get_tool_schemas() {
    return json::array(
        {// get_console_log
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
          {"description", "Find an empty (non-overlapping) position for a new object. With "
                          "near_x/near_y, returns the nearest free spot to that point; "
                          "otherwise places to the right of existing objects."},
          {"inputSchema",
           {{"type", "object"},
            {"properties",
             {{"patch_id", {{"type", "string"}, {"description", "Patch ID to query"}}},
              {"width", {{"type", "number"}, {"description", "Object width (default: 50)"}}},
              {"height", {{"type", "number"}, {"description", "Object height (default: 20)"}}},
              {"near_x",
               {{"type", "number"},
                {"description", "Optional target x. Provide with near_y to search near a point."}}},
              {"near_y",
               {{"type", "number"},
                {"description",
                 "Optional target y. Provide with near_x to search near a point."}}}}},
            {"required", json::array({"patch_id"})}}}}});
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

#ifndef MAXMCP_TEST_MODE

/**
 * @brief Execute get_avoid_rect_position tool
 */
static json execute_get_avoid_rect_position(const json& params) {
    std::string patch_id = params.value("patch_id", "");
    double width = params.value("width", 50.0);
    double height = params.value("height", 20.0);

    if (patch_id.empty()) {
        return ToolCommon::missing_param_error("patch_id");
    }

    // near_x/near_y are optional: when both are present, the search anchors at
    // that point; otherwise it falls back to placing to the right.
    bool has_near = params.contains("near_x") && params.contains("near_y");
    double near_x = params.value("near_x", 0.0);
    double near_y = params.value("near_y", 0.0);

    t_maxmcp* patch = PatchRegistry::find_patch(patch_id);
    if (!patch) {
        return ToolCommon::patch_not_found_error(patch_id);
    }

    auto* deferred_result = new DeferredResult();
    auto* data =
        new t_get_position_data{patch, width, height, has_near, near_x, near_y, deferred_result};

    return ToolCommon::run_deferred(patch, (method)get_position_deferred, "get_position", data,
                                    ToolCommon::DEFAULT_DEFER_TIMEOUT,
                                    "waiting for position calculation",
                                    ToolCommon::DeferredWrap::Always);
}

#endif  // MAXMCP_TEST_MODE

// ============================================================================
// Public Dispatcher
// ============================================================================

json execute(const std::string& tool, const json& params) {
    if (tool == "get_console_log") {
        return execute_get_console_log(params);
    } else if (tool == "get_avoid_rect_position") {
#ifdef MAXMCP_TEST_MODE
        return ToolCommon::test_mode_error();
#else
        return execute_get_avoid_rect_position(params);
#endif
    }

    return nullptr;
}

}  // namespace UtilityTools
