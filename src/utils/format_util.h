/**
    @file format_util.h
    MaxMCP - Small numeric-to-string helpers for finding/rationale text

    Header-only formatting helpers shared by the layout tools (layout_checks,
    io_geometry) so coordinate/area strings render consistently in one place
    instead of being duplicated per translation unit.

    @ingroup maxmcp
*/

#ifndef FORMAT_UTIL_H
#define FORMAT_UTIL_H

#include <cmath>
#include <cstdio>
#include <string>

namespace fmtutil {

// One-decimal coordinate, e.g. "876.8" — the precision used in layout findings
// and alignment rationales, without dumping full double precision.
inline std::string fmt1(double v) {
    char buf[32];
    std::snprintf(buf, sizeof(buf), "%.1f", v);
    return buf;
}

// Rounded integer string, for areas and overlap-region bounds.
inline std::string fmti(double v) {
    return std::to_string(static_cast<long>(std::llround(v)));
}

}  // namespace fmtutil

#endif  // FORMAT_UTIL_H
