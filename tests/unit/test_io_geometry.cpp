/**
    @file test_io_geometry.cpp
    Unit tests for the inlet/outlet placement rule (src/utils/io_geometry.cpp).

    Two layers:
      1. Pure-rule tests of compute_io_positions / io_positions / io_calibration_for
         with literal inputs (spacing, single-nub inset, inlet/outlet y, the
         drawfirstin renumbering, empty edge cases).
      2. Calibration fixtures: real cord-endpoint measurements taken on Max 9
         (2026-06-04) via jpatchline_get_startpoint/endpoint. These pin the
         calibrated constant (9.5) so a Max-version change that shifts a nub trips
         a failure — the whole point of issue #76 is that this is verified, not
         inferred. Measurement method: connect a cord to each nub, read its real
         pixel center, after pinning and reading back the box's patching_rect.
*/

#include "utils/io_geometry.h"

#include <string>
#include <vector>

#include <gtest/gtest.h>

using geometry::AlignAdjust;
using geometry::AlignmentResult;
using geometry::compute_io_positions;
using geometry::io_calibration_for;
using geometry::io_positions;
using geometry::IoCalibration;
using geometry::IoPosition;
using geometry::IoSide;
using geometry::recommend_alignment_rect;
using geometry::Rect;

namespace {

// The calibrated default both insets share.
constexpr double kInset = 9.5;

// Assert a computed position list matches expected {index, x, y} triples within
// tol. Exact fixtures use a tight tol; the narrow-box case loosens it (see note).
void expect_positions(const std::vector<IoPosition>& actual,
                      const std::vector<std::tuple<int, double, double>>& expected, double tol) {
    ASSERT_EQ(actual.size(), expected.size());
    for (size_t i = 0; i < expected.size(); ++i) {
        EXPECT_EQ(actual[i].index, std::get<0>(expected[i])) << "index mismatch at " << i;
        EXPECT_NEAR(actual[i].center.x, std::get<1>(expected[i]), tol) << "x mismatch at " << i;
        EXPECT_NEAR(actual[i].center.y, std::get<2>(expected[i]), tol) << "y mismatch at " << i;
    }
}

}  // namespace

// ---------------------------------------------------------------------------
// io_calibration_for
// ---------------------------------------------------------------------------

TEST(IoCalibration, DefaultsTo95ForUnknownClass) {
    IoCalibration cal = io_calibration_for("some.unmeasured.object");
    EXPECT_DOUBLE_EQ(cal.edge_inset, 9.5);
    EXPECT_DOUBLE_EQ(cal.single_inset, 9.5);
}

TEST(IoCalibration, NewobjUsesDefault) {
    IoCalibration cal = io_calibration_for("cycle~");
    EXPECT_DOUBLE_EQ(cal.edge_inset, 9.5);
    EXPECT_DOUBLE_EQ(cal.single_inset, 9.5);
}

// ---------------------------------------------------------------------------
// compute_io_positions — the pure rule
// ---------------------------------------------------------------------------

TEST(ComputeIoPositions, SingleNubUsesSingleInsetOnTopEdge) {
    Rect rect{100.0, 50.0, 120.0, 22.0};
    auto pos = compute_io_positions(rect, 1, IoSide::Inlet, {kInset, kInset});
    expect_positions(pos, {{0, 109.5, 50.0}}, 1e-9);
}

TEST(ComputeIoPositions, SingleNubOutletUsesBottomEdge) {
    Rect rect{100.0, 50.0, 120.0, 22.0};
    auto pos = compute_io_positions(rect, 1, IoSide::Outlet, {kInset, kInset});
    expect_positions(pos, {{0, 109.5, 72.0}}, 1e-9);  // y = 50 + 22
}

TEST(ComputeIoPositions, TwoNubsLandOnEdgeInsets) {
    Rect rect{100.0, 50.0, 120.0, 22.0};
    auto pos = compute_io_positions(rect, 2, IoSide::Inlet, {kInset, kInset});
    // left = 100+9.5 = 109.5, right = 220-9.5 = 210.5
    expect_positions(pos, {{0, 109.5, 50.0}, {1, 210.5, 50.0}}, 1e-9);
}

TEST(ComputeIoPositions, FourNubsAreEquallySpaced) {
    Rect rect{300.0, 200.0, 120.0, 22.0};
    auto pos = compute_io_positions(rect, 4, IoSide::Inlet, {kInset, kInset});
    // step = (120-19)/3 = 33.6667
    expect_positions(pos,
                     {{0, 309.5, 200.0},
                      {1, 343.16666666666667, 200.0},
                      {2, 376.83333333333333, 200.0},
                      {3, 410.5, 200.0}},
                     1e-9);
}

TEST(ComputeIoPositions, NonPositiveCountIsEmpty) {
    Rect rect{0.0, 0.0, 100.0, 20.0};
    EXPECT_TRUE(compute_io_positions(rect, 0, IoSide::Inlet, {kInset, kInset}).empty());
    EXPECT_TRUE(compute_io_positions(rect, -1, IoSide::Outlet, {kInset, kInset}).empty());
}

// ---------------------------------------------------------------------------
// io_positions — calibration resolution + drawfirstin correction
// ---------------------------------------------------------------------------

TEST(IoPositions, DrawnInletsKeepLogicalIndices) {
    Rect rect{300.0, 200.0, 120.0, 22.0};
    auto pos = io_positions(rect, 2, IoSide::Inlet, /*draw_first_in=*/true, "cycle~");
    expect_positions(pos, {{0, 309.5, 200.0}, {1, 410.5, 200.0}}, 1e-9);
}

TEST(IoPositions, UndrawnFirstInletDropsNubAndRenumbers) {
    Rect rect{300.0, 200.0, 120.0, 22.0};
    // 3 logical inlets, first not drawn -> 2 visible nubs at logical indices 1,2.
    auto pos = io_positions(rect, 3, IoSide::Inlet, /*draw_first_in=*/false, "fake.class");
    // 2 nubs spaced across the edge insets, indices shifted to 1,2.
    expect_positions(pos, {{1, 309.5, 200.0}, {2, 410.5, 200.0}}, 1e-9);
}

TEST(IoPositions, UndrawnFirstInletWithSingleLogicalInletIsEmpty) {
    Rect rect{300.0, 200.0, 120.0, 22.0};
    EXPECT_TRUE(
        io_positions(rect, 1, IoSide::Inlet, /*draw_first_in=*/false, "fake.class").empty());
}

TEST(IoPositions, OutletsIgnoreDrawFirstIn) {
    Rect rect{300.0, 200.0, 120.0, 22.0};
    auto with = io_positions(rect, 2, IoSide::Outlet, /*draw_first_in=*/true, "number");
    auto without = io_positions(rect, 2, IoSide::Outlet, /*draw_first_in=*/false, "number");
    ASSERT_EQ(with.size(), without.size());
    for (size_t i = 0; i < with.size(); ++i) {
        EXPECT_EQ(with[i].index, without[i].index);
        EXPECT_DOUBLE_EQ(with[i].center.x, without[i].center.x);
        EXPECT_DOUBLE_EQ(with[i].center.y, without[i].center.y);
    }
}

// ---------------------------------------------------------------------------
// Calibration fixtures — measured on Max 9 (2026-06-04)
//
// Each case pins io_positions() against real cord-endpoint centers. Rects are the
// values read back from Max after pinning (UI objects clamp their height). Tol is
// tight (0.01) for every class except the deliberately narrow gain~, which Max
// rounds by ~0.5px (documented limitation).
// ---------------------------------------------------------------------------

TEST(IoCalibrationFixtures, CycleNewobj) {
    Rect rect{300.0, 200.0, 120.0, 22.0};
    expect_positions(io_positions(rect, 2, IoSide::Inlet, true, "cycle~"),
                     {{0, 309.5, 200.0}, {1, 410.5, 200.0}}, 0.01);
    expect_positions(io_positions(rect, 1, IoSide::Outlet, true, "cycle~"), {{0, 309.5, 222.0}},
                     0.01);
}

TEST(IoCalibrationFixtures, PakFourInlets) {
    Rect rect{300.0, 200.0, 120.0, 22.0};
    expect_positions(io_positions(rect, 4, IoSide::Inlet, true, "pak"),
                     {{0, 309.5, 200.0},
                      {1, 343.16666666666667, 200.0},
                      {2, 376.83333333333333, 200.0},
                      {3, 410.5, 200.0}},
                     0.01);
}

TEST(IoCalibrationFixtures, NumberBoxTwoOutlets) {
    Rect rect{300.0, 200.0, 120.0, 22.0};
    expect_positions(io_positions(rect, 1, IoSide::Inlet, true, "number"), {{0, 309.5, 200.0}},
                     0.01);
    expect_positions(io_positions(rect, 2, IoSide::Outlet, true, "number"),
                     {{0, 309.5, 222.0}, {1, 410.5, 222.0}}, 0.01);
}

TEST(IoCalibrationFixtures, LiveNumbox) {
    Rect rect{300.0, 200.0, 120.0, 15.0};  // height clamped to 15
    expect_positions(io_positions(rect, 1, IoSide::Inlet, true, "live.numbox"), {{0, 309.5, 200.0}},
                     0.01);
    expect_positions(io_positions(rect, 2, IoSide::Outlet, true, "live.numbox"),
                     {{0, 309.5, 215.0}, {1, 410.5, 215.0}}, 0.01);
}

TEST(IoCalibrationFixtures, LiveDial) {
    Rect rect{300.0, 200.0, 120.0, 48.0};  // height clamped to 48
    expect_positions(io_positions(rect, 1, IoSide::Inlet, true, "live.dial"), {{0, 309.5, 200.0}},
                     0.01);
    expect_positions(io_positions(rect, 2, IoSide::Outlet, true, "live.dial"),
                     {{0, 309.5, 248.0}, {1, 410.5, 248.0}}, 0.01);
}

TEST(IoCalibrationFixtures, GainTallWide) {
    Rect rect{300.0, 180.0, 120.0, 140.0};  // tall object, normal width
    expect_positions(io_positions(rect, 1, IoSide::Inlet, true, "gain~"), {{0, 309.5, 180.0}},
                     0.01);
    expect_positions(io_positions(rect, 2, IoSide::Outlet, true, "gain~"),
                     {{0, 309.5, 320.0}, {1, 410.5, 320.0}}, 0.01);
}

TEST(IoCalibrationFixtures, GainNarrowRoundingLimitation) {
    // gain~ at its native narrow width (22px). Max rounds the right outlet to
    // 313.0 where the rule predicts 312.5 — a sub-pixel deviation that only
    // appears on pathologically narrow boxes. Documented limitation; loose tol.
    Rect rect{300.0, 180.0, 22.0, 140.0};
    expect_positions(io_positions(rect, 2, IoSide::Outlet, true, "gain~"),
                     {{0, 309.5, 320.0}, {1, 313.0, 320.0}}, 0.6);
}

TEST(IoCalibrationFixtures, SliderTallNarrowSingleNub) {
    Rect rect{300.0, 160.0, 20.0, 140.0};
    expect_positions(io_positions(rect, 1, IoSide::Inlet, true, "slider"), {{0, 309.5, 160.0}},
                     0.01);
    expect_positions(io_positions(rect, 1, IoSide::Outlet, true, "slider"), {{0, 309.5, 300.0}},
                     0.01);
}

TEST(IoCalibrationFixtures, LiveGainFiveOutlets) {
    Rect rect{300.0, 160.0, 48.0, 136.0};  // tall Live UI, 2 inlets / 5 outlets
    expect_positions(io_positions(rect, 2, IoSide::Inlet, true, "live.gain~"),
                     {{0, 309.5, 160.0}, {1, 338.5, 160.0}}, 0.01);
    expect_positions(io_positions(rect, 5, IoSide::Outlet, true, "live.gain~"),
                     {{0, 309.5, 296.0},
                      {1, 316.75, 296.0},
                      {2, 324.0, 296.0},
                      {3, 331.25, 296.0},
                      {4, 338.5, 296.0}},
                     0.01);
}

TEST(IoCalibrationFixtures, ToggleSquareSingleNub) {
    Rect rect{300.0, 200.0, 24.0, 24.0};
    expect_positions(io_positions(rect, 1, IoSide::Inlet, true, "toggle"), {{0, 309.5, 200.0}},
                     0.01);
    expect_positions(io_positions(rect, 1, IoSide::Outlet, true, "toggle"), {{0, 309.5, 224.0}},
                     0.01);
}

// ---------------------------------------------------------------------------
// recommend_alignment_rect — inverse placement (suggest_alignment)
// ---------------------------------------------------------------------------

namespace {
// Assert the recommended rect round-trips: the chosen nub lands on anchor_x.
void expect_nub_at(const Rect& rect, int count, IoSide side, int index, bool draw_first_in,
                   const std::string& maxclass, double anchor_x) {
    auto pos = io_positions(rect, count, side, draw_first_in, maxclass);
    bool found = false;
    for (const auto& p : pos) {
        if (p.index == index) {
            EXPECT_NEAR(p.center.x, anchor_x, 1e-6);
            found = true;
        }
    }
    EXPECT_TRUE(found) << "index " << index << " not present after alignment";
}
}  // namespace

TEST(RecommendAlignment, WidthReproducesSpecExample) {
    // Spec §4: 6-inlet target at left=453, align inlet 5 onto anchor x=669.5 by
    // resizing -> width 226 (right - 9.5 == 669.5). Current width is irrelevant.
    Rect target{453.0, 1180.0, 80.0, 20.0};
    AlignmentResult r = recommend_alignment_rect(target, 6, IoSide::Inlet, 5, true, "newobj", 669.5,
                                                 AlignAdjust::Width);
    ASSERT_TRUE(r.ok) << r.reason;
    EXPECT_DOUBLE_EQ(r.rect.origin.x, 453.0);
    EXPECT_DOUBLE_EQ(r.rect.origin.y, 1180.0);
    EXPECT_DOUBLE_EQ(r.rect.width, 226.0);
    EXPECT_DOUBLE_EQ(r.rect.height, 20.0);
    expect_nub_at(r.rect, 6, IoSide::Inlet, 5, true, "newobj", 669.5);
}

TEST(RecommendAlignment, LeftMovesSingleInlet) {
    Rect target{100.0, 200.0, 50.0, 22.0};
    AlignmentResult r = recommend_alignment_rect(target, 1, IoSide::Inlet, 0, true, "number", 300.0,
                                                 AlignAdjust::Left);
    ASSERT_TRUE(r.ok) << r.reason;
    EXPECT_DOUBLE_EQ(r.rect.origin.x, 290.5);  // 300 - 9.5
    EXPECT_DOUBLE_EQ(r.rect.width, 50.0);      // width unchanged
    expect_nub_at(r.rect, 1, IoSide::Inlet, 0, true, "number", 300.0);
}

TEST(RecommendAlignment, LeftMovesMultiNub) {
    Rect target{100.0, 200.0, 120.0, 22.0};
    AlignmentResult r = recommend_alignment_rect(target, 2, IoSide::Inlet, 1, true, "newobj", 500.0,
                                                 AlignAdjust::Left);
    ASSERT_TRUE(r.ok) << r.reason;
    EXPECT_DOUBLE_EQ(r.rect.width, 120.0);  // width unchanged
    expect_nub_at(r.rect, 2, IoSide::Inlet, 1, true, "newobj", 500.0);
}

TEST(RecommendAlignment, WidthCannotMoveLeftmostNub) {
    Rect target{100.0, 200.0, 120.0, 22.0};
    AlignmentResult r = recommend_alignment_rect(target, 4, IoSide::Inlet, 0, true, "newobj", 300.0,
                                                 AlignAdjust::Width);
    EXPECT_FALSE(r.ok);
    EXPECT_NE(r.reason.find("leftmost"), std::string::npos);
}

TEST(RecommendAlignment, WidthCannotMoveSingleNub) {
    Rect target{100.0, 200.0, 50.0, 22.0};
    AlignmentResult r = recommend_alignment_rect(target, 1, IoSide::Outlet, 0, true, "number",
                                                 300.0, AlignAdjust::Width);
    EXPECT_FALSE(r.ok);
    EXPECT_NE(r.reason.find("single nub"), std::string::npos);
}

TEST(RecommendAlignment, WidthRejectsNonPositiveSolution) {
    // anchor far left of where inlet 1 can reach -> solved width <= 0.
    Rect target{100.0, 200.0, 120.0, 22.0};
    AlignmentResult r = recommend_alignment_rect(target, 2, IoSide::Inlet, 1, true, "newobj", 50.0,
                                                 AlignAdjust::Width);
    EXPECT_FALSE(r.ok);
    EXPECT_NE(r.reason.find("non-positive"), std::string::npos);
}

TEST(RecommendAlignment, IndexOutOfRangeFails) {
    Rect target{100.0, 200.0, 120.0, 22.0};
    AlignmentResult r = recommend_alignment_rect(target, 2, IoSide::Inlet, 5, true, "newobj", 300.0,
                                                 AlignAdjust::Left);
    EXPECT_FALSE(r.ok);
    EXPECT_NE(r.reason.find("out of range"), std::string::npos);
}

TEST(RecommendAlignment, DrawfirstinUndrawnInletZeroFails) {
    Rect target{100.0, 200.0, 120.0, 22.0};
    AlignmentResult r = recommend_alignment_rect(
        target, 3, IoSide::Inlet, 0, /*draw_first_in=*/false, "x", 300.0, AlignAdjust::Left);
    EXPECT_FALSE(r.ok);
    EXPECT_NE(r.reason.find("not drawn"), std::string::npos);
}

TEST(RecommendAlignment, DrawfirstinRenumberedNubAligns) {
    // 3 logical inlets, first undrawn -> visible nubs at logical 1,2. Align nub 2.
    Rect target{100.0, 200.0, 120.0, 22.0};
    AlignmentResult r = recommend_alignment_rect(
        target, 3, IoSide::Inlet, 2, /*draw_first_in=*/false, "x", 400.0, AlignAdjust::Left);
    ASSERT_TRUE(r.ok) << r.reason;
    expect_nub_at(r.rect, 3, IoSide::Inlet, 2, false, "x", 400.0);
}
