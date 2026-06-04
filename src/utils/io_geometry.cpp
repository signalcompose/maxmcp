/**
    @file io_geometry.cpp
    MaxMCP - Inlet/outlet placement rule implementation

    Pure reproduction of Max's nub placement (see io_geometry.h for the rule and
    its calibration provenance). Max-API independent; the SDK glue that supplies
    the rect, counts, and drawfirstin lives in src/tools/layout_tools.cpp.

    @ingroup maxmcp
*/

#include "io_geometry.h"

#include "format_util.h"

#include <string>
#include <unordered_map>
#include <vector>

namespace geometry {

// Every object measured on Max 9 placed its nubs with a 9.5px inset on both the
// two-or-more case (edge_inset) and the single-nub case (single_inset).
namespace {
constexpr IoCalibration kDefaultCalibration{/*edge_inset=*/9.5, /*single_inset=*/9.5};

// Per-class overrides. Empty today — calibration found no class that diverges
// from the default. Add a row here (and a fixture in test_io_geometry.cpp) if a
// class or Max version is ever measured to differ.
const std::unordered_map<std::string, IoCalibration>& class_calibration() {
    static const std::unordered_map<std::string, IoCalibration> table{};
    return table;
}
}  // namespace

IoCalibration io_calibration_for(const std::string& maxclass) {
    const auto& table = class_calibration();
    auto it = table.find(maxclass);
    return it != table.end() ? it->second : kDefaultCalibration;
}

std::vector<IoPosition> compute_io_positions(const Rect& rect, int visual_count, IoSide side,
                                             const IoCalibration& cal) {
    std::vector<IoPosition> positions;
    if (visual_count <= 0) {
        return positions;
    }

    const double y = (side == IoSide::Inlet) ? rect.origin.y : rect.bottom();

    if (visual_count == 1) {
        positions.push_back({0, {rect.origin.x + cal.single_inset, y}});
        return positions;
    }

    // Two or more nubs: equally spaced between the left and right inset centers.
    const double left_center = rect.origin.x + cal.edge_inset;
    const double right_center = rect.right() - cal.edge_inset;
    const double span = right_center - left_center;
    positions.reserve(visual_count);
    for (int i = 0; i < visual_count; ++i) {
        const double x = left_center + span * i / (visual_count - 1);
        positions.push_back({i, {x, y}});
    }
    return positions;
}

std::vector<IoPosition> io_positions(const Rect& rect, int logical_count, IoSide side,
                                     bool draw_first_in, const std::string& maxclass) {
    const IoCalibration cal = io_calibration_for(maxclass);

    // Outlets always draw every nub; inlets draw every nub unless drawfirstin is
    // false, in which case logical inlet 0 has no nub and the visible nubs map to
    // logical indices 1..logical_count-1.
    if (side == IoSide::Outlet || draw_first_in) {
        return compute_io_positions(rect, logical_count, side, cal);
    }

    std::vector<IoPosition> positions = compute_io_positions(rect, logical_count - 1, side, cal);
    for (IoPosition& p : positions) {
        p.index += 1;  // shift 0..n-2 back onto logical indices 1..n-1
    }
    return positions;
}

AlignmentResult recommend_alignment_rect(const Rect& target_rect, int target_logical_count,
                                         IoSide side, int target_index, bool draw_first_in,
                                         const std::string& maxclass, double anchor_x,
                                         AlignAdjust adjust) {
    if (target_index < 0 || target_index >= target_logical_count) {
        return {false,
                {},
                "target index " + std::to_string(target_index) + " out of range (count " +
                    std::to_string(target_logical_count) + ")"};
    }

    // Map the logical index onto the visible-nub frame (drawfirstin correction):
    // an undrawn first inlet shifts logical indices 1..n-1 onto visible 0..n-2.
    const bool undrawn_first = (side == IoSide::Inlet) && !draw_first_in;
    int visual_count = target_logical_count;
    int visual_index = target_index;
    if (undrawn_first) {
        if (target_index == 0) {
            return {false, {}, "target inlet 0 is not drawn (drawfirstin); it has no position"};
        }
        visual_count = target_logical_count - 1;
        visual_index = target_index - 1;
    }

    const IoCalibration cal = io_calibration_for(maxclass);
    const double left = target_rect.origin.x;
    const double w = target_rect.width;

    double new_left = left;
    double new_width = w;

    if (visual_count == 1) {
        // Single nub: x = left + single_inset, independent of width.
        if (adjust == AlignAdjust::Width) {
            return {false, {}, "a single nub cannot be aligned by width; use adjust=\"left\""};
        }
        new_left = anchor_x - cal.single_inset;
    } else {
        // x_i = left + edge + (w - 2*edge) * vi/(vc-1)
        const double edge = cal.edge_inset;
        const double frac = static_cast<double>(visual_index) / (visual_count - 1);
        if (adjust == AlignAdjust::Left) {
            new_left = anchor_x - edge - (w - 2 * edge) * frac;
        } else {  // Width: keep left, solve for the width that lands nub vi on anchor_x.
            if (visual_index == 0) {
                return {
                    false, {}, "the leftmost nub cannot be aligned by width; use adjust=\"left\""};
            }
            // anchor_x = left + edge + (new_width - 2*edge) * vi/(vc-1)
            new_width = 2 * edge + (anchor_x - left - edge) * (visual_count - 1) / visual_index;
            if (new_width <= 0.0) {
                return {false,
                        {},
                        "solved width " + fmtutil::fmt1(new_width) +
                            " is non-positive; the anchor is left of where this nub can reach"};
            }
        }
    }

    const Rect result{new_left, target_rect.origin.y, new_width, target_rect.height};
    const std::string nub =
        ((side == IoSide::Inlet) ? "inlet" : "outlet") + std::to_string(target_index);
    std::string reason = nub + " x must equal " + fmtutil::fmt1(anchor_x) + "; ";
    reason +=
        (adjust == AlignAdjust::Width)
            ? "with left=" + fmtutil::fmt1(left) + " -> width=" + fmtutil::fmt1(new_width)
            : "with width=" + fmtutil::fmt1(new_width) + " -> left=" + fmtutil::fmt1(new_left);
    return {true, result, reason};
}

}  // namespace geometry
