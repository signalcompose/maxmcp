/**
    @file test_layout_validate.cpp
    Unit tests for LayoutTools::run_layout_checks (pure check orchestration).

    These exercise the validate_layout decision logic without the Max SDK: the
    five checks (overlap, upward, cord_object, cord_cord, presentation_overlap),
    severity assignment, endpoint exclusion for cord-vs-object, and the summary
    counters. The underlying geometry is covered separately in test_geometry.cpp.
*/

#include "tools/layout_tools.h"

#include <string>
#include <vector>

#include <gtest/gtest.h>

using LayoutTools::LayoutCheckOptions;
using LayoutTools::LayoutCord;
using LayoutTools::LayoutObject;
using LayoutTools::run_layout_checks;
using json = nlohmann::json;

namespace {

// An object with a varname at a given rect (in_presentation defaults true so the
// same fixtures can be reused for presentation-mode checks).
LayoutObject obj(int id, const std::string& name, double x, double y, double w, double h) {
    return LayoutObject{id, name, "newobj", {x, y, w, h}, true};
}

// A straight (no-midpoint) cord between two objects.
LayoutCord cord(int src_id, const std::string& src, int dst_id, const std::string& dst, double sx,
                double sy, double ex, double ey) {
    return LayoutCord{src_id, src, 0, dst_id, dst, 0, {sx, sy}, {ex, ey}, {}};
}

int summary_of(const json& result, const std::string& key) {
    return result.at("summary").at(key).get<int>();
}

}  // namespace

// ---------------------------------------------------------------------------
// clean / overlap
// ---------------------------------------------------------------------------

TEST(RunLayoutChecks, CleanLayoutReportsClean) {
    std::vector<LayoutObject> objects{obj(0, "a", 0.0, 0.0, 50.0, 20.0),
                                      obj(1, "b", 200.0, 0.0, 50.0, 20.0)};
    json result = run_layout_checks(objects, {}, LayoutCheckOptions{});
    EXPECT_TRUE(result.at("clean").get<bool>());
    EXPECT_TRUE(result.at("findings").empty());
}

TEST(RunLayoutChecks, DetectsObjectOverlap) {
    std::vector<LayoutObject> objects{obj(0, "a", 0.0, 0.0, 100.0, 40.0),
                                      obj(1, "b", 50.0, 20.0, 100.0, 40.0)};
    json result = run_layout_checks(objects, {}, LayoutCheckOptions{});
    EXPECT_FALSE(result.at("clean").get<bool>());
    EXPECT_EQ(summary_of(result, "overlap"), 1);
    const json& f = result.at("findings").at(0);
    EXPECT_EQ(f.at("type"), "overlap");
    EXPECT_EQ(f.at("severity"), "error");
}

// ---------------------------------------------------------------------------
// upward
// ---------------------------------------------------------------------------

TEST(RunLayoutChecks, StraightUpwardCordIsError) {
    std::vector<LayoutObject> objects{obj(0, "a", 0.0, 200.0, 50.0, 20.0),
                                      obj(1, "b", 0.0, 0.0, 50.0, 20.0)};
    std::vector<LayoutCord> cords{cord(0, "a", 1, "b", 25.0, 220.0, 25.0, 0.0)};
    json result = run_layout_checks(objects, cords, LayoutCheckOptions{});
    EXPECT_EQ(summary_of(result, "upward"), 1);
    // Find the upward finding (overlap is absent here).
    const json& f = result.at("findings").at(0);
    EXPECT_EQ(f.at("type"), "upward");
    EXPECT_EQ(f.at("severity"), "error");
}

TEST(RunLayoutChecks, UpwardSegmentOfFoldedCordIsWarning) {
    std::vector<LayoutObject> objects{obj(0, "a", 0.0, 200.0, 50.0, 20.0),
                                      obj(1, "b", 0.0, 0.0, 50.0, 20.0)};
    LayoutCord c{0, "a", 0, 1, "b", 0, {25.0, 220.0}, {25.0, 0.0}, {{400.0, 220.0}, {400.0, 0.0}}};
    json result = run_layout_checks(objects, {c}, LayoutCheckOptions{});
    ASSERT_GE(summary_of(result, "upward"), 1);
    bool saw_warning = false;
    for (const json& f : result.at("findings")) {
        if (f.at("type") == "upward") {
            EXPECT_EQ(f.at("severity"), "warning");
            saw_warning = true;
        }
    }
    EXPECT_TRUE(saw_warning);
}

// ---------------------------------------------------------------------------
// cord_object
// ---------------------------------------------------------------------------

TEST(RunLayoutChecks, CordCrossingUnrelatedObjectIsFlagged) {
    // Cord from a(0) to b(1) runs horizontally through obstacle c(2).
    std::vector<LayoutObject> objects{obj(0, "a", 0.0, 100.0, 50.0, 20.0),
                                      obj(1, "b", 400.0, 100.0, 50.0, 20.0),
                                      obj(2, "c", 200.0, 90.0, 50.0, 40.0)};
    std::vector<LayoutCord> cords{cord(0, "a", 1, "b", 50.0, 110.0, 400.0, 110.0)};
    json result = run_layout_checks(objects, cords, LayoutCheckOptions{});
    EXPECT_EQ(summary_of(result, "cord_object"), 1);
    const json& f = result.at("findings").at(0);
    EXPECT_EQ(f.at("type"), "cord_object");
    EXPECT_EQ(f.at("object"), "c");
}

TEST(RunLayoutChecks, CordDoesNotFlagItsOwnEndpoints) {
    // The cord starts inside a's rect and ends inside b's rect; neither endpoint
    // object must be reported as a crossing.
    std::vector<LayoutObject> objects{obj(0, "a", 0.0, 100.0, 60.0, 40.0),
                                      obj(1, "b", 400.0, 100.0, 60.0, 40.0)};
    std::vector<LayoutCord> cords{cord(0, "a", 1, "b", 30.0, 120.0, 430.0, 120.0)};
    json result = run_layout_checks(objects, cords, LayoutCheckOptions{});
    EXPECT_EQ(summary_of(result, "cord_object"), 0);
    EXPECT_TRUE(result.at("clean").get<bool>());
}

// ---------------------------------------------------------------------------
// cord_cord
// ---------------------------------------------------------------------------

TEST(RunLayoutChecks, CollinearCordsOverlap) {
    std::vector<LayoutObject> objects{
        obj(0, "a", 0.0, 0.0, 50.0, 20.0), obj(1, "b", 0.0, 300.0, 50.0, 20.0),
        obj(2, "c", 0.0, 100.0, 50.0, 20.0), obj(3, "d", 0.0, 400.0, 50.0, 20.0)};
    // Two vertical cords sharing x=25, overlapping in y.
    std::vector<LayoutCord> cords{cord(0, "a", 1, "b", 25.0, 20.0, 25.0, 300.0),
                                  cord(2, "c", 3, "d", 25.0, 120.0, 25.0, 400.0)};
    LayoutCheckOptions opt;
    opt.check_overlap = false;  // isolate the cord_cord check
    opt.check_cord_object = false;
    opt.check_upward = false;
    json result = run_layout_checks(objects, cords, opt);
    EXPECT_EQ(summary_of(result, "cord_cord"), 1);
}

// ---------------------------------------------------------------------------
// presentation mode
// ---------------------------------------------------------------------------

TEST(RunLayoutChecks, PresentationOverlapLabelledAndCordsSkipped) {
    std::vector<LayoutObject> objects{obj(0, "a", 0.0, 0.0, 100.0, 40.0),
                                      obj(1, "b", 50.0, 20.0, 100.0, 40.0)};
    // A cord that would normally be upward; in presentation mode it is ignored.
    std::vector<LayoutCord> cords{cord(0, "a", 1, "b", 25.0, 220.0, 25.0, 0.0)};
    LayoutCheckOptions opt;
    opt.presentation_mode = true;
    json result = run_layout_checks(objects, cords, opt);
    EXPECT_EQ(summary_of(result, "presentation_overlap"), 1);
    EXPECT_EQ(summary_of(result, "upward"), 0);
    EXPECT_EQ(result.at("findings").at(0).at("type"), "presentation_overlap");
}

// ---------------------------------------------------------------------------
// check selection
// ---------------------------------------------------------------------------

TEST(RunLayoutChecks, DisabledChecksProduceNoFindings) {
    std::vector<LayoutObject> objects{obj(0, "a", 0.0, 0.0, 100.0, 40.0),
                                      obj(1, "b", 50.0, 20.0, 100.0, 40.0)};
    LayoutCheckOptions opt;
    opt.check_overlap = false;
    json result = run_layout_checks(objects, {}, opt);
    EXPECT_TRUE(result.at("clean").get<bool>());
    EXPECT_EQ(summary_of(result, "overlap"), 0);
}
