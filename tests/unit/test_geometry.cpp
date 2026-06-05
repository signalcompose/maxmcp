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
#include <string>
#include <utility>
#include <vector>

#include <gtest/gtest.h>

using geometry::aabb_intersection;
using geometry::aabb_overlap;
using geometry::aabb_overlap_area;
using geometry::align_mode_name_list;
using geometry::AlignMode;
using geometry::AlignResult;
using geometry::parse_align_mode;
using geometry::Point;
using geometry::polyline_to_segments;
using geometry::recommend_alignment;
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

// ---------------------------------------------------------------------------
// recommend_alignment
// ---------------------------------------------------------------------------

// Find the recommended rect for input index i, or nullptr if it does not move.
static const Rect* move_for(const AlignResult& res, int i) {
    for (const auto& m : res.moves) {
        if (m.index == i) {
            return &m.rect;
        }
    }
    return nullptr;
}

TEST(ComputeAlignment, NeedsAtLeastTwoObjects) {
    std::vector<Rect> one{{10.0, 20.0, 50.0, 22.0}};
    AlignResult res = recommend_alignment(one, AlignMode::Left);
    EXPECT_FALSE(res.ok);
    EXPECT_TRUE(res.moves.empty());
}

TEST(ComputeAlignment, LeftSnapsToMinLeft) {
    std::vector<Rect> rects{
        {100.0, 0.0, 50.0, 22.0}, {30.0, 40.0, 80.0, 22.0}, {70.0, 80.0, 50.0, 22.0}};
    AlignResult res = recommend_alignment(rects, AlignMode::Left);
    ASSERT_TRUE(res.ok);
    // The leftmost object (index 1, x=30) does not move; the others snap to x=30.
    EXPECT_EQ(move_for(res, 1), nullptr);
    ASSERT_NE(move_for(res, 0), nullptr);
    EXPECT_DOUBLE_EQ(move_for(res, 0)->origin.x, 30.0);
    EXPECT_DOUBLE_EQ(move_for(res, 2)->origin.x, 30.0);
    // Other dimensions are preserved.
    EXPECT_DOUBLE_EQ(move_for(res, 0)->origin.y, 0.0);
    EXPECT_DOUBLE_EQ(move_for(res, 0)->width, 50.0);
}

TEST(ComputeAlignment, RightSnapsRightEdges) {
    std::vector<Rect> rects{{100.0, 0.0, 50.0, 22.0},   // right=150
                            {30.0, 40.0, 80.0, 22.0}};  // right=110
    AlignResult res = recommend_alignment(rects, AlignMode::Right);
    ASSERT_TRUE(res.ok);
    // max right is 150; object 1 moves so its right edge is 150 -> x = 150-80 = 70.
    EXPECT_EQ(move_for(res, 0), nullptr);
    ASSERT_NE(move_for(res, 1), nullptr);
    EXPECT_DOUBLE_EQ(move_for(res, 1)->origin.x, 70.0);
    EXPECT_DOUBLE_EQ(move_for(res, 1)->right(), 150.0);
}

TEST(ComputeAlignment, TopAndBottom) {
    std::vector<Rect> rects{{0.0, 100.0, 50.0, 20.0},  // bottom=120
                            {0.0, 40.0, 50.0, 60.0}};  // bottom=100
    AlignResult top = recommend_alignment(rects, AlignMode::Top);
    ASSERT_TRUE(top.ok);
    EXPECT_EQ(move_for(top, 1), nullptr);  // y=40 is the min top
    ASSERT_NE(move_for(top, 0), nullptr);
    EXPECT_DOUBLE_EQ(move_for(top, 0)->origin.y, 40.0);

    AlignResult bottom = recommend_alignment(rects, AlignMode::Bottom);
    ASSERT_TRUE(bottom.ok);
    EXPECT_EQ(move_for(bottom, 0), nullptr);  // bottom=120 is the max bottom
    ASSERT_NE(move_for(bottom, 1), nullptr);
    EXPECT_DOUBLE_EQ(move_for(bottom, 1)->bottom(), 120.0);
}

TEST(ComputeAlignment, HCenterUsesBoundingBoxCenter) {
    // Bounding box x spans [0, 200] -> center x = 100.
    std::vector<Rect> rects{{0.0, 0.0, 40.0, 22.0}, {160.0, 50.0, 40.0, 22.0}};
    AlignResult res = recommend_alignment(rects, AlignMode::HCenter);
    ASSERT_TRUE(res.ok);
    // Each 40-wide object centered on x=100 -> origin.x = 80.
    ASSERT_NE(move_for(res, 0), nullptr);
    ASSERT_NE(move_for(res, 1), nullptr);
    EXPECT_DOUBLE_EQ(move_for(res, 0)->origin.x, 80.0);
    EXPECT_DOUBLE_EQ(move_for(res, 1)->origin.x, 80.0);
}

TEST(ComputeAlignment, AlreadyAlignedYieldsNoMoves) {
    std::vector<Rect> rects{{50.0, 0.0, 30.0, 22.0}, {50.0, 40.0, 30.0, 22.0}};
    AlignResult res = recommend_alignment(rects, AlignMode::Left);
    EXPECT_TRUE(res.ok);
    EXPECT_TRUE(res.moves.empty());
}

TEST(ComputeAlignment, DistributeHNeedsThree) {
    std::vector<Rect> two{{0.0, 0.0, 20.0, 20.0}, {100.0, 0.0, 20.0, 20.0}};
    AlignResult res = recommend_alignment(two, AlignMode::DistributeH);
    EXPECT_FALSE(res.ok);
}

TEST(ComputeAlignment, DistributeHEqualGaps) {
    // Three 20-wide objects spanning [0, 200]: span=200, extent=60,
    // gap = (200-60)/2 = 70. Sorted by x: 0, then 70+20=90, then 180.
    std::vector<Rect> rects{
        {0.0, 0.0, 20.0, 20.0}, {50.0, 0.0, 20.0, 20.0}, {180.0, 0.0, 20.0, 20.0}};
    AlignResult res = recommend_alignment(rects, AlignMode::DistributeH);
    ASSERT_TRUE(res.ok);
    // Endpoints (index 0 at x=0, index 2 at x=180) stay; the middle moves to x=90.
    EXPECT_EQ(move_for(res, 0), nullptr);
    EXPECT_EQ(move_for(res, 2), nullptr);
    ASSERT_NE(move_for(res, 1), nullptr);
    EXPECT_DOUBLE_EQ(move_for(res, 1)->origin.x, 90.0);
}

TEST(ComputeAlignment, DistributeVEqualGaps) {
    // Three 20-tall objects spanning [0, 200]: gap = (200-60)/2 = 70.
    std::vector<Rect> rects{
        {0.0, 0.0, 20.0, 20.0}, {0.0, 30.0, 20.0, 20.0}, {0.0, 180.0, 20.0, 20.0}};
    AlignResult res = recommend_alignment(rects, AlignMode::DistributeV);
    ASSERT_TRUE(res.ok);
    EXPECT_EQ(move_for(res, 0), nullptr);
    EXPECT_EQ(move_for(res, 2), nullptr);
    ASSERT_NE(move_for(res, 1), nullptr);
    EXPECT_DOUBLE_EQ(move_for(res, 1)->origin.y, 90.0);
}

// ---------------------------------------------------------------------------
// AlignMode name table (parse_align_mode / align_mode_name_list / mode_name)
// ---------------------------------------------------------------------------

TEST(AlignModeNames, ParseAcceptsEveryKnownName) {
    // Each wire-format name must map to its enum. This pins the shared kModeNames
    // table so a rename or a missing entry is caught here, not only in the field.
    const std::pair<const char*, AlignMode> cases[] = {
        {"align_left", AlignMode::Left},
        {"align_right", AlignMode::Right},
        {"align_top", AlignMode::Top},
        {"align_bottom", AlignMode::Bottom},
        {"align_hcenter", AlignMode::HCenter},
        {"align_vcenter", AlignMode::VCenter},
        {"distribute_h", AlignMode::DistributeH},
        {"distribute_v", AlignMode::DistributeV},
    };
    for (const auto& [name, expected] : cases) {
        AlignMode mode;
        EXPECT_TRUE(parse_align_mode(name, mode)) << "should parse " << name;
        EXPECT_EQ(mode, expected) << "wrong enum for " << name;
    }
}

TEST(AlignModeNames, ParseRejectsUnknownName) {
    AlignMode mode = AlignMode::Right;
    EXPECT_FALSE(parse_align_mode("align_diagonal", mode));
    EXPECT_FALSE(parse_align_mode("", mode));
    // The out-parameter is left untouched on failure.
    EXPECT_EQ(mode, AlignMode::Right);
}

TEST(AlignModeNames, NameListMatchesEnumOrder) {
    EXPECT_EQ(align_mode_name_list(), "align_left, align_right, align_top, align_bottom, "
                                      "align_hcenter, align_vcenter, distribute_h, distribute_v");
}

TEST(AlignModeNames, EveryListedNameParsesBack) {
    // The error-message name list and the parser must never drift apart: every
    // name advertised in align_mode_name_list() round-trips through the parser.
    const std::string list = align_mode_name_list();
    size_t start = 0;
    int counted = 0;
    while (start < list.size()) {
        size_t comma = list.find(", ", start);
        const size_t end = (comma == std::string::npos) ? list.size() : comma;
        const std::string name = list.substr(start, end - start);
        AlignMode mode;
        EXPECT_TRUE(parse_align_mode(name, mode)) << "listed name does not parse: " << name;
        ++counted;
        start = (comma == std::string::npos) ? list.size() : comma + 2;
    }
    EXPECT_EQ(counted, 8) << "expected all 8 modes in the list";
}

TEST(AlignModeNames, ReasonStringCarriesModeName) {
    // recommend_alignment embeds mode_name() in its rationale; assert it so the
    // enum->string direction of the table is covered, not just executed.
    std::vector<Rect> rects{{100.0, 0.0, 50.0, 22.0}, {30.0, 40.0, 80.0, 22.0}};
    EXPECT_EQ(recommend_alignment(rects, AlignMode::Left).reason.rfind("align_left:", 0), 0u);
    EXPECT_EQ(recommend_alignment(rects, AlignMode::Bottom).reason.rfind("align_bottom:", 0), 0u);
}
