/**
    @file layout_tools.h
    MaxMCP - Layout Validation MCP Tools

    Tools for validating patch layout geometry:
    - validate_layout: machine-check Phase 8 of organize-patch (overlaps, upward
      cords, cord-vs-object crossings, collinear cord overlaps) and return a
      structured findings list. Read-only — never modifies the patch.

    This header is the Max-facing glue: the MCP dispatch and schema. The plain data
    structures (LayoutObject / LayoutCord / LayoutCheckOptions) and the pure check
    orchestration (run_layout_checks) live in layout_checks.h, Max-API independent
    and unit-tested; only the data extraction (walking the patcher for rects and
    patchlines) needs the Max main thread.

    @ingroup maxmcp
*/

#ifndef LAYOUT_TOOLS_H
#define LAYOUT_TOOLS_H

#include "layout_checks.h"

#include <string>

#include <nlohmann/json.hpp>

// Forward declarations
struct _maxmcp;
typedef struct _maxmcp t_maxmcp;

namespace LayoutTools {

using json = nlohmann::json;

// ============================================================================
// MCP tool interface
// ============================================================================

/**
 * @brief Get the JSON schemas for all layout tools.
 *
 * @return JSON array of tool schemas (validate_layout)
 */
json get_tool_schemas();

/**
 * @brief Execute a layout tool.
 *
 * @param tool The tool name to execute
 * @param params The tool parameters as JSON
 * @return JSON result, or null if the tool is not handled by this module
 */
json execute(const std::string& tool, const json& params);

}  // namespace LayoutTools

#endif  // LAYOUT_TOOLS_H
