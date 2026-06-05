/**
    @file geometry.cpp
    MaxMCP - Pure 2D geometry predicates for layout validation

    Implementation of the Max-API independent geometry declared in geometry.h.
    See that header for the coordinate-system contract and per-function notes.

    @ingroup maxmcp
*/

#include "geometry.h"

#include "format_util.h"

#include <algorithm>
#include <cmath>
#include <string>

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

namespace {
// Moves smaller than this are sub-pixel no-ops (the object is already on target).
constexpr double kMoveEps = 0.01;

// A computed placement: the new rects (caller order) plus a human-readable detail
// describing the target line/gap, for the rationale string.
struct Solution {
    std::vector<Rect> rects;
    std::string detail;
};

// Single source of truth for the AlignMode⇄string mapping, in enum order. Both
// directions (mode_name, parse_align_mode) and the accepted-name list derive from
// this, so adding a mode means touching exactly one table.
struct ModeName {
    AlignMode mode;
    const char* name;
};
constexpr ModeName kModeNames[] = {
    {AlignMode::Left, "align_left"},
    {AlignMode::Right, "align_right"},
    {AlignMode::Top, "align_top"},
    {AlignMode::Bottom, "align_bottom"},
    {AlignMode::HCenter, "align_hcenter"},
    {AlignMode::VCenter, "align_vcenter"},
    {AlignMode::DistributeH, "distribute_h"},
    {AlignMode::DistributeV, "distribute_v"},
};

std::string mode_name(AlignMode m) {
    for (const ModeName& e : kModeNames) {
        if (e.mode == m) {
            return e.name;
        }
    }
    return "align";
}

bool is_distribute(AlignMode m) {
    return m == AlignMode::DistributeH || m == AlignMode::DistributeV;
}

// The smallest rect enclosing every input rect.
Rect bounding_box(const std::vector<Rect>& rects) {
    double min_left = rects[0].origin.x, max_right = rects[0].right();
    double min_top = rects[0].origin.y, max_bottom = rects[0].bottom();
    for (const Rect& r : rects) {
        min_left = std::min(min_left, r.origin.x);
        max_right = std::max(max_right, r.right());
        min_top = std::min(min_top, r.origin.y);
        max_bottom = std::max(max_bottom, r.bottom());
    }
    return {min_left, min_top, max_right - min_left, max_bottom - min_top};
}

// Edge and center modes. Each reduces to a single formula: move one axis of the
// origin so a chosen reference point of the object — its near edge (factor 0),
// far edge (factor 1), or center (factor 0.5) — lands on a group-wide target
// line taken from the bounding box.
Solution solve_edge_center(const std::vector<Rect>& rects, AlignMode mode) {
    const Rect bb = bounding_box(rects);
    const bool x_axis =
        (mode == AlignMode::Left || mode == AlignMode::Right || mode == AlignMode::HCenter);

    double target = 0.0;
    double factor = 0.0;
    const char* label = "";
    switch (mode) {
    case AlignMode::Left:
        target = bb.origin.x;
        factor = 0.0;
        label = "left x";
        break;
    case AlignMode::Right:
        target = bb.right();
        factor = 1.0;
        label = "right x";
        break;
    case AlignMode::Top:
        target = bb.origin.y;
        factor = 0.0;
        label = "top y";
        break;
    case AlignMode::Bottom:
        target = bb.bottom();
        factor = 1.0;
        label = "bottom y";
        break;
    case AlignMode::HCenter:
        target = bb.origin.x + bb.width / 2;
        factor = 0.5;
        label = "center x";
        break;
    case AlignMode::VCenter:
        target = bb.origin.y + bb.height / 2;
        factor = 0.5;
        label = "center y";
        break;
    default:
        break;
    }

    Solution sol{rects, std::string(label) + "=" + fmtutil::fmt1(target)};
    for (Rect& r : sol.rects) {
        if (x_axis) {
            r.origin.x = target - factor * r.width;
        } else {
            r.origin.y = target - factor * r.height;
        }
    }
    return sol;
}

// Distribute modes. Sort along the axis, pin the two extreme objects, and lay the
// interior ones out so every adjacent gap is equal.
Solution solve_distribute(const std::vector<Rect>& rects, AlignMode mode) {
    const bool horizontal = (mode == AlignMode::DistributeH);
    const int n = static_cast<int>(rects.size());

    // Axis selectors keep the placement logic free of per-axis branching.
    auto lead = [&](const Rect& r) { return horizontal ? r.origin.x : r.origin.y; };
    auto size = [&](const Rect& r) { return horizontal ? r.width : r.height; };
    auto far = [&](const Rect& r) { return horizontal ? r.right() : r.bottom(); };

    std::vector<int> order(n);
    for (int i = 0; i < n; ++i) {
        order[i] = i;
    }
    std::sort(order.begin(), order.end(),
              [&](int a, int b) { return lead(rects[a]) < lead(rects[b]); });

    double extent = 0.0;  // total object size along the axis
    for (const Rect& r : rects) {
        extent += size(r);
    }
    const double span = far(rects[order.back()]) - lead(rects[order.front()]);
    const double gap = (span - extent) / (n - 1);

    Solution sol{rects, "equal gap=" + fmtutil::fmt1(gap)};
    double cursor = lead(rects[order.front()]);
    for (int idx : order) {
        Rect& r = sol.rects[idx];
        if (horizontal) {
            r.origin.x = cursor;
        } else {
            r.origin.y = cursor;
        }
        cursor += size(r) + gap;
    }
    return sol;
}
}  // namespace

AlignResult recommend_alignment(const std::vector<Rect>& rects, AlignMode mode) {
    const int n = static_cast<int>(rects.size());
    if (n < 2) {
        return {false, {}, "alignment needs at least 2 objects (got " + std::to_string(n) + ")"};
    }
    if (is_distribute(mode) && n < 3) {
        return {false,
                {},
                mode_name(mode) + " needs at least 3 objects (got " + std::to_string(n) + ")"};
    }

    const Solution sol =
        is_distribute(mode) ? solve_distribute(rects, mode) : solve_edge_center(rects, mode);

    // Report only the objects that actually move (already-on-target objects are
    // omitted), so an already-aligned group yields ok=true with no moves.
    AlignResult result;
    result.ok = true;
    for (int i = 0; i < n; ++i) {
        if (std::abs(sol.rects[i].origin.x - rects[i].origin.x) > kMoveEps ||
            std::abs(sol.rects[i].origin.y - rects[i].origin.y) > kMoveEps) {
            result.moves.push_back({i, sol.rects[i]});
        }
    }

    result.reason =
        mode_name(mode) + ": " + (result.moves.empty() ? "already aligned" : sol.detail) + " (" +
        std::to_string(result.moves.size()) + " of " + std::to_string(n) + " objects move)";
    return result;
}

bool parse_align_mode(const std::string& name, AlignMode& out) {
    for (const ModeName& e : kModeNames) {
        if (name == e.name) {
            out = e.mode;
            return true;
        }
    }
    return false;
}

std::string align_mode_name_list() {
    std::string list;
    for (const ModeName& e : kModeNames) {
        if (!list.empty()) {
            list += ", ";
        }
        list += e.name;
    }
    return list;
}

}  // namespace geometry
