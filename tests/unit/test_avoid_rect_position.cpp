/**
    @file test_avoid_rect_position.cpp
    Unit tests for UtilityTools::find_avoid_rect_position (pure geometry).

    These tests exercise the Max-API independent placement logic behind the
    get_avoid_rect_position MCP tool: the non-overlap (gap) guarantee, the
    near-point search, nearest-spot selection, size sensitivity, bounds, the
    rationale strings, and the legacy "place to the right" fallback.
*/

#include "tools/utility_tools.h"

#include <cmath>
#include <string>
#include <vector>

#include <gtest/gtest.h>

using UtilityTools::find_avoid_rect_position;
using UtilityTools::PlacedPosition;
using UtilityTools::Rect;
using UtilityTools::rects_conflict;

namespace {

// Mirrors PLACE_GAP in utility_tools.cpp: the minimum clearance the algorithm
// must keep between the placed object and every existing object.
constexpr double kGap = 8.0;

// Assert the placed rect clears every existing object by at least kGap, reusing
// the production predicate (see the RectsConflict tests below for its own
// boundary coverage).
void expect_cleared(const PlacedPosition& placed, double width, double height,
                    const std::vector<Rect>& existing) {
    const Rect r{placed.position.x, placed.position.y, width, height};
    for (const Rect& e : existing) {
        EXPECT_FALSE(rects_conflict(r, e, kGap))
            << "placed (" << placed.position.x << "," << placed.position.y
            << ") is within the gap of existing (" << e.origin.x << "," << e.origin.y << ","
            << e.width << "," << e.height << ")";
    }
}

bool contains(const std::string& haystack, const std::string& needle) {
    return haystack.find(needle) != std::string::npos;
}

}  // namespace

// ---------------------------------------------------------------------------
// rects_conflict: the overlap/clearance predicate shared with the placement
// search. Tested directly because expect_cleared() relies on it.
// ---------------------------------------------------------------------------

// gap == 0 is a plain strict AABB overlap test.
TEST(RectsConflict, OverlappingRectsConflictWithoutGap) {
    Rect a{0.0, 0.0, 50.0, 50.0};
    Rect b{25.0, 25.0, 50.0, 50.0};  // overlaps a
    EXPECT_TRUE(rects_conflict(a, b, 0.0));
    EXPECT_TRUE(rects_conflict(b, a, 0.0));  // symmetric
}

TEST(RectsConflict, EdgeTouchingDoesNotConflictWithoutGap) {
    Rect a{0.0, 0.0, 50.0, 20.0};
    Rect b{50.0, 0.0, 50.0, 20.0};  // shares the x=50 edge, no overlap
    EXPECT_FALSE(rects_conflict(a, b, 0.0));
}

TEST(RectsConflict, OneAxisSeparatedDoesNotConflict) {
    Rect a{0.0, 0.0, 50.0, 20.0};
    Rect b{0.0, 100.0, 50.0, 20.0};  // same x span, far apart in y
    EXPECT_FALSE(rects_conflict(a, b, 0.0));
}

TEST(RectsConflict, GapBoundaryIsExclusive) {
    Rect a{0.0, 0.0, 10.0, 10.0};
    // b sits exactly `gap` to the right of a (a right edge = 10, gap = 8).
    Rect at_gap{18.0, 0.0, 10.0, 10.0};
    EXPECT_FALSE(rects_conflict(a, at_gap, 8.0)) << "exactly gap apart should clear";

    // One pixel closer than the gap -> conflict.
    Rect within_gap{17.0, 0.0, 10.0, 10.0};
    EXPECT_TRUE(rects_conflict(a, within_gap, 8.0));
}

TEST(RectsConflict, WithinGapOnlyConflictsWhenBothAxesClose) {
    Rect a{0.0, 0.0, 10.0, 10.0};
    // Within the gap horizontally but clearly separated vertically.
    Rect b{15.0, 200.0, 10.0, 10.0};
    EXPECT_FALSE(rects_conflict(a, b, 8.0));
}

// Empty patch, no anchor -> default origin (50, 50).
TEST(AvoidRectPosition, EmptyPatchReturnsOrigin) {
    PlacedPosition p = find_avoid_rect_position({}, 50.0, 20.0, false, 0.0, 0.0);
    EXPECT_DOUBLE_EQ(p.position.x, 50.0);
    EXPECT_DOUBLE_EQ(p.position.y, 50.0);
    EXPECT_TRUE(contains(p.rationale, "origin")) << p.rationale;
}

// No anchor with existing objects -> placed to the right of the rightmost edge
// (legacy behavior preserved).
TEST(AvoidRectPosition, NoAnchorPlacesToTheRight) {
    std::vector<Rect> existing{{50.0, 50.0, 100.0, 22.0}};
    PlacedPosition p = find_avoid_rect_position(existing, 50.0, 20.0, false, 0.0, 0.0);
    // rightmost edge = 150, + margin 50 = 200
    EXPECT_DOUBLE_EQ(p.position.x, 200.0);
    EXPECT_DOUBLE_EQ(p.position.y, 50.0);
    EXPECT_TRUE(contains(p.rationale, "right")) << p.rationale;
    expect_cleared(p, 50.0, 20.0, existing);
}

// A free anchor point is returned exactly, and the rationale names it.
TEST(AvoidRectPosition, FreeAnchorReturnedExactly) {
    std::vector<Rect> existing{{0.0, 0.0, 40.0, 40.0}};
    PlacedPosition p = find_avoid_rect_position(existing, 50.0, 20.0, true, 400.0, 300.0);
    EXPECT_DOUBLE_EQ(p.position.x, 400.0);
    EXPECT_DOUBLE_EQ(p.position.y, 300.0);
    EXPECT_TRUE(contains(p.rationale, "near")) << p.rationale;
    expect_cleared(p, 50.0, 20.0, existing);
}

// An anchor on top of an object resolves to the *nearest* free spot. For a
// single 100x40 object at (100,100) and a 50x20 object anchored at the object's
// center (150,120), the closest grid-aligned, gap-clearing spot is directly
// below at (150,150): the rows above/left/right all stay within the gap, so the
// search settles on the first free ring (radius 30) just under the object.
TEST(AvoidRectPosition, OccupiedAnchorPicksNearestFreeSpot) {
    std::vector<Rect> existing{{100.0, 100.0, 100.0, 40.0}};
    PlacedPosition p = find_avoid_rect_position(existing, 50.0, 20.0, true, 150.0, 120.0);
    EXPECT_DOUBLE_EQ(p.position.x, 150.0);
    EXPECT_DOUBLE_EQ(p.position.y, 150.0);
    EXPECT_TRUE(contains(p.rationale, "nearest")) << p.rationale;
    expect_cleared(p, 50.0, 20.0, existing);
}

// A small object fits a narrow vertical gap between two stacked objects; a tall
// object does not and must be relocated elsewhere. Both must stay cleared.
TEST(AvoidRectPosition, RespectsObjectSizeWhenFittingGaps) {
    std::vector<Rect> existing{{100.0, 100.0, 100.0, 40.0}, {100.0, 200.0, 100.0, 40.0}};
    // The gap spans y in [140, 200]; anchor at its middle.
    PlacedPosition small = find_avoid_rect_position(existing, 50.0, 20.0, true, 150.0, 170.0);
    EXPECT_DOUBLE_EQ(small.position.x, 150.0);
    EXPECT_DOUBLE_EQ(small.position.y, 170.0);  // 20px-tall object fits, anchor is already free
    expect_cleared(small, 50.0, 20.0, existing);

    PlacedPosition tall = find_avoid_rect_position(existing, 50.0, 80.0, true, 150.0, 170.0);
    // 80px tall cannot fit the 60px gap, so it is placed clear of both objects.
    EXPECT_FALSE(tall.position.y >= 140.0 && tall.position.y + 80.0 <= 200.0)
        << "tall object squeezed into the gap";
    expect_cleared(tall, 50.0, 80.0, existing);
}

// Densely packed neighbors: the result must still clear every object.
TEST(AvoidRectPosition, ClearsAllInClusteredLayout) {
    std::vector<Rect> existing;
    for (int row = 0; row < 4; ++row) {
        for (int col = 0; col < 4; ++col) {
            existing.push_back({100.0 + col * 60.0, 100.0 + row * 40.0, 50.0, 20.0});
        }
    }
    PlacedPosition p = find_avoid_rect_position(existing, 50.0, 20.0, true, 180.0, 160.0);
    expect_cleared(p, 50.0, 20.0, existing);
}

// Negative anchors are clamped; results stay in the visible (non-negative) area.
TEST(AvoidRectPosition, AvoidsNegativeCoordinates) {
    std::vector<Rect> existing{{0.0, 0.0, 200.0, 200.0}};
    PlacedPosition p = find_avoid_rect_position(existing, 50.0, 20.0, true, -100.0, -100.0);
    EXPECT_GE(p.position.x, 0.0);
    EXPECT_GE(p.position.y, 0.0);
    expect_cleared(p, 50.0, 20.0, existing);
}

// Determinism: identical inputs yield identical placements.
TEST(AvoidRectPosition, IsDeterministic) {
    std::vector<Rect> existing{{100.0, 100.0, 100.0, 40.0}};
    PlacedPosition a = find_avoid_rect_position(existing, 50.0, 20.0, true, 150.0, 120.0);
    PlacedPosition b = find_avoid_rect_position(existing, 50.0, 20.0, true, 150.0, 120.0);
    EXPECT_DOUBLE_EQ(a.position.x, b.position.x);
    EXPECT_DOUBLE_EQ(a.position.y, b.position.y);
}
