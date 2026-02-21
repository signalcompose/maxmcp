/**
    @file object_tools.cpp
    MaxMCP - Object Operation MCP Tools Implementation

    @ingroup maxmcp
*/

#include "object_tools.h"

#include "maxmcp.h"
#include "tool_common.h"
#include "utils/console_logger.h"
#include "utils/patch_helpers.h"
#include "utils/patch_registry.h"

#include <set>
#include <vector>

#ifndef MAXMCP_TEST_MODE
#include "ext.h"
#include "ext_obex.h"

#include "jpatcher_api.h"
#endif

namespace ObjectTools {

// ============================================================================
// Data Structures for Deferred Callbacks
// ============================================================================

struct t_add_object_data {
    t_maxmcp* patch;
    std::string obj_type;
    double x;
    double y;
    std::string varname;
    json arguments;
    json attributes;
    ToolCommon::DeferredResult* deferred_result;
};

struct t_remove_object_data {
    t_maxmcp* patch;
    std::string varname;
    ToolCommon::DeferredResult* deferred_result;
};

struct t_set_attribute_data {
    t_maxmcp* patch;
    std::string varname;
    std::string attribute;
    json value;
    ToolCommon::DeferredResult* deferred_result;
};

struct t_get_attribute_data {
    t_maxmcp* patch;
    std::string varname;
    std::string attribute;
    ToolCommon::DeferredResult* deferred_result;
};

struct t_get_objects_data {
    t_maxmcp* patch;
    ToolCommon::DeferredResult* deferred_result;
};

struct t_get_io_info_data {
    t_maxmcp* patch;
    std::string varname;
    ToolCommon::DeferredResult* deferred_result;
};

struct t_get_hidden_data {
    t_maxmcp* patch;
    std::string varname;
    ToolCommon::DeferredResult* deferred_result;
};

struct t_set_hidden_data {
    t_maxmcp* patch;
    std::string varname;
    bool hidden;
    ToolCommon::DeferredResult* deferred_result;
};

struct t_redraw_data {
    t_maxmcp* patch;
    std::string varname;
    ToolCommon::DeferredResult* deferred_result;
};

struct t_replace_text_data {
    t_maxmcp* patch;
    std::string varname;
    std::string new_text;
    ToolCommon::DeferredResult* deferred_result;
};

struct t_assign_varnames_data {
    t_maxmcp* patch;
    json assignments;
    ToolCommon::DeferredResult* deferred_result;
};

struct t_get_value_data {
    t_maxmcp* patch;
    std::string varname;
    ToolCommon::DeferredResult* deferred_result;
};

// ============================================================================
// Deferred Callbacks (run on main thread)
// ============================================================================

#ifndef MAXMCP_TEST_MODE

static void add_object_deferred(t_maxmcp* patch, t_symbol* s, long argc, t_atom* argv) {
    VALIDATE_DEFERRED_ARGS("add_object_deferred");
    EXTRACT_DEFERRED_DATA_WITH_RESULT(t_add_object_data, data, argv);

    // Build object string with arguments
    std::string obj_string = data->obj_type;

    if (!data->arguments.is_null() && data->arguments.is_array()) {
        for (const auto& arg : data->arguments) {
            obj_string += " ";
            if (arg.is_number_integer()) {
                obj_string += std::to_string(arg.get<int>());
            } else if (arg.is_number_float()) {
                obj_string += std::to_string(arg.get<double>());
            } else if (arg.is_string()) {
                obj_string += arg.get<std::string>();
            }
        }
    }

    // Create object using newobject_fromboxtext (same as typing in object box)
    t_object* obj = (t_object*)newobject_fromboxtext(data->patch->patcher, obj_string.c_str());

    if (obj) {
        // Set position
        t_atom pos[2];
        atom_setfloat(&pos[0], data->x);
        atom_setfloat(&pos[1], data->y);
        object_attr_setvalueof(obj, gensym("patching_position"), 2, pos);
        // Set varname if provided
        if (!data->varname.empty()) {
            object_attr_setsym(obj, gensym("varname"), gensym(data->varname.c_str()));
        }

        // Set attributes if provided
        if (!data->attributes.is_null() && data->attributes.is_object()) {
            for (auto& [attr_name, attr_value] : data->attributes.items()) {
                PatchHelpers::set_box_attribute(obj, attr_name, attr_value);
            }
        }

        // Set text content for textfield-content objects (message, comment, textedit).
        // newobject_fromboxtext handles arguments for normal objects (e.g. "metro 500"),
        // but does NOT populate displayed text for these types.
        if (PatchHelpers::is_textfield_content_type(data->obj_type)) {
            std::string text_content = PatchHelpers::build_text_from_arguments(data->arguments);
            if (!text_content.empty()) {
                PatchHelpers::set_textfield_content(obj, text_content);
            }
        }

        object_method(obj, gensym("bringtofront"));
        object_attr_setlong(obj, gensym("presentation"), 1);
        jpatcher_set_dirty(data->patch->patcher, 1);
        ConsoleLogger::log(("Object created: " + obj_string).c_str());

        COMPLETE_DEFERRED(data, (json{{"result",
                                       {{"status", "success"},
                                        {"obj_type", data->obj_type},
                                        {"position", json::array({data->x, data->y})},
                                        {"varname", data->varname}}}}));
    } else {
        std::string msg = "Failed to create object: " + obj_string;
        ConsoleLogger::log(msg.c_str());
        COMPLETE_DEFERRED(data, ToolCommon::make_error(ToolCommon::ErrorCode::INTERNAL_ERROR, msg));
    }
}

static void remove_object_deferred(t_maxmcp* patch, t_symbol* s, long argc, t_atom* argv) {
    VALIDATE_DEFERRED_ARGS("remove_object_deferred");
    EXTRACT_DEFERRED_DATA_WITH_RESULT(t_remove_object_data, data, argv);

    t_object* box = PatchHelpers::find_box_by_varname(data->patch->patcher, data->varname);

    if (box) {
        object_free(box);
        jpatcher_set_dirty(data->patch->patcher, 1);
        ConsoleLogger::log(("Object removed: " + data->varname).c_str());
        COMPLETE_DEFERRED(data,
                          (json{{"result", {{"status", "success"}, {"varname", data->varname}}}}));
    } else {
        std::string msg = "Object not found: " + data->varname;
        ConsoleLogger::log(msg.c_str());
        COMPLETE_DEFERRED(data, ToolCommon::object_not_found_error(data->varname));
    }
}

static void set_attribute_deferred(t_maxmcp* patch, t_symbol* s, long argc, t_atom* argv) {
    VALIDATE_DEFERRED_ARGS("set_attribute_deferred");
    EXTRACT_DEFERRED_DATA_WITH_RESULT(t_set_attribute_data, data, argv);

    t_object* box = PatchHelpers::find_box_by_varname(data->patch->patcher, data->varname);

    if (!box) {
        std::string msg = "Object not found: " + data->varname;
        ConsoleLogger::log(msg.c_str());
        COMPLETE_DEFERRED(data, ToolCommon::object_not_found_error(data->varname));
        return;
    }

    if (!PatchHelpers::set_box_attribute(box, data->attribute, data->value)) {
        std::string msg = "Unsupported value type for attribute: " + data->attribute;
        ConsoleLogger::log(msg.c_str());
        COMPLETE_DEFERRED(data, ToolCommon::make_error(ToolCommon::ErrorCode::INVALID_PARAMS, msg));
        return;
    }

    jpatcher_set_dirty(data->patch->patcher, 1);
    ConsoleLogger::log(("Attribute set: " + data->varname + "." + data->attribute).c_str());
    COMPLETE_DEFERRED(data, (json{{"result",
                                   {{"status", "success"},
                                    {"varname", data->varname},
                                    {"attribute", data->attribute}}}}));
}

static void get_attribute_deferred(t_maxmcp* patch, t_symbol* s, long argc, t_atom* argv) {
    VALIDATE_DEFERRED_ARGS("get_attribute_deferred");
    EXTRACT_DEFERRED_DATA_WITH_RESULT(t_get_attribute_data, data, argv);

    t_object* box = PatchHelpers::find_box_by_varname(data->patch->patcher, data->varname);

    if (!box) {
        std::string msg = "Object not found: " + data->varname;
        ConsoleLogger::log(msg.c_str());
        COMPLETE_DEFERRED(data, ToolCommon::object_not_found_error(data->varname));
        return;
    }

    long ac = 0;
    t_atom* av = nullptr;
    t_max_err err = object_attr_getvalueof(box, gensym(data->attribute.c_str()), &ac, &av);

    if (err != MAX_ERR_NONE || ac == 0) {
        if (av)
            sysmem_freeptr(av);
        std::string msg = "Attribute not found or has no value: " + data->attribute;
        ConsoleLogger::log(msg.c_str());
        COMPLETE_DEFERRED(data, ToolCommon::make_error(ToolCommon::ErrorCode::INVALID_PARAMS, msg));
        return;
    }

    json value = PatchHelpers::atoms_to_json(ac, av);
    sysmem_freeptr(av);

    ConsoleLogger::log(("Attribute read: " + data->varname + "." + data->attribute).c_str());
    COMPLETE_DEFERRED(
        data,
        (json{{"result",
               {{"varname", data->varname}, {"attribute", data->attribute}, {"value", value}}}}));
}

static void get_objects_deferred(t_maxmcp* patch, t_symbol* s, long argc, t_atom* argv) {
    VALIDATE_DEFERRED_ARGS("get_objects_deferred");
    EXTRACT_DEFERRED_DATA_WITH_RESULT(t_get_objects_data, data, argv);

    json objects = json::array();
    t_object* patcher = data->patch->patcher;
    int index = 0;

    for (t_object* box = jpatcher_get_firstobject(patcher); box; box = jbox_get_nextobject(box)) {
        t_symbol* varname = object_attr_getsym(box, gensym("varname"));
        std::string varname_str = (varname && varname->s_name) ? varname->s_name : "";

        t_symbol* maxclass = jbox_get_maxclass(box);
        std::string maxclass_str = (maxclass && maxclass->s_name) ? maxclass->s_name : "unknown";

        std::string text = PatchHelpers::get_box_text(box);

        t_rect rect;
        jbox_get_patching_rect(box, &rect);

        json obj_info = {{"index", index},
                         {"maxclass", maxclass_str},
                         {"text", text},
                         {"position", json::array({rect.x, rect.y})},
                         {"size", json::array({rect.width, rect.height})}};

        if (!varname_str.empty()) {
            obj_info["varname"] = varname_str;
        }

        objects.push_back(obj_info);
        index++;
    }

    COMPLETE_DEFERRED(data, (json{{"patch_id", data->patch->patch_id},
                                  {"objects", objects},
                                  {"count", objects.size()}}));
}

static void get_io_info_deferred(t_maxmcp* patch, t_symbol* s, long argc, t_atom* argv) {
    VALIDATE_DEFERRED_ARGS("get_io_info_deferred");
    EXTRACT_DEFERRED_DATA_WITH_RESULT(t_get_io_info_data, data, argv);

    t_object* box = PatchHelpers::find_box_by_varname(data->patch->patcher, data->varname);

    if (!box) {
        COMPLETE_DEFERRED(data, ToolCommon::object_not_found_error(data->varname));
        return;
    }

    long inlet_count = PatchHelpers::get_inlet_count(box);
    long outlet_count = PatchHelpers::get_outlet_count(box);

    COMPLETE_DEFERRED(data, (json{{"varname", data->varname},
                                  {"inlet_count", inlet_count},
                                  {"outlet_count", outlet_count}}));
}

static void get_hidden_deferred(t_maxmcp* patch, t_symbol* s, long argc, t_atom* argv) {
    VALIDATE_DEFERRED_ARGS("get_hidden_deferred");
    EXTRACT_DEFERRED_DATA_WITH_RESULT(t_get_hidden_data, data, argv);

    t_object* box = PatchHelpers::find_box_by_varname(data->patch->patcher, data->varname);

    if (!box) {
        COMPLETE_DEFERRED(data, ToolCommon::object_not_found_error(data->varname));
        return;
    }

    char hidden = jbox_get_hidden(box);

    COMPLETE_DEFERRED(data, (json{{"varname", data->varname}, {"hidden", hidden != 0}}));
}

static void set_hidden_deferred(t_maxmcp* patch, t_symbol* s, long argc, t_atom* argv) {
    VALIDATE_DEFERRED_ARGS("set_hidden_deferred");
    EXTRACT_DEFERRED_DATA_WITH_RESULT(t_set_hidden_data, data, argv);

    t_object* box = PatchHelpers::find_box_by_varname(data->patch->patcher, data->varname);

    if (!box) {
        COMPLETE_DEFERRED(data, ToolCommon::object_not_found_error(data->varname));
        return;
    }

    jbox_set_hidden(box, data->hidden ? 1 : 0);
    jpatcher_set_dirty(data->patch->patcher, 1);

    ConsoleLogger::log(("Object " + data->varname +
                        " hidden set to: " + std::string(data->hidden ? "true" : "false"))
                           .c_str());

    COMPLETE_DEFERRED(
        data, (json{{"success", true}, {"varname", data->varname}, {"hidden", data->hidden}}));
}

static void redraw_deferred(t_maxmcp* patch, t_symbol* s, long argc, t_atom* argv) {
    VALIDATE_DEFERRED_ARGS("redraw_deferred");
    EXTRACT_DEFERRED_DATA_WITH_RESULT(t_redraw_data, data, argv);

    t_object* box = PatchHelpers::find_box_by_varname(data->patch->patcher, data->varname);

    if (!box) {
        COMPLETE_DEFERRED(data, ToolCommon::object_not_found_error(data->varname));
        return;
    }

    jbox_redraw(reinterpret_cast<t_jbox*>(box));
    ConsoleLogger::log(("Object redrawn: " + data->varname).c_str());

    COMPLETE_DEFERRED(data, (json{{"success", true}, {"varname", data->varname}}));
}

/**
 * Deferred callback for replacing object box text.
 * Saves connections, deletes object, recreates with new text, restores connections.
 * Executes on the Max main thread via defer().
 */
static void replace_object_text_deferred(t_maxmcp* patch, t_symbol* s, long argc, t_atom* argv) {
    VALIDATE_DEFERRED_ARGS("replace_object_text_deferred");
    EXTRACT_DEFERRED_DATA_WITH_RESULT(t_replace_text_data, data, argv);

    t_object* patcher = data->patch->patcher;
    t_object* box = PatchHelpers::find_box_by_varname(patcher, data->varname);

    if (!box) {
        std::string msg = "Object not found: " + data->varname;
        ConsoleLogger::log(msg.c_str());
        COMPLETE_DEFERRED(data, ToolCommon::object_not_found_error(data->varname));
        return;
    }

    // --- 1. Save current box state ---
    t_symbol* maxclass = jbox_get_maxclass(box);
    std::string maxclass_str = (maxclass && maxclass->s_name) ? maxclass->s_name : "";
    std::string old_text = PatchHelpers::get_box_text(box);
    auto saved_attrs = PatchHelpers::save_box_attributes(box);
    auto saved_connections = PatchHelpers::save_box_connections(patcher, box);

    // --- 2. Delete the old box ---
    object_free(box);
    box = nullptr;

    // --- 3. Create new box ---
    t_object* new_box = nullptr;
    bool is_textfield_type = PatchHelpers::is_textfield_content_type(maxclass_str);

    if (is_textfield_type) {
        new_box = (t_object*)newobject_fromboxtext(patcher, maxclass_str.c_str());
    } else {
        new_box = (t_object*)newobject_fromboxtext(patcher, data->new_text.c_str());
    }

    if (!new_box) {
        std::string msg = "Failed to recreate object with text: " + data->new_text;
        ConsoleLogger::log(msg.c_str());
        COMPLETE_DEFERRED(data, ToolCommon::make_error(ToolCommon::ErrorCode::INTERNAL_ERROR, msg));
        return;
    }

    // --- 4. Restore state ---
    PatchHelpers::restore_box_attributes(new_box, saved_attrs);
    object_attr_setsym(new_box, gensym("varname"), gensym(data->varname.c_str()));

    if (is_textfield_type) {
        PatchHelpers::set_textfield_content(new_box, data->new_text);
    }

    long reconnected = PatchHelpers::restore_box_connections(patcher, new_box, saved_connections);
    jpatcher_set_dirty(patcher, 1);

    ConsoleLogger::log(("Object text replaced: " + data->varname + " (" +
                        std::to_string(reconnected) + " connections restored)")
                           .c_str());

    COMPLETE_DEFERRED(data, (json{{"result",
                                   {{"status", "success"},
                                    {"varname", data->varname},
                                    {"old_text", old_text},
                                    {"new_text", data->new_text},
                                    {"reconnected", reconnected}}}}));
}

static void assign_varnames_deferred(t_maxmcp* patch, t_symbol* s, long argc, t_atom* argv) {
    VALIDATE_DEFERRED_ARGS("assign_varnames_deferred");
    EXTRACT_DEFERRED_DATA_WITH_RESULT(t_assign_varnames_data, data, argv);

    t_object* patcher = data->patch->patcher;

    // Build index → box mapping
    std::vector<t_object*> boxes;
    for (t_object* box = jpatcher_get_firstobject(patcher); box; box = jbox_get_nextobject(box)) {
        boxes.push_back(box);
    }

    // Validate all indices are in range
    for (const auto& assignment : data->assignments) {
        int idx = assignment["index"].get<int>();
        if (idx < 0 || idx >= static_cast<int>(boxes.size())) {
            std::string msg = "Index " + std::to_string(idx) + " out of range (0-" +
                              std::to_string(boxes.size() - 1) + ")";
            COMPLETE_DEFERRED(data,
                              ToolCommon::make_error(ToolCommon::ErrorCode::INVALID_PARAMS, msg));
            return;
        }
    }

    // Apply varname assignments
    json assigned = json::array();
    for (const auto& assignment : data->assignments) {
        int idx = assignment["index"].get<int>();
        std::string varname = assignment["varname"].get<std::string>();
        t_object* box = boxes[idx];

        object_attr_setsym(box, gensym("varname"), gensym(varname.c_str()));

        t_symbol* maxclass = jbox_get_maxclass(box);
        std::string maxclass_str = (maxclass && maxclass->s_name) ? maxclass->s_name : "unknown";

        assigned.push_back({{"index", idx}, {"varname", varname}, {"maxclass", maxclass_str}});
    }

    jpatcher_set_dirty(patcher, 1);
    ConsoleLogger::log(
        ("Varnames assigned: " + std::to_string(assigned.size()) + " objects").c_str());

    COMPLETE_DEFERRED(data, (json{{"result",
                                   {{"status", "success"},
                                    {"assigned", static_cast<int>(assigned.size())},
                                    {"assignments", assigned}}}}));
}

static void get_value_deferred(t_maxmcp* patch, t_symbol* s, long argc, t_atom* argv) {
    VALIDATE_DEFERRED_ARGS("get_value_deferred");
    EXTRACT_DEFERRED_DATA_WITH_RESULT(t_get_value_data, data, argv);

    t_object* box = PatchHelpers::find_box_by_varname(data->patch->patcher, data->varname);

    if (!box) {
        std::string msg = "Object not found: " + data->varname;
        ConsoleLogger::log(msg.c_str());
        COMPLETE_DEFERRED(data, ToolCommon::object_not_found_error(data->varname));
        return;
    }

    long ac = 0;
    t_atom* av = nullptr;
    t_max_err err = object_getvalueof(box, &ac, &av);

    if (err != MAX_ERR_NONE || ac == 0) {
        if (av)
            freebytes(av, ac * sizeof(t_atom));
        std::string msg = "Object does not support getvalueof or has no value: " + data->varname;
        ConsoleLogger::log(msg.c_str());
        COMPLETE_DEFERRED(data, ToolCommon::make_error(ToolCommon::ErrorCode::INVALID_PARAMS, msg));
        return;
    }

    json value = PatchHelpers::atoms_to_json(ac, av);
    freebytes(av, ac * sizeof(t_atom));

    ConsoleLogger::log(("Value read: " + data->varname).c_str());
    COMPLETE_DEFERRED(data, (json{{"result", {{"varname", data->varname}, {"value", value}}}}));
}

#endif  // MAXMCP_TEST_MODE

// ============================================================================
// Tool Schemas
// ============================================================================

// clang-format off
json get_tool_schemas() {
    return json::array({
        // ---- add_max_object ----
        {
            {"name", "add_max_object"},
            {"description", "Add a Max object to a patch"},
            {"inputSchema", {
                {"type", "object"},
                {"properties", {
                    {"patch_id", {{"type", "string"}, {"description", "Patch ID to add object to"}}},
                    {"obj_type", {
                        {"type", "string"},
                        {"description", "Max object type (e.g., 'number', 'button', 'dac~')"}
                    }},
                    {"position", {
                        {"type", "array"},
                        {"items", {{"type", "number"}}},
                        {"description", "Position [x, y] in patch"}
                    }},
                    {"varname", {
                        {"type", "string"},
                        {"description", "Variable name for the object (optional)"}
                    }},
                    {"arguments", {
                        {"type", "array"},
                        {"description", "Object arguments (e.g., [440] for 'cycle~')"}
                    }},
                    {"attributes", {
                        {"type", "object"},
                        {"description", "Object attributes (e.g., {\"bgcolor\": [1.0, 0.5, 0.0, 1.0]})"}
                    }}
                }},
                {"required", json::array({"patch_id", "obj_type", "position"})}
            }}
        },
        // ---- remove_max_object ----
        {
            {"name", "remove_max_object"},
            {"description", "Remove a Max object from a patch by varname"},
            {"inputSchema", {
                {"type", "object"},
                {"properties", {
                    {"patch_id", {{"type", "string"}, {"description", "Patch ID containing the object"}}},
                    {"varname", {{"type", "string"}, {"description", "Variable name of the object to remove"}}}
                }},
                {"required", json::array({"patch_id", "varname"})}
            }}
        },
        // ---- get_objects_in_patch ----
        {
            {"name", "get_objects_in_patch"},
            {"description", "List all objects in a patch with metadata"},
            {"inputSchema", {
                {"type", "object"},
                {"properties", {
                    {"patch_id", {{"type", "string"}, {"description", "Patch ID to query"}}}
                }},
                {"required", json::array({"patch_id"})}
            }}
        },
        // ---- set_object_attribute ----
        {
            {"name", "set_object_attribute"},
            {"description",
                "Set an attribute of a Max object. "
                "Supports scalar (number, string) and array values. "
                "Array examples: patching_rect [x, y, width, height], "
                "patching_position [x, y], bgcolor [r, g, b, a]"},
            {"inputSchema", {
                {"type", "object"},
                {"properties", {
                    {"patch_id", {{"type", "string"}, {"description", "Patch ID containing the object"}}},
                    {"varname", {{"type", "string"}, {"description", "Variable name of the object"}}},
                    {"attribute", {{"type", "string"}, {"description", "Attribute name to set"}}},
                    {"value", {
                        {"description",
                            "Attribute value (number, string, or array). "
                            "e.g. 440, \"hello\", [100, 200, 300, 50]"}
                    }}
                }},
                {"required", json::array({"patch_id", "varname", "attribute", "value"})}
            }}
        },
        // ---- get_object_attribute ----
        {
            {"name", "get_object_attribute"},
            {"description",
                "Get the value of an attribute of a Max object. "
                "Returns the current value of the specified attribute."},
            {"inputSchema", {
                {"type", "object"},
                {"properties", {
                    {"patch_id", {{"type", "string"}, {"description", "Patch ID containing the object"}}},
                    {"varname", {{"type", "string"}, {"description", "Variable name of the object"}}},
                    {"attribute", {
                        {"type", "string"},
                        {"description", "Attribute name to get (e.g., patching_rect, bgcolor, fontsize)"}
                    }}
                }},
                {"required", json::array({"patch_id", "varname", "attribute"})}
            }}
        },
        // ---- get_object_io_info ----
        {
            {"name", "get_object_io_info"},
            {"description", "Get inlet and outlet count for an object"},
            {"inputSchema", {
                {"type", "object"},
                {"properties", {
                    {"patch_id", {{"type", "string"}, {"description", "Patch ID containing the object"}}},
                    {"varname", {{"type", "string"}, {"description", "Variable name of the object"}}}
                }},
                {"required", json::array({"patch_id", "varname"})}
            }}
        },
        // ---- get_object_hidden ----
        {
            {"name", "get_object_hidden"},
            {"description", "Check if an object is hidden"},
            {"inputSchema", {
                {"type", "object"},
                {"properties", {
                    {"patch_id", {{"type", "string"}, {"description", "Patch ID"}}},
                    {"varname", {{"type", "string"}, {"description", "Variable name of the object"}}}
                }},
                {"required", json::array({"patch_id", "varname"})}
            }}
        },
        // ---- set_object_hidden ----
        {
            {"name", "set_object_hidden"},
            {"description", "Set the visibility of an object in a patch"},
            {"inputSchema", {
                {"type", "object"},
                {"properties", {
                    {"patch_id", {{"type", "string"}, {"description", "Patch ID"}}},
                    {"varname", {{"type", "string"}, {"description", "Variable name of the object"}}},
                    {"hidden", {{"type", "boolean"}, {"description", "true=hide, false=show"}}}
                }},
                {"required", json::array({"patch_id", "varname", "hidden"})}
            }}
        },
        // ---- redraw_object ----
        {
            {"name", "redraw_object"},
            {"description", "Force redraw of a specific object"},
            {"inputSchema", {
                {"type", "object"},
                {"properties", {
                    {"patch_id", {{"type", "string"}, {"description", "Patch ID"}}},
                    {"varname", {{"type", "string"}, {"description", "Variable name of the object"}}}
                }},
                {"required", json::array({"patch_id", "varname"})}
            }}
        },
        // ---- replace_object_text ----
        {
            {"name", "replace_object_text"},
            {"description",
                "Replace the box text of an existing Max object by deleting and recreating it. "
                "All patchcord connections are automatically saved and restored. "
                "For regular objects, new_text is the full box text (e.g., 'cycle~ 880'). "
                "For message/comment/textedit, new_text is the displayed text content."},
            {"inputSchema", {
                {"type", "object"},
                {"properties", {
                    {"patch_id", {{"type", "string"}, {"description", "Patch ID containing the object"}}},
                    {"varname", {
                        {"type", "string"},
                        {"description", "Variable name of the object to replace text for"}
                    }},
                    {"new_text", {
                        {"type", "string"},
                        {"description",
                            "New box text. For regular objects: full text including class "
                            "(e.g., 'cycle~ 880'). For message/comment/textedit: the displayed "
                            "text content."}
                    }}
                }},
                {"required", json::array({"patch_id", "varname", "new_text"})}
            }}
        },
        // ---- assign_varnames ----
        {
            {"name", "assign_varnames"},
            {"description",
                "Assign varnames to objects identified by index. Use get_objects_in_patch first "
                "to get object indices, then assign meaningful varnames based on object type and "
                "context. Existing varnames can be overwritten."},
            {"inputSchema", {
                {"type", "object"},
                {"properties", {
                    {"patch_id", {{"type", "string"}, {"description", "Patch ID"}}},
                    {"assignments", {
                        {"type", "array"},
                        {"items", {
                            {"type", "object"},
                            {"properties", {
                                {"index", {
                                    {"type", "integer"},
                                    {"description", "Object index from get_objects_in_patch"}
                                }},
                                {"varname", {
                                    {"type", "string"},
                                    {"description", "Varname to assign (e.g., 'osc_440', 'gain_ctrl')"}
                                }}
                            }},
                            {"required", json::array({"index", "varname"})}
                        }},
                        {"description", "Array of index-varname pairs to assign"}
                    }}
                }},
                {"required", json::array({"patch_id", "assignments"})}
            }}
        },
        // ---- get_object_value ----
        {
            {"name", "get_object_value"},
            {"description",
                "Get the current value of a Max object. Uses object_getvalueof() internally. "
                "Works with objects that implement the getvalueof interface "
                "(e.g., number boxes, flonum, sliders, dials). "
                "Returns the value as a number or array depending on the object type."},
            {"inputSchema", {
                {"type", "object"},
                {"properties", {
                    {"patch_id", {{"type", "string"}, {"description", "Patch ID containing the object"}}},
                    {"varname", {{"type", "string"}, {"description", "Variable name of the object"}}}
                }},
                {"required", json::array({"patch_id", "varname"})}
            }}
        }
    });
}
// clang-format on

// ============================================================================
// Tool Execution
// ============================================================================

#ifndef MAXMCP_TEST_MODE

json execute_add_max_object(const json& params) {
    std::string patch_id = params.value("patch_id", "");
    std::string obj_type = params.value("obj_type", "");
    std::string varname = params.value("varname", "");
    json arguments = params.value("arguments", json::array());

    json attributes = json::object();
    if (params.contains("attributes")) {
        if (params["attributes"].is_string()) {
            try {
                attributes = json::parse(params["attributes"].get<std::string>());
            } catch (const json::exception& e) {
                return ToolCommon::make_error(ToolCommon::ErrorCode::INVALID_PARAMS,
                                              "Invalid attributes JSON: " + std::string(e.what()));
            }
        } else if (params["attributes"].is_object()) {
            attributes = params["attributes"];
        }
    }

    if (patch_id.empty() || obj_type.empty()) {
        return ToolCommon::make_error(ToolCommon::ErrorCode::INVALID_PARAMS,
                                      "Missing required parameters: patch_id and obj_type");
    }

    if (!params.contains("position") || !params["position"].is_array() ||
        params["position"].size() < 2) {
        return ToolCommon::make_error(ToolCommon::ErrorCode::INVALID_PARAMS,
                                      "Invalid position parameter: must be array [x, y]");
    }

    double x = params["position"][0].get<double>();
    double y = params["position"][1].get<double>();

    t_maxmcp* patch = PatchRegistry::find_patch(patch_id);
    if (!patch) {
        return ToolCommon::patch_not_found_error(patch_id);
    }

    auto* deferred_result = new ToolCommon::DeferredResult();
    auto* data = new t_add_object_data{patch,   obj_type,  x,          y,
                                       varname, arguments, attributes, deferred_result};

    t_atom a;
    atom_setobj(&a, data);
    defer(patch, (method)add_object_deferred, gensym("add_object"), 1, &a);

    if (!deferred_result->wait_for(ToolCommon::DEFAULT_DEFER_TIMEOUT)) {
        delete deferred_result;
        return ToolCommon::timeout_error("creating object");
    }

    json result = deferred_result->result;
    delete deferred_result;
    return result;
}

json execute_remove_max_object(const json& params) {
    std::string patch_id = params.value("patch_id", "");
    std::string varname = params.value("varname", "");

    if (patch_id.empty() || varname.empty()) {
        return ToolCommon::make_error(ToolCommon::ErrorCode::INVALID_PARAMS,
                                      "Missing required parameters: patch_id and varname");
    }

    t_maxmcp* patch = PatchRegistry::find_patch(patch_id);
    if (!patch) {
        return ToolCommon::patch_not_found_error(patch_id);
    }

    auto* deferred_result = new ToolCommon::DeferredResult();
    auto* data = new t_remove_object_data{patch, varname, deferred_result};

    t_atom a;
    atom_setobj(&a, data);
    defer(patch, (method)remove_object_deferred, gensym("remove_object"), 1, &a);

    if (!deferred_result->wait_for(ToolCommon::DEFAULT_DEFER_TIMEOUT)) {
        delete deferred_result;
        return ToolCommon::timeout_error("removing object");
    }

    json result = deferred_result->result;
    delete deferred_result;
    return result;
}

json execute_get_objects_in_patch(const json& params) {
    std::string patch_id = params.value("patch_id", "");

    if (patch_id.empty()) {
        return ToolCommon::missing_param_error("patch_id");
    }

    t_maxmcp* patch = PatchRegistry::find_patch(patch_id);
    if (!patch) {
        return ToolCommon::patch_not_found_error(patch_id);
    }

    auto* deferred_result = new ToolCommon::DeferredResult();
    t_get_objects_data* data = new t_get_objects_data{patch, deferred_result};

    t_atom a;
    atom_setobj(&a, data);
    defer(patch, (method)get_objects_deferred, gensym("get_objects"), 1, &a);

    if (!deferred_result->wait_for(ToolCommon::HEAVY_OPERATION_TIMEOUT)) {
        delete deferred_result;
        return ToolCommon::timeout_error("waiting for patch object list");
    }

    json result = deferred_result->result;
    delete deferred_result;
    return {{"result", result}};
}

json execute_set_object_attribute(const json& params) {
    std::string patch_id = params.value("patch_id", "");
    std::string varname = params.value("varname", "");
    std::string attribute = params.value("attribute", "");

    if (patch_id.empty() || varname.empty() || attribute.empty()) {
        return ToolCommon::make_error(
            ToolCommon::ErrorCode::INVALID_PARAMS,
            "Missing required parameters: patch_id, varname, and attribute");
    }

    if (!params.contains("value")) {
        return ToolCommon::missing_param_error("value");
    }

    json value = params["value"];

    t_maxmcp* patch = PatchRegistry::find_patch(patch_id);
    if (!patch) {
        return ToolCommon::patch_not_found_error(patch_id);
    }

    auto* deferred_result = new ToolCommon::DeferredResult();
    auto* data = new t_set_attribute_data{patch, varname, attribute, value, deferred_result};

    t_atom a;
    atom_setobj(&a, data);
    defer(patch, (method)set_attribute_deferred, gensym("set_attribute"), 1, &a);

    if (!deferred_result->wait_for(ToolCommon::DEFAULT_DEFER_TIMEOUT)) {
        delete deferred_result;
        return ToolCommon::timeout_error("setting attribute");
    }

    json result = deferred_result->result;
    delete deferred_result;
    return result;
}

json execute_get_object_attribute(const json& params) {
    std::string patch_id = params.value("patch_id", "");
    std::string varname = params.value("varname", "");
    std::string attribute = params.value("attribute", "");

    if (patch_id.empty() || varname.empty() || attribute.empty()) {
        return ToolCommon::make_error(
            ToolCommon::ErrorCode::INVALID_PARAMS,
            "Missing required parameters: patch_id, varname, and attribute");
    }

    t_maxmcp* patch = PatchRegistry::find_patch(patch_id);
    if (!patch) {
        return ToolCommon::patch_not_found_error(patch_id);
    }

    auto* deferred_result = new ToolCommon::DeferredResult();
    auto* data = new t_get_attribute_data{patch, varname, attribute, deferred_result};

    t_atom a;
    atom_setobj(&a, data);
    defer(patch, (method)get_attribute_deferred, gensym("get_attribute"), 1, &a);

    if (!deferred_result->wait_for(ToolCommon::DEFAULT_DEFER_TIMEOUT)) {
        delete deferred_result;
        return ToolCommon::timeout_error("getting attribute");
    }

    json result = deferred_result->result;
    delete deferred_result;
    return result;
}

json execute_get_object_io_info(const json& params) {
    std::string patch_id = params.value("patch_id", "");
    std::string varname = params.value("varname", "");

    if (patch_id.empty() || varname.empty()) {
        return ToolCommon::make_error(ToolCommon::ErrorCode::INVALID_PARAMS,
                                      "Missing required parameters: patch_id and varname");
    }

    t_maxmcp* patch = PatchRegistry::find_patch(patch_id);
    if (!patch) {
        return ToolCommon::patch_not_found_error(patch_id);
    }

    auto* deferred_result = new ToolCommon::DeferredResult();
    t_get_io_info_data* data = new t_get_io_info_data{patch, varname, deferred_result};

    t_atom a;
    atom_setobj(&a, data);
    defer(patch, (method)get_io_info_deferred, gensym("get_io_info"), 1, &a);

    if (!deferred_result->wait_for(ToolCommon::DEFAULT_DEFER_TIMEOUT)) {
        delete deferred_result;
        return ToolCommon::timeout_error("getting I/O info");
    }

    json result = deferred_result->result;
    delete deferred_result;

    if (result.contains("error")) {
        return result;
    }
    return {{"result", result}};
}

json execute_get_object_hidden(const json& params) {
    std::string patch_id = params.value("patch_id", "");
    std::string varname = params.value("varname", "");

    if (patch_id.empty() || varname.empty()) {
        return ToolCommon::make_error(ToolCommon::ErrorCode::INVALID_PARAMS,
                                      "Missing required parameters: patch_id and varname");
    }

    t_maxmcp* patch = PatchRegistry::find_patch(patch_id);
    if (!patch) {
        return ToolCommon::patch_not_found_error(patch_id);
    }

    auto* deferred_result = new ToolCommon::DeferredResult();
    t_get_hidden_data* data = new t_get_hidden_data{patch, varname, deferred_result};

    t_atom a;
    atom_setobj(&a, data);
    defer(patch, (method)get_hidden_deferred, gensym("get_hidden"), 1, &a);

    if (!deferred_result->wait_for(ToolCommon::DEFAULT_DEFER_TIMEOUT)) {
        delete deferred_result;
        return ToolCommon::timeout_error("getting hidden state");
    }

    json result = deferred_result->result;
    delete deferred_result;

    if (result.contains("error")) {
        return result;
    }
    return {{"result", result}};
}

json execute_set_object_hidden(const json& params) {
    std::string patch_id = params.value("patch_id", "");
    std::string varname = params.value("varname", "");

    if (patch_id.empty() || varname.empty()) {
        return ToolCommon::make_error(ToolCommon::ErrorCode::INVALID_PARAMS,
                                      "Missing required parameters: patch_id and varname");
    }

    if (!params.contains("hidden") || !params["hidden"].is_boolean()) {
        return ToolCommon::missing_param_error("hidden");
    }

    bool hidden = params["hidden"].get<bool>();

    t_maxmcp* patch = PatchRegistry::find_patch(patch_id);
    if (!patch) {
        return ToolCommon::patch_not_found_error(patch_id);
    }

    auto* deferred_result = new ToolCommon::DeferredResult();
    t_set_hidden_data* data = new t_set_hidden_data{patch, varname, hidden, deferred_result};

    t_atom a;
    atom_setobj(&a, data);
    defer(patch, (method)set_hidden_deferred, gensym("set_hidden"), 1, &a);

    if (!deferred_result->wait_for(ToolCommon::DEFAULT_DEFER_TIMEOUT)) {
        delete deferred_result;
        return ToolCommon::timeout_error("setting hidden state");
    }

    json result = deferred_result->result;
    delete deferred_result;

    if (result.contains("error")) {
        return result;
    }
    return {{"result", result}};
}

json execute_redraw_object(const json& params) {
    std::string patch_id = params.value("patch_id", "");
    std::string varname = params.value("varname", "");

    if (patch_id.empty() || varname.empty()) {
        return ToolCommon::make_error(ToolCommon::ErrorCode::INVALID_PARAMS,
                                      "Missing required parameters: patch_id and varname");
    }

    t_maxmcp* patch = PatchRegistry::find_patch(patch_id);
    if (!patch) {
        return ToolCommon::patch_not_found_error(patch_id);
    }

    auto* deferred_result = new ToolCommon::DeferredResult();
    t_redraw_data* data = new t_redraw_data{patch, varname, deferred_result};

    t_atom a;
    atom_setobj(&a, data);
    defer(patch, (method)redraw_deferred, gensym("redraw"), 1, &a);

    if (!deferred_result->wait_for(ToolCommon::DEFAULT_DEFER_TIMEOUT)) {
        delete deferred_result;
        return ToolCommon::timeout_error("redrawing object");
    }

    json result = deferred_result->result;
    delete deferred_result;

    if (result.contains("error")) {
        return result;
    }
    return {{"result", result}};
}

json execute_replace_object_text(const json& params) {
    std::string patch_id = params.value("patch_id", "");
    std::string varname = params.value("varname", "");
    std::string new_text = params.value("new_text", "");

    if (patch_id.empty() || varname.empty() || new_text.empty()) {
        return ToolCommon::make_error(
            ToolCommon::ErrorCode::INVALID_PARAMS,
            "Missing required parameters: patch_id, varname, and new_text");
    }

    t_maxmcp* patch = PatchRegistry::find_patch(patch_id);
    if (!patch) {
        return ToolCommon::patch_not_found_error(patch_id);
    }

    auto* deferred_result = new ToolCommon::DeferredResult();
    auto* data = new t_replace_text_data{patch, varname, new_text, deferred_result};

    t_atom a;
    atom_setobj(&a, data);
    defer(patch, (method)replace_object_text_deferred, gensym("replace_object_text"), 1, &a);

    if (!deferred_result->wait_for(ToolCommon::DEFAULT_DEFER_TIMEOUT)) {
        delete deferred_result;
        return ToolCommon::timeout_error("replacing object text");
    }

    json result = deferred_result->result;
    delete deferred_result;
    return result;
}

json execute_assign_varnames(const json& params) {
    std::string patch_id = params.value("patch_id", "");

    if (patch_id.empty()) {
        return ToolCommon::missing_param_error("patch_id");
    }

    if (!params.contains("assignments") || !params["assignments"].is_array()) {
        return ToolCommon::missing_param_error("assignments");
    }

    json assignments = params["assignments"];

    if (assignments.empty()) {
        return ToolCommon::make_error(ToolCommon::ErrorCode::INVALID_PARAMS,
                                      "assignments array must not be empty");
    }

    // Validate each assignment and check for duplicate varnames
    std::set<std::string> seen_varnames;
    for (const auto& a : assignments) {
        if (!a.contains("index") || !a["index"].is_number_integer()) {
            return ToolCommon::make_error(ToolCommon::ErrorCode::INVALID_PARAMS,
                                          "Each assignment must have an integer 'index'");
        }
        if (!a.contains("varname") || !a["varname"].is_string() ||
            a["varname"].get<std::string>().empty()) {
            return ToolCommon::make_error(ToolCommon::ErrorCode::INVALID_PARAMS,
                                          "Each assignment must have a non-empty string 'varname'");
        }
        std::string vn = a["varname"].get<std::string>();
        if (seen_varnames.count(vn)) {
            return ToolCommon::make_error(ToolCommon::ErrorCode::INVALID_PARAMS,
                                          "Duplicate varname: " + vn);
        }
        seen_varnames.insert(vn);
    }

    t_maxmcp* patch = PatchRegistry::find_patch(patch_id);
    if (!patch) {
        return ToolCommon::patch_not_found_error(patch_id);
    }

    auto* deferred_result = new ToolCommon::DeferredResult();
    auto* data = new t_assign_varnames_data{patch, assignments, deferred_result};

    t_atom a;
    atom_setobj(&a, data);
    defer(patch, (method)assign_varnames_deferred, gensym("assign_varnames"), 1, &a);

    if (!deferred_result->wait_for(ToolCommon::HEAVY_OPERATION_TIMEOUT)) {
        delete deferred_result;
        return ToolCommon::timeout_error("assigning varnames");
    }

    json result = deferred_result->result;
    delete deferred_result;
    return result;
}

json execute_get_object_value(const json& params) {
    std::string patch_id = params.value("patch_id", "");
    std::string varname = params.value("varname", "");

    if (patch_id.empty() || varname.empty()) {
        return ToolCommon::make_error(ToolCommon::ErrorCode::INVALID_PARAMS,
                                      "Missing required parameters: patch_id and varname");
    }

    t_maxmcp* patch = PatchRegistry::find_patch(patch_id);
    if (!patch) {
        return ToolCommon::patch_not_found_error(patch_id);
    }

    auto* deferred_result = new ToolCommon::DeferredResult();
    auto* data = new t_get_value_data{patch, varname, deferred_result};

    t_atom a;
    atom_setobj(&a, data);
    defer(patch, (method)get_value_deferred, gensym("get_value"), 1, &a);

    if (!deferred_result->wait_for(ToolCommon::DEFAULT_DEFER_TIMEOUT)) {
        delete deferred_result;
        return ToolCommon::timeout_error("getting object value");
    }

    json result = deferred_result->result;
    delete deferred_result;
    return result;
}

#endif  // MAXMCP_TEST_MODE

json execute(const std::string& tool, const json& params) {
#ifdef MAXMCP_TEST_MODE
    if (tool == "add_max_object" || tool == "remove_max_object" || tool == "get_objects_in_patch" ||
        tool == "set_object_attribute" || tool == "get_object_attribute" ||
        tool == "get_object_io_info" || tool == "get_object_hidden" ||
        tool == "set_object_hidden" || tool == "redraw_object" || tool == "replace_object_text" ||
        tool == "assign_varnames" || tool == "get_object_value") {
        return ToolCommon::test_mode_error();
    }
    return nullptr;
#else
    if (tool == "add_max_object") {
        return execute_add_max_object(params);
    } else if (tool == "remove_max_object") {
        return execute_remove_max_object(params);
    } else if (tool == "get_objects_in_patch") {
        return execute_get_objects_in_patch(params);
    } else if (tool == "set_object_attribute") {
        return execute_set_object_attribute(params);
    } else if (tool == "get_object_attribute") {
        return execute_get_object_attribute(params);
    } else if (tool == "get_object_io_info") {
        return execute_get_object_io_info(params);
    } else if (tool == "get_object_hidden") {
        return execute_get_object_hidden(params);
    } else if (tool == "set_object_hidden") {
        return execute_set_object_hidden(params);
    } else if (tool == "redraw_object") {
        return execute_redraw_object(params);
    } else if (tool == "replace_object_text") {
        return execute_replace_object_text(params);
    } else if (tool == "assign_varnames") {
        return execute_assign_varnames(params);
    } else if (tool == "get_object_value") {
        return execute_get_object_value(params);
    }

    return nullptr;
#endif
}

}  // namespace ObjectTools
