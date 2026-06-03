/**
    @file geometry.h
    MaxMCP - Pure 2D geometry predicates for layout validation

    Max-API independent geometry used by the layout validation tools
    (validate_layout). Everything here is a pure function over plain structs so
    it can be unit-tested without the Max SDK. The whole point of issue #76 is to
    move geometric reasoning out of the LLM and into code whose correctness is
    pinned by tests — so this header must stay free of any Max dependency.

    Coordinate system: patching coordinates, origin top-left, +y downward.
    A Rect is [x, y, width, height]; right() = x + width, bottom() = y + height.

    @ingroup maxmcp
*/

#ifndef GEOMETRY_H
#define GEOMETRY_H

#include <vector>

namespace geometry {

/**
 * @brief A 2D point in patch coordinates.
 */
struct Point {
    double x;
    double y;
};

/**
 * @brief Axis-aligned rectangle in patch coordinates.
 *
 * @c origin is the top-left corner (reusing Point); @c width / @c height are the
 * extents. Thanks to aggregate brace elision, a flat @c Rect{x,y,w,h} still
 * initializes @c origin{x,y} with @c width=w, @c height=h.
 */
struct Rect {
    Point origin;
    double width;
    double height;

    double right() const {
        return origin.x + width;
    }
    double bottom() const {
        return origin.y + height;
    }
    // Area of a well-formed rect. For a degenerate rect (negative width/height,
    // e.g. an empty intersection) this is meaningless — two negative extents would
    // multiply to a positive value — so callers that may hold a degenerate rect
    // must check the extents first (see aabb_overlap_area).
    double area() const {
        return width * height;
    }
};

/**
 * @brief A directed line segment from @c a to @c b.
 */
struct Segment {
    Point a;
    Point b;
};

/**
 * @brief Whether two rectangles overlap by more than @p eps on both axes.
 *
 * Unlike a clearance test, @p eps here *shrinks* the overlap zone: rectangles
 * that merely touch, or overlap by no more than @p eps, are NOT reported as
 * overlapping. This tolerance absorbs sub-pixel rounding so adjacent objects
 * placed flush against each other do not produce false positives.
 *
 * @param a   First rectangle
 * @param b   Second rectangle
 * @param eps Penetration tolerance (overlap must exceed this on both axes)
 * @return true if the rectangles overlap by more than @p eps on both axes
 */
bool aabb_overlap(const Rect& a, const Rect& b, double eps);

/**
 * @brief The geometric intersection of two rectangles.
 *
 * The overlap region is itself an axis-aligned rectangle: its origin is the
 * per-axis maximum of the starts and its far corner the per-axis minimum of the
 * ends. When the rectangles do not overlap the result is degenerate — @c width
 * and/or @c height come out zero or negative — so callers should treat a
 * non-positive extent as "no overlap".
 *
 * @param a First rectangle
 * @param b Second rectangle
 * @return The intersection rectangle (degenerate if @p a and @p b are disjoint)
 */
Rect aabb_intersection(const Rect& a, const Rect& b);

/**
 * @brief Area of the geometric intersection of two rectangles, in px^2.
 *
 * Returns 0 when the rectangles do not overlap. This is the raw intersection
 * area (no @p eps tolerance) intended for human-readable finding details.
 *
 * @param a First rectangle
 * @param b Second rectangle
 * @return Overlap area in square pixels (0 if disjoint)
 */
double aabb_overlap_area(const Rect& a, const Rect& b);

/**
 * @brief Whether a segment passes through a rectangle (Liang-Barsky).
 *
 * Reports a crossing only when the segment's interior chord inside @p r is
 * longer than @p eps, so a cord that merely grazes a corner or edge within the
 * tolerance is not flagged. The endpoints are taken as-is; callers that want to
 * ignore the cord's own source/destination objects must exclude them.
 *
 * @param s     The segment to test
 * @param r     The rectangle to test against
 * @param eps   Minimum interior chord length to count as a crossing
 * @param entry If non-null and a crossing is found, set to the first point
 *              inside the rectangle (segment start + u1 * direction)
 * @return true if @p s crosses @p r by more than @p eps
 */
bool segment_intersects_rect(const Segment& s, const Rect& r, double eps, Point* entry);

/**
 * @brief Whether two axis-aligned, collinear segments overlap along their axis.
 *
 * Only vertical-vs-vertical (sharing an x within @p eps) and
 * horizontal-vs-horizontal (sharing a y within @p eps) pairs can overlap; any
 * other combination (including diagonals) returns false. The shared interval
 * must be longer than @p eps to count.
 *
 * @param s1  First segment
 * @param s2  Second segment
 * @param eps Tolerance for axis-alignment, collinearity, and interval length
 * @return true if the two segments are collinear and their overlap exceeds @p eps
 */
bool segments_overlap_collinear(const Segment& s1, const Segment& s2, double eps);

/**
 * @brief Whether a segment runs "upward" on screen (start below its end).
 *
 * In patch coordinates +y points down, so an upward cord has a larger start.y
 * than end.y. Returns true when start.y - end.y exceeds @p eps; near-horizontal
 * and downward segments return false.
 *
 * @param s   The segment to test
 * @param eps Vertical tolerance (near-horizontal segments are not upward)
 * @return true if the segment rises by more than @p eps
 */
bool segment_is_upward(const Segment& s, double eps);

/**
 * @brief Decompose a polyline (start -> mids... -> end) into its segments.
 *
 * @param start First point of the polyline
 * @param mids  Intermediate points in order (may be empty)
 * @param end   Last point of the polyline
 * @return The segment list; size is mids.size() + 1
 */
std::vector<Segment> polyline_to_segments(const Point& start, const std::vector<Point>& mids,
                                          const Point& end);

}  // namespace geometry

#endif  // GEOMETRY_H
