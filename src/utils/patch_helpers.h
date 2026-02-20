/**
    @file patch_helpers.h
    MaxMCP - Patch Helper Functions

    Common utility functions for patch and object operations.

    @ingroup maxmcp
*/

#ifndef PATCH_HELPERS_H
#define PATCH_HELPERS_H

#include <string>

#ifdef MAXMCP_TEST_MODE
// Forward declarations for test mode (no Max SDK)
struct _object;
typedef struct _object t_object;
#else
// Include Max SDK for full type definitions
#include "ext.h"
#include "ext_obex.h"

#include "jpatcher_api.h"
#endif

namespace PatchHelpers {

/**
 * @brief Find a box in a patcher by its varname
 *
 * Iterates through all boxes in the patcher and returns the first
 * box with a matching varname attribute.
 *
 * @param patcher The patcher object to search in
 * @param varname The varname to search for
 * @return Pointer to the box object, or nullptr if not found
 *
 * @note This function must be called on the main thread (or via defer)
 */
t_object* find_box_by_varname(t_object* patcher, const std::string& varname);

/**
 * @brief Get inlet count for a box
 *
 * @param box The box object
 * @return Number of inlets, or -1 on error
 */
long get_inlet_count(t_object* box);

/**
 * @brief Get outlet count for a box
 *
 * @param box The box object
 * @return Number of outlets, or -1 on error
 */
long get_outlet_count(t_object* box);

}  // namespace PatchHelpers

#endif  // PATCH_HELPERS_H
