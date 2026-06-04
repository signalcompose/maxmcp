/**
    @file io_geometry.cpp
    MaxMCP - Inlet/outlet placement rule implementation

    Pure reproduction of Max's nub placement (see io_geometry.h for the rule and
    its calibration provenance). Max-API independent; the SDK glue that supplies
    the rect, counts, and drawfirstin lives in src/tools/layout_tools.cpp.

    @ingroup maxmcp
*/

#include "io_geometry.h"

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

}  // namespace geometry
