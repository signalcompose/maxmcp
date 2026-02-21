/**
    @file patch_helpers.h
    MaxMCP - Patch Helper Functions

    Common utility functions for patch and object operations.

    @ingroup maxmcp
*/

#ifndef PATCH_HELPERS_H
#define PATCH_HELPERS_H

#include <string>
#include <unordered_set>
#include <vector>

#include <nlohmann/json.hpp>

#ifdef MAXMCP_TEST_MODE
// Forward declarations for test mode (no Max SDK)
struct _object;
typedef struct _object t_object;
struct _atom;
typedef struct _atom t_atom;
struct _symbol;
typedef struct _symbol t_symbol;
typedef struct _jrgba {
    double red;
    double green;
    double blue;
    double alpha;
} t_jrgba;
#else
// Include Max SDK for full type definitions
#include "ext.h"
#include "ext_obex.h"

#include "jpatcher_api.h"
#endif

namespace PatchHelpers {

/**
 * @brief Object types where the textfield IS the displayed content.
 *
 * These need explicit textfield setting because newobject_fromboxtext
 * does not populate their text. Other textfield objects (number, flonum,
 * metro, etc.) have their content managed internally by Max.
 */
inline const std::unordered_set<std::string> kTextfieldContentTypes = {
    "message",
    "comment",
    "textedit",
    "live.comment",
};

/**
 * @brief Attributes to always save during box replacement.
 *
 * These common box attributes may not have the save=1 meta-attribute
 * set (especially when modified via API), so they are explicitly whitelisted
 * to ensure properties are preserved across replace_object_text operations.
 *
 * Includes geometry (patching_rect, presentation_rect) and visual appearance.
 */
inline const std::unordered_set<std::string> kBoxAttributeWhitelist = {
    // Geometry
    "patching_rect",
    "presentation_rect",
    // Visual appearance
    "bgcolor",
    "textcolor",
    "textcolor_inverse",
    "fontname",
    "fontsize",
    "fontface",
    "color",
    "elementcolor",
    "accentcolor",
};

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

/**
 * @brief Convert a JSON value to a vector of t_atom
 *
 * Handles integer, float, string scalars and arrays containing these types.
 * For arrays, each element is recursively converted to a single atom.
 *
 * @param value JSON value (number, string, or array of these)
 * @return Vector of t_atom, empty if the value type is unsupported
 */
std::vector<t_atom> json_to_atoms(const nlohmann::json& value);

/**
 * @brief Set an attribute on a box from a JSON value
 *
 * Converts the JSON value to atoms via json_to_atoms() and sets the attribute
 * using object_attr_setvalueof().
 *
 * @param box The box object to set the attribute on
 * @param attr_name The attribute name
 * @param value The JSON value (number, string, or array)
 * @return true if the attribute was set, false if box is null or value is unsupported
 *
 * @note This function must be called on the main thread (or via defer)
 */
bool set_box_attribute(t_object* box, const std::string& attr_name, const nlohmann::json& value);

/**
 * @brief Build a space-separated text string from a JSON arguments array
 *
 * Converts each element (string, integer, float) to its string representation
 * and joins them with spaces.
 *
 * @param arguments JSON array of arguments
 * @return Concatenated text string, or empty string if arguments is null/empty
 */
std::string build_text_from_arguments(const nlohmann::json& arguments);

/**
 * @brief Check if an object type uses textfield as its primary content.
 *
 * Objects like message, comment, and textedit display their textfield
 * content directly. Other objects with textfields (number, flonum, metro, etc.)
 * use the textfield for internal display but their content is managed
 * differently by newobject_fromboxtext.
 *
 * @param obj_type The object type string (e.g. "message", "comment")
 * @return true if the object type uses textfield as its displayed content
 */
bool is_textfield_content_type(const std::string& obj_type);

/**
 * @brief Set text content on a textfield-based object (message, comment, etc.)
 *
 * Uses jbox_get_textfield() to detect if the box has a textfield,
 * then sets the text content via the textfield's "settext" method.
 *
 * @param box The box object to set text on
 * @param text The text content to set
 * @return true if text was set (box has a textfield), false otherwise
 *
 * @note This function must be called on the main thread (or via defer)
 */
bool set_textfield_content(t_object* box, const std::string& text);

// ============================================================================
// Box State Save/Restore (for replace_object_text and similar operations)
// ============================================================================

/**
 * @brief A single saved attribute: name + atom values
 */
struct SavedAttribute {
    t_symbol* name;
    std::vector<t_atom> values;
};

/**
 * @brief A single saved patchline connection with visual properties
 */
struct SavedConnection {
    t_object* other_box;
    long outlet;
    long inlet;
    bool target_is_src;  ///< true: target→other, false: other→target
    char hidden;
    t_jrgba color;
    std::vector<double> midpoints;  ///< flat [x1, y1, x2, y2, ...]
};

/**
 * @brief Get the textfield text content of a box
 *
 * @param box The box object
 * @return The text string, or empty if no textfield
 */
std::string get_box_text(t_object* box);

/**
 * @brief Save all saveable attributes of a box
 *
 * Enumerates all attributes and saves those that are writable AND either have
 * the "save" meta-attribute set to 1 or are in kBoxAttributeWhitelist.
 *
 * @param box The box object to save attributes from
 * @return Vector of saved attributes
 */
std::vector<SavedAttribute> save_box_attributes(t_object* box);

/**
 * @brief Save all patchline connections involving a box
 *
 * Iterates all patchlines in the patcher and collects those connected
 * to the specified box, including visual properties (hidden, color, midpoints).
 *
 * @param patcher The patcher containing the box
 * @param box The box whose connections to save
 * @return Vector of saved connections
 */
std::vector<SavedConnection> save_box_connections(t_object* patcher, t_object* box);

/**
 * @brief Restore saved attributes onto a new box
 *
 * @param new_box The new box to restore attributes to
 * @param attrs The saved attributes to restore
 */
void restore_box_attributes(t_object* new_box, const std::vector<SavedAttribute>& attrs);

/**
 * @brief Reconnect saved patchlines to a new box and restore visual properties
 *
 * @param patcher The patcher to create connections in
 * @param new_box The new box to reconnect to
 * @param connections The saved connections to restore
 * @return Number of connections restored
 */
long restore_box_connections(t_object* patcher, t_object* new_box,
                             const std::vector<SavedConnection>& connections);

}  // namespace PatchHelpers

#endif  // PATCH_HELPERS_H
