/**
    @file utility_tools.h
    MaxMCP - Utility MCP Tools

    Tools for utility operations:
    - get_console_log
    - get_avoid_rect_position

    @ingroup maxmcp
*/

#ifndef UTILITY_TOOLS_H
#define UTILITY_TOOLS_H

#include "utils/geometry.h"

#include <string>
#include <vector>

#include <nlohmann/json.hpp>

// Forward declarations
struct _maxmcp;
typedef struct _maxmcp t_maxmcp;

namespace UtilityTools {

using json = nlohmann::json;

// ============================================================================
// Geometry helpers for get_avoid_rect_position (Max-API independent, testable)
// ============================================================================

/**
 * @brief Axis-aligned rectangle in patch coordinates.
 *
 * Aliased to geometry::Rect so the placement search and the layout validation
 * tools share a single rectangle type (see src/utils/geometry.h).
 */
using Rect = geometry::Rect;

/**
 * @brief Result of a placement search.
 */
struct PlacedPosition {
    geometry::Point position;
    std::string rationale;
};

/**
 * @brief Whether two rectangles are closer than @p gap on both axes.
 *
 * With @p gap == 0 this is a plain (strict) axis-aligned overlap test: rectangles
 * that merely touch edges do not conflict. A positive @p gap additionally
 * enforces that much clearance — rectangles separated by exactly @p gap do not
 * conflict, while anything closer does.
 *
 * @param a   First rectangle
 * @param b   Second rectangle
 * @param gap Required clearance (0 for a pure overlap test)
 * @return true if the rectangles overlap or sit within @p gap on both axes
 */
bool rects_conflict(const Rect& a, const Rect& b, double gap);

/**
 * @brief Find a non-overlapping position for a new object of the given size.
 *
 * Pure geometry, free of any Max API dependency so it can be unit-tested.
 *
 * - When @p has_near is true, searches outward from (@p near_x, @p near_y) on a
 *   grid for the nearest spot where a @p width x @p height rectangle does not
 *   overlap (within a small gap) any rectangle in @p existing.
 * - When @p has_near is false, the anchor defaults to the right of all existing
 *   objects (legacy behavior) — or the origin when the patch is empty — and the
 *   same non-overlap search is applied from there.
 *
 * @param existing Existing object rectangles to avoid
 * @param width    Width of the rectangle to place
 * @param height   Height of the rectangle to place
 * @param has_near Whether a target anchor point was supplied
 * @param near_x   Target anchor x (used only when @p has_near)
 * @param near_y   Target anchor y (used only when @p has_near)
 * @return The chosen position and a human-readable rationale
 */
PlacedPosition find_avoid_rect_position(const std::vector<Rect>& existing, double width,
                                        double height, bool has_near, double near_x, double near_y);

/**
 * @brief Get the JSON schemas for all utility tools
 *
 * Returns an array of tool definitions for:
 * - get_console_log: Retrieve recent Max Console messages
 * - get_avoid_rect_position: Find empty position for placing new objects
 *
 * @return JSON array of tool schemas
 */
json get_tool_schemas();

/**
 * @brief Execute a utility tool
 *
 * Dispatcher for utility tools. Routes the tool name to the
 * appropriate handler function.
 *
 * @param tool The tool name to execute
 * @param params The tool parameters as JSON
 * @return JSON result from tool execution, or error if tool not found
 */
json execute(const std::string& tool, const json& params);

}  // namespace UtilityTools

#endif  // UTILITY_TOOLS_H
