/**
    @file io_geometry.h
    MaxMCP - Inlet/outlet pixel-position reproduction (Max-API independent)

    Reproduces Max's inlet/outlet placement rule so callers can compute the pixel
    center of any nub without a connected patchcord. The Max SDK exposes no public
    getter for per-inlet/outlet coordinates, so this header encodes the rule and
    its calibrated constants; the constants are pinned by fixture tests measured on
    a real device (see tests/unit/test_io_geometry.cpp).

    Placement rule (calibrated on Max 9, 2026-06-04, across newobj / number /
    live.numbox / live.dial / gain~ / slider / live.gain~ / toggle):
      - Inlets sit on the top edge (y = rect.y); outlets on the bottom edge
        (y = rect.bottom()). This holds regardless of the box aspect ratio, so
        tall objects need no special handling — the height is already in bottom().
      - count == 1 : x = rect.x + single_inset
      - count >= 2 : x_i = (rect.x + edge_inset)
                           + (width - 2*edge_inset) * i / (count - 1)
      - edge_inset == single_inset == 9.5 for every object measured; a single
        global calibration is used. io_calibration_for() is the extension hook
        for the rare class that ever diverges.
      - Limitation: for pathologically narrow boxes (e.g. gain~ at width 22) Max
        rounds a nub by up to ~0.5px from this rule; widening removes it.

    Pure and Max-API independent (reuses geometry::Rect/Point only) so it is
    unit-tested without the SDK and shared by the get_io_position glue.

    @ingroup maxmcp
*/

#ifndef IO_GEOMETRY_H
#define IO_GEOMETRY_H

#include "utils/geometry.h"

#include <string>
#include <vector>

namespace geometry {

/**
 * @brief Which edge of the box the nubs sit on.
 */
enum class IoSide { Inlet, Outlet };

/**
 * @brief Calibrated insets for one object class (or the global default).
 *
 * @c edge_inset is the distance from the box's left edge to the center of the
 * leftmost nub (and symmetrically from the right edge to the rightmost nub) when
 * there are two or more nubs. @c single_inset is the center offset used when
 * there is exactly one nub. Both are 9.5 for every object measured to date.
 */
struct IoCalibration {
    double edge_inset;
    double single_inset;
};

/**
 * @brief One computed nub: its logical index and pixel center.
 *
 * @c center reuses geometry::Point so callers share one coordinate vocabulary.
 */
struct IoPosition {
    int index;
    Point center;
};

/**
 * @brief Calibration constants for a maxclass.
 *
 * Returns the per-class override when one is registered, otherwise the global
 * default (9.5 / 9.5). Calibration showed no class needs an override yet; this
 * lookup is the seam for adding one (or for a future Max version) without
 * touching call sites.
 *
 * @param maxclass The object's maxclass (e.g. "cycle~", "live.dial")
 * @return The calibration to use for that class
 */
IoCalibration io_calibration_for(const std::string& maxclass);

/**
 * @brief Compute nub centers from the placement rule (pure, no SDK).
 *
 * Applies the equal-spacing rule for @p visual_count nubs on the given side of
 * @p rect using @p cal. Returned positions are indexed 0..visual_count-1. A
 * @p visual_count <= 0 yields an empty vector.
 *
 * @param rect         The box's patching rectangle
 * @param visual_count Number of drawn nubs on this side
 * @param side         Inlet (top edge) or Outlet (bottom edge)
 * @param cal          Calibration constants
 * @return Nub centers, one per drawn nub, in index order
 */
std::vector<IoPosition> compute_io_positions(const Rect& rect, int visual_count, IoSide side,
                                             const IoCalibration& cal);

/**
 * @brief Compute nub centers for an object, resolving calibration and the
 *        drawfirstin correction.
 *
 * Convenience wrapper over compute_io_positions(): resolves calibration by
 * @p maxclass and, for inlets whose first nub is not drawn (@p draw_first_in is
 * false), computes @c logical_count-1 visible nubs and renumbers their @c index
 * to the logical range 1..logical_count-1 (so indices stay aligned with the
 * inlet numbers used by connect_max_objects). Outlets are unaffected by
 * @p draw_first_in. The drawfirstin branch follows the documented rule but was
 * not exercised by any measured object (all tested classes draw every inlet).
 *
 * @param rect          The box's patching rectangle
 * @param logical_count The object's logical inlet/outlet count
 * @param side          Inlet or Outlet
 * @param draw_first_in Whether the first inlet is drawn (inlets only)
 * @param maxclass      The object's maxclass, for calibration lookup
 * @return Nub centers with logical indices
 */
std::vector<IoPosition> io_positions(const Rect& rect, int logical_count, IoSide side,
                                     bool draw_first_in, const std::string& maxclass);

}  // namespace geometry

#endif  // IO_GEOMETRY_H
