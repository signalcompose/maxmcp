/**
    @file geometry.cpp
    MaxMCP - Pure 2D geometry predicates for layout validation

    Implementation of the Max-API independent geometry declared in geometry.h.
    See that header for the coordinate-system contract and per-function notes.

    @ingroup maxmcp
*/

#include "geometry.h"

#include <algorithm>
#include <cmath>

namespace geometry {

bool aabb_overlap(const Rect& a, const Rect& b, double eps) {
    // Two AABBs overlap iff they overlap on BOTH axes. The classic separating-axis
    // form is "they are apart if separated on ANY axis", so we negate the OR of the
    // four separation tests (a left of b, b left of a, a above b, b above a).
    //
    // Adding eps to the right-hand side widens the "apart" condition: a penetration
    // of eps or less still counts as separated. So eps SHRINKS the reported overlap
    // zone, absorbing sub-pixel rounding for objects placed flush against each other.
    return !(a.right() <= b.origin.x + eps || b.right() <= a.origin.x + eps ||
             a.bottom() <= b.origin.y + eps || b.bottom() <= a.origin.y + eps);
}

Rect aabb_intersection(const Rect& a, const Rect& b) {
    // The intersection of two AABBs is itself an AABB: its origin is the per-axis
    // maximum of the starts and its far corner the per-axis minimum of the ends.
    // When the rects are disjoint the extents come out non-positive.
    const double x0 = std::max(a.origin.x, b.origin.x);
    const double y0 = std::max(a.origin.y, b.origin.y);
    const double x1 = std::min(a.right(), b.right());
    const double y1 = std::min(a.bottom(), b.bottom());
    return {{x0, y0}, x1 - x0, y1 - y0};
}

double aabb_overlap_area(const Rect& a, const Rect& b) {
    // A non-positive extent on either axis means the rectangles do not actually
    // overlap there, so the intersection is empty.
    const Rect r = aabb_intersection(a, b);
    if (r.width <= 0.0 || r.height <= 0.0) {
        return 0.0;
    }
    return r.area();
}

bool segment_intersects_rect(const Segment& s, const Rect& r, double eps, Point* entry) {
    // Parametrize the segment as P(u) = s.a + u * d for u in [0, 1], where d is the
    // direction vector below. Liang-Barsky clips that [0, 1] range against the
    // rect's four edges ("slabs") and reports the surviving sub-range [u1, u2] that
    // lies inside the rect — empty range means no crossing.
    const double dx = s.b.x - s.a.x;
    const double dy = s.b.y - s.a.y;

    // Each rect edge gives one constraint of the form p[k]*u <= q[k]:
    //   p = [-dx, dx, -dy, dy]   (how u moves the point across the left/right/top/bottom edge)
    //   q = [x0-xmin, xmax-x0, y0-ymin, ymax-y0]   (start point's signed distance to that edge)
    // p[k] < 0 means u is ENTERING the rect through edge k (raises the lower bound u1);
    // p[k] > 0 means u is LEAVING through edge k (lowers the upper bound u2).
    const double p[4] = {-dx, dx, -dy, dy};
    const double q[4] = {s.a.x - r.origin.x, r.right() - s.a.x, s.a.y - r.origin.y,
                         r.bottom() - s.a.y};

    double u1 = 0.0;  // largest "entering" parameter so far (start of the inside range)
    double u2 = 1.0;  // smallest "leaving" parameter so far (end of the inside range)
    for (int k = 0; k < 4; ++k) {
        if (p[k] == 0.0) {
            // Segment is parallel to edge k. There is no crossing parameter to
            // compute; the segment can only be inside if its start is already on the
            // inner side (q[k] >= 0). If it starts outside, it never enters.
            if (q[k] < 0.0) {
                return false;
            }
            continue;
        }
        const double t = q[k] / p[k];  // parameter where the segment meets edge k
        if (p[k] < 0.0) {
            u1 = std::max(u1, t);
        } else {
            u2 = std::min(u2, t);
        }
    }

    // If the entering bound overtook the leaving bound, the clipped range is empty.
    if (u1 > u2) {
        return false;
    }

    // [u1, u2] is the portion of the segment inside the rect; its length is the
    // total segment length scaled by (u2 - u1). Require it to exceed eps so a cord
    // that merely grazes a corner/edge within the tolerance is not flagged.
    const double interior = std::hypot(dx, dy) * (u2 - u1);
    if (interior <= eps) {
        return false;
    }

    // entry is the first point inside the rect: the segment start advanced to u1.
    if (entry) {
        entry->x = s.a.x + u1 * dx;
        entry->y = s.a.y + u1 * dy;
    }
    return true;
}

namespace {

// Span of a segment along the x or y axis, returned as [lo, hi].
struct Span {
    double lo;
    double hi;
};

Span x_span(const Segment& s) {
    return {std::min(s.a.x, s.b.x), std::max(s.a.x, s.b.x)};
}

Span y_span(const Segment& s) {
    return {std::min(s.a.y, s.b.y), std::max(s.a.y, s.b.y)};
}

bool is_vertical(const Segment& s, double eps) {
    return std::fabs(s.a.x - s.b.x) <= eps;
}

bool is_horizontal(const Segment& s, double eps) {
    return std::fabs(s.a.y - s.b.y) <= eps;
}

// Whether two 1-D intervals overlap by more than eps. A shared length of eps or
// less is just touching endpoints, not a real overlap.
bool spans_overlap(const Span& a, const Span& b, double eps) {
    return std::min(a.hi, b.hi) - std::max(a.lo, b.lo) > eps;
}

}  // namespace

bool segments_overlap_collinear(const Segment& s1, const Segment& s2, double eps) {
    // Two segments can only "stack" (draw on top of each other) when they share an
    // axis: both vertical on the same column, or both horizontal on the same row.
    // For each case we check collinearity first (the shared coordinate matches),
    // then whether their spans along the other axis overlap. Anything else —
    // including a vertical-vs-horizontal pair or any diagonal — cannot collinearly
    // overlap.
    if (is_vertical(s1, eps) && is_vertical(s2, eps)) {
        const bool same_column = std::fabs(s1.a.x - s2.a.x) <= eps;
        return same_column && spans_overlap(y_span(s1), y_span(s2), eps);
    }

    if (is_horizontal(s1, eps) && is_horizontal(s2, eps)) {
        const bool same_row = std::fabs(s1.a.y - s2.a.y) <= eps;
        return same_row && spans_overlap(x_span(s1), x_span(s2), eps);
    }

    return false;
}

bool segment_is_upward(const Segment& s, double eps) {
    // +y points down in patch coordinates, so a segment that rises on screen ends at
    // a smaller y than it starts: start.y - end.y is positive. Requiring it to exceed
    // eps also excludes near-horizontal segments (where the difference is tiny).
    return (s.a.y - s.b.y) > eps;
}

std::vector<Segment> polyline_to_segments(const Point& start, const std::vector<Point>& mids,
                                          const Point& end) {
    std::vector<Segment> segments;
    segments.reserve(mids.size() + 1);
    Point prev = start;
    for (const Point& m : mids) {
        segments.push_back({prev, m});
        prev = m;
    }
    segments.push_back({prev, end});
    return segments;
}

}  // namespace geometry
