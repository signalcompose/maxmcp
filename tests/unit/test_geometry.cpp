/**
    @file test_geometry.cpp
    Unit tests for the pure geometry predicates (src/utils/geometry.cpp).

    These pin the geometric reasoning that validate_layout relies on: AABB
    overlap with tolerance, segment-vs-rect crossing (Liang-Barsky), collinear
    segment overlap, the upward-cord test, and polyline decomposition. The whole
    value of issue #76 is that this logic is verified here rather than inferred
    by the LLM, so the boundary cases (eps, grazing, touching) are covered
    explicitly.
*/

#include "utils/geometry.h"

#include <cmath>
#include <vector>

#include <gtest/gtest.h>

using geometry::aabb_intersection;
using geometry::aabb_overlap;
using geometry::aabb_overlap_area;
using geometry::Point;
using geometry::polyline_to_segments;
using geometry::Rect;
using geometry::Segment;
using geometry::segment_intersects_rect;
using geometry::segment_is_upward;
using geometry::segments_overlap_collinear;

namespace {
constexpr double kEps = 2.0;
}

// ---------------------------------------------------------------------------
// Rect helpers
// ---------------------------------------------------------------------------

TEST(GeometryRect, RightAndBottom) {
    Rect r{10.0, 20.0, 100.0, 40.0};
    EXPECT_DOUBLE_EQ(r.right(), 110.0);
    EXPECT_DOUBLE_EQ(r.bottom(), 60.0);
}

// ---------------------------------------------------------------------------
// aabb_overlap
// ---------------------------------------------------------------------------

TEST(AabbOverlap, ClearlyOverlapping) {
    Rect a{0.0, 0.0, 50.0, 50.0};
    Rect b{25.0, 25.0, 50.0, 50.0};
    EXPECT_TRUE(aabb_overlap(a, b, kEps));
    EXPECT_TRUE(aabb_overlap(b, a, kEps));  // symmetric
}

TEST(AabbOverlap, Disjoint) {
    Rect a{0.0, 0.0, 50.0, 20.0};
    Rect b{100.0, 100.0, 50.0, 20.0};
    EXPECT_FALSE(aabb_overlap(a, b, kEps));
}

TEST(AabbOverlap, EdgeTouchingIsNotOverlap) {
    Rect a{0.0, 0.0, 50.0, 20.0};
    Rect b{50.0, 0.0, 50.0, 20.0};  // shares the x=50 edge
    EXPECT_FALSE(aabb_overlap(a, b, 0.0));
    EXPECT_FALSE(aabb_overlap(a, b, kEps));
}

TEST(AabbOverlap, EpsToleranceAbsorbsTinyPenetration) {
    Rect a{0.0, 0.0, 50.0, 50.0};
    // b penetrates a by exactly 2px on x and well within on y.
    Rect b{48.0, 0.0, 50.0, 50.0};
    EXPECT_FALSE(aabb_overlap(a, b, kEps)) << "2px overlap is within eps -> not flagged";
    // 3px penetration exceeds eps -> overlap.
    Rect c{47.0, 0.0, 50.0, 50.0};
    EXPECT_TRUE(aabb_overlap(a, c, kEps));
}

TEST(AabbOverlap, OverlapOnOneAxisOnlyIsNotOverlap) {
    Rect a{0.0, 0.0, 50.0, 50.0};
    Rect b{25.0, 200.0, 50.0, 50.0};  // x ranges overlap, y far apart
    EXPECT_FALSE(aabb_overlap(a, b, kEps));
}

// ---------------------------------------------------------------------------
// aabb_overlap_area
// ---------------------------------------------------------------------------

TEST(AabbOverlapArea, ComputesIntersection) {
    Rect a{0.0, 0.0, 50.0, 50.0};
    Rect b{40.0, 40.0, 50.0, 50.0};  // 10x10 intersection
    EXPECT_DOUBLE_EQ(aabb_overlap_area(a, b), 100.0);
}

TEST(AabbOverlapArea, ZeroWhenDisjoint) {
    Rect a{0.0, 0.0, 50.0, 50.0};
    Rect b{100.0, 100.0, 10.0, 10.0};
    EXPECT_DOUBLE_EQ(aabb_overlap_area(a, b), 0.0);
}

TEST(AabbOverlapArea, ZeroWhenEdgeTouching) {
    Rect a{0.0, 0.0, 50.0, 50.0};
    Rect b{50.0, 0.0, 50.0, 50.0};
    EXPECT_DOUBLE_EQ(aabb_overlap_area(a, b), 0.0);
}

// ---------------------------------------------------------------------------
// aabb_intersection
// ---------------------------------------------------------------------------

TEST(AabbIntersection, ReturnsOverlapRect) {
    Rect a{0.0, 0.0, 50.0, 50.0};
    Rect b{40.0, 30.0, 50.0, 50.0};  // overlap x:40-50 y:30-50
    Rect r = aabb_intersection(a, b);
    EXPECT_DOUBLE_EQ(r.origin.x, 40.0);
    EXPECT_DOUBLE_EQ(r.origin.y, 30.0);
    EXPECT_DOUBLE_EQ(r.right(), 50.0);
    EXPECT_DOUBLE_EQ(r.bottom(), 50.0);
    EXPECT_DOUBLE_EQ(r.width, 10.0);
    EXPECT_DOUBLE_EQ(r.height, 20.0);
}

TEST(AabbIntersection, DegenerateWhenDisjoint) {
    Rect a{0.0, 0.0, 50.0, 50.0};
    Rect b{100.0, 0.0, 50.0, 50.0};  // far apart in x
    Rect r = aabb_intersection(a, b);
    // A non-positive extent signals "no overlap".
    EXPECT_LE(r.width, 0.0);
}

// ---------------------------------------------------------------------------
// segment_intersects_rect (Liang-Barsky)
// ---------------------------------------------------------------------------

TEST(SegmentIntersectsRect, HorizontalThroughMiddle) {
    Rect r{100.0, 100.0, 100.0, 40.0};  // x:100-200 y:100-140
    Segment s{{50.0, 120.0}, {300.0, 120.0}};
    Point entry{-1.0, -1.0};
    EXPECT_TRUE(segment_intersects_rect(s, r, kEps, &entry));
    EXPECT_DOUBLE_EQ(entry.x, 100.0);  // enters at the left edge
    EXPECT_DOUBLE_EQ(entry.y, 120.0);
}

TEST(SegmentIntersectsRect, MissesRect) {
    Rect r{100.0, 100.0, 100.0, 40.0};
    Segment s{{50.0, 300.0}, {300.0, 300.0}};  // well below
    EXPECT_FALSE(segment_intersects_rect(s, r, kEps, nullptr));
}

TEST(SegmentIntersectsRect, EndpointInsideCounts) {
    Rect r{100.0, 100.0, 100.0, 40.0};
    Segment s{{150.0, 120.0}, {400.0, 120.0}};  // starts inside, exits right
    EXPECT_TRUE(segment_intersects_rect(s, r, kEps, nullptr));
}

TEST(SegmentIntersectsRect, GrazingCornerWithinEpsIsIgnored) {
    Rect r{100.0, 100.0, 100.0, 40.0};
    // A near-vertical line clipping the top-left corner with a tiny interior
    // chord (< eps) must not be flagged.
    Segment s{{101.0, 90.0}, {101.0, 101.0}};  // interior chord ~1px
    EXPECT_FALSE(segment_intersects_rect(s, r, kEps, nullptr));
}

TEST(SegmentIntersectsRect, ParallelOutsideIsRejected) {
    Rect r{100.0, 100.0, 100.0, 40.0};
    Segment s{{0.0, 50.0}, {300.0, 50.0}};  // horizontal, above the rect
    EXPECT_FALSE(segment_intersects_rect(s, r, kEps, nullptr));
}

// Mirrors the real ParameterMapper case from the spec (appendix): a cord from
// target_id_prefix out0 toward set_obj in1 must clear the prop_route rect
// (x:520-869, y:860-882). At in1 x = 843.5 the cord still pierces prop_route;
// at in1 x = 993.5 it passes to the right and clears it.
TEST(SegmentIntersectsRect, ParameterMapperCordVsPropRoute) {
    Rect prop_route{520.0, 860.0, 349.0, 22.0};  // right = 869
    Segment piercing{{839.5, 700.0}, {843.5, 870.0}};
    EXPECT_TRUE(segment_intersects_rect(piercing, prop_route, kEps, nullptr));

    Segment clearing{{839.5, 700.0}, {993.5, 870.0}};  // x at y=860 ~= 876.8 > 869
    EXPECT_FALSE(segment_intersects_rect(clearing, prop_route, kEps, nullptr));
}

// ---------------------------------------------------------------------------
// segments_overlap_collinear
// ---------------------------------------------------------------------------

TEST(SegmentsOverlapCollinear, VerticalSameColumnOverlap) {
    Segment s1{{100.0, 0.0}, {100.0, 100.0}};
    Segment s2{{100.0, 50.0}, {100.0, 200.0}};  // overlap y:50-100
    EXPECT_TRUE(segments_overlap_collinear(s1, s2, kEps));
}

TEST(SegmentsOverlapCollinear, VerticalDifferentColumns) {
    Segment s1{{100.0, 0.0}, {100.0, 100.0}};
    Segment s2{{120.0, 50.0}, {120.0, 200.0}};  // 20px apart in x
    EXPECT_FALSE(segments_overlap_collinear(s1, s2, kEps));
}

TEST(SegmentsOverlapCollinear, HorizontalSameRowOverlap) {
    Segment s1{{0.0, 50.0}, {100.0, 50.0}};
    Segment s2{{80.0, 50.0}, {200.0, 50.0}};  // overlap x:80-100
    EXPECT_TRUE(segments_overlap_collinear(s1, s2, kEps));
}

TEST(SegmentsOverlapCollinear, VerticalAndHorizontalNeverOverlap) {
    Segment s1{{100.0, 0.0}, {100.0, 100.0}};
    Segment s2{{0.0, 50.0}, {200.0, 50.0}};
    EXPECT_FALSE(segments_overlap_collinear(s1, s2, kEps));
}

TEST(SegmentsOverlapCollinear, CollinearButTouchingWithinEpsIsNotOverlap) {
    Segment s1{{100.0, 0.0}, {100.0, 100.0}};
    Segment s2{{100.0, 101.0}, {100.0, 200.0}};  // 1px shared interval after eps
    EXPECT_FALSE(segments_overlap_collinear(s1, s2, kEps));
}

TEST(SegmentsOverlapCollinear, DiagonalsAreNotConsidered) {
    Segment s1{{0.0, 0.0}, {100.0, 100.0}};
    Segment s2{{0.0, 0.0}, {100.0, 100.0}};  // identical diagonals
    EXPECT_FALSE(segments_overlap_collinear(s1, s2, kEps));
}

// ---------------------------------------------------------------------------
// segment_is_upward
// ---------------------------------------------------------------------------

TEST(SegmentIsUpward, RisingSegmentIsUpward) {
    Segment s{{0.0, 200.0}, {0.0, 100.0}};  // start below end (+y is down)
    EXPECT_TRUE(segment_is_upward(s, kEps));
}

TEST(SegmentIsUpward, DescendingSegmentIsNotUpward) {
    Segment s{{0.0, 100.0}, {0.0, 200.0}};
    EXPECT_FALSE(segment_is_upward(s, kEps));
}

TEST(SegmentIsUpward, NearHorizontalIsNotUpward) {
    Segment s{{0.0, 100.0}, {100.0, 99.0}};  // dy = 1 <= eps
    EXPECT_FALSE(segment_is_upward(s, kEps));
}

TEST(SegmentIsUpward, JustOverEpsIsUpward) {
    Segment s{{0.0, 103.0}, {100.0, 100.0}};  // dy = 3 > eps
    EXPECT_TRUE(segment_is_upward(s, kEps));
}

// ---------------------------------------------------------------------------
// polyline_to_segments
// ---------------------------------------------------------------------------

TEST(PolylineToSegments, NoMidpointsIsSingleSegment) {
    std::vector<Segment> segs = polyline_to_segments({0.0, 0.0}, {}, {100.0, 100.0});
    ASSERT_EQ(segs.size(), 1u);
    EXPECT_DOUBLE_EQ(segs[0].a.x, 0.0);
    EXPECT_DOUBLE_EQ(segs[0].b.x, 100.0);
}

TEST(PolylineToSegments, MidpointsProduceChainedSegments) {
    std::vector<Point> mids{{50.0, 0.0}, {50.0, 100.0}};
    std::vector<Segment> segs = polyline_to_segments({0.0, 0.0}, mids, {100.0, 100.0});
    ASSERT_EQ(segs.size(), 3u);
    EXPECT_DOUBLE_EQ(segs[0].b.x, 50.0);
    EXPECT_DOUBLE_EQ(segs[0].b.y, 0.0);
    EXPECT_DOUBLE_EQ(segs[1].a.y, 0.0);
    EXPECT_DOUBLE_EQ(segs[1].b.y, 100.0);
    EXPECT_DOUBLE_EQ(segs[2].b.x, 100.0);
    EXPECT_DOUBLE_EQ(segs[2].b.y, 100.0);
}
