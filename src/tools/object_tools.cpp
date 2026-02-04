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
};

struct t_remove_object_data {
    t_maxmcp* patch;
    std::string varname;
};

struct t_set_attribute_data {
    t_maxmcp* patch;
    std::string varname;
    std::string attribute;
    json value;
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

// ============================================================================
// Deferred Callbacks (run on main thread)
// ============================================================================

#ifndef MAXMCP_TEST_MODE

static void add_object_deferred(t_maxmcp* patch, t_symbol* s, long argc, t_atom* argv) {
    VALIDATE_DEFERRED_ARGS("add_object_deferred");
    EXTRACT_DEFERRED_DATA(t_add_object_data, data, argv);

    // Build object string with arguments
    std::string obj_string = data->obj_type;

    if (!data->arguments.is_null() && data->arguments.is_array()) {
        for (const auto& arg : data->arguments) {
            obj_string += " ";
            if (arg.is_number()) {
                obj_string += std::to_string(arg.get<double>());
            } else if (arg.is_string()) {
                obj_string += arg.get<std::string>();
            }
        }
    }

    // Escape special characters
    std::string escaped = obj_string;
    size_t pos = 0;

    // Escape backslash first
    while ((pos = escaped.find('\\', pos)) != std::string::npos) {
        escaped.replace(pos, 1, "\\\\");
        pos += 2;
    }

    // Escape quotes
    pos = 0;
    while ((pos = escaped.find('"', pos)) != std::string::npos) {
        escaped.replace(pos, 1, "\\\"");
        pos += 2;
    }

    // Escape percent signs
    pos = 0;
    while ((pos = escaped.find('%', pos)) != std::string::npos) {
        escaped.replace(pos, 1, "%%");
        pos += 2;
    }

    // Replace newlines with space
    pos = 0;
    while ((pos = escaped.find('\n', pos)) != std::string::npos) {
        escaped.replace(pos, 1, " ");
        pos += 1;
    }
    pos = 0;
    while ((pos = escaped.find('\r', pos)) != std::string::npos) {
        escaped.erase(pos, 1);
    }

    // Create object
    t_object* obj = (t_object*)newobject_sprintf(
        data->patch->patcher, "@maxclass newobj @text \"%s\" @patching_position %.2f %.2f",
        escaped.c_str(), data->x, data->y);

    if (obj) {
        // Set varname if provided
        if (!data->varname.empty()) {
            object_attr_setsym(obj, gensym("varname"), gensym(data->varname.c_str()));
        }

        // Set attributes if provided
        if (!data->attributes.is_null() && data->attributes.is_object()) {
            for (auto& [attr_name, attr_value] : data->attributes.items()) {
                if (attr_value.is_array() && attr_value.size() == 4) {
                    // Color array [r, g, b, a]
                    t_atom color_atoms[4];
                    for (size_t i = 0; i < 4; i++) {
                        atom_setfloat(&color_atoms[i], attr_value[i].get<double>());
                    }
                    object_attr_setvalueof(obj, gensym(attr_name.c_str()), 4, color_atoms);
                } else if (attr_value.is_number()) {
                    double val = attr_value.get<double>();
                    t_atom a;
                    atom_setfloat(&a, val);
                    object_attr_setvalueof(obj, gensym(attr_name.c_str()), 1, &a);
                } else if (attr_value.is_string()) {
                    std::string val = attr_value.get<std::string>();
                    t_atom a;
                    atom_setsym(&a, gensym(val.c_str()));
                    object_attr_setvalueof(obj, gensym(attr_name.c_str()), 1, &a);
                }
            }
        }

        object_method(obj, gensym("bringtofront"));
        object_attr_setlong(obj, gensym("presentation"), 1);
        ConsoleLogger::log(("Object created: " + obj_string).c_str());
    } else {
        ConsoleLogger::log(("Failed to create object: " + obj_string).c_str());
    }

    delete data;
}

static void remove_object_deferred(t_maxmcp* patch, t_symbol* s, long argc, t_atom* argv) {
    VALIDATE_DEFERRED_ARGS("remove_object_deferred");
    EXTRACT_DEFERRED_DATA(t_remove_object_data, data, argv);

    t_object* box = PatchHelpers::find_box_by_varname(data->patch->patcher, data->varname);

    if (box) {
        object_free(box);
        ConsoleLogger::log(("Object removed: " + data->varname).c_str());
    } else {
        ConsoleLogger::log(("Object not found: " + data->varname).c_str());
    }

    delete data;
}

static void set_attribute_deferred(t_maxmcp* patch, t_symbol* s, long argc, t_atom* argv) {
    VALIDATE_DEFERRED_ARGS("set_attribute_deferred");
    EXTRACT_DEFERRED_DATA(t_set_attribute_data, data, argv);

    t_object* box = PatchHelpers::find_box_by_varname(data->patch->patcher, data->varname);

    if (!box) {
        ConsoleLogger::log(("Object not found: " + data->varname).c_str());
        delete data;
        return;
    }

    t_symbol* attr_sym = gensym(data->attribute.c_str());

    if (data->value.is_number()) {
        double val = data->value.get<double>();
        if (val == (long)val) {
            object_attr_setlong(box, attr_sym, (long)val);
        } else {
            object_attr_setfloat(box, attr_sym, val);
        }
        ConsoleLogger::log(("Attribute set: " + data->varname + "." + data->attribute).c_str());
    } else if (data->value.is_string()) {
        std::string str_val = data->value.get<std::string>();
        object_attr_setsym(box, attr_sym, gensym(str_val.c_str()));
        ConsoleLogger::log(("Attribute set: " + data->varname + "." + data->attribute).c_str());
    } else {
        ConsoleLogger::log(("Unsupported value type for attribute: " + data->attribute).c_str());
    }

    delete data;
}

static void get_objects_deferred(t_maxmcp* patch, t_symbol* s, long argc, t_atom* argv) {
    VALIDATE_DEFERRED_ARGS("get_objects_deferred");
    EXTRACT_DEFERRED_DATA_WITH_RESULT(t_get_objects_data, data, argv);

    json objects = json::array();
    t_object* patcher = data->patch->patcher;

    for (t_object* box = jpatcher_get_firstobject(patcher); box; box = jbox_get_nextobject(box)) {
        t_symbol* varname = object_attr_getsym(box, gensym("varname"));
        std::string varname_str = (varname && varname->s_name) ? varname->s_name : "";

        t_symbol* maxclass = jbox_get_maxclass(box);
        std::string maxclass_str = (maxclass && maxclass->s_name) ? maxclass->s_name : "unknown";

        t_rect rect;
        jbox_get_patching_rect(box, &rect);

        json obj_info = {{"maxclass", maxclass_str},
                         {"position", json::array({rect.x, rect.y})},
                         {"size", json::array({rect.width, rect.height})}};

        if (!varname_str.empty()) {
            obj_info["varname"] = varname_str;
        }

        objects.push_back(obj_info);
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

#endif  // MAXMCP_TEST_MODE

// ============================================================================
// Tool Schemas
// ============================================================================

json get_tool_schemas() {
    return json::array(
        {{{"name", "add_max_object"},
          {"description", "Add a Max object to a patch"},
          {"inputSchema",
           {{"type", "object"},
            {"properties",
             {{"patch_id", {{"type", "string"}, {"description", "Patch ID to add object to"}}},
              {"obj_type",
               {{"type", "string"},
                {"description", "Max object type (e.g., 'number', 'button', 'dac~')"}}},
              {"position",
               {{"type", "array"},
                {"items", {{"type", "number"}}},
                {"description", "Position [x, y] in patch"}}},
              {"varname",
               {{"type", "string"}, {"description", "Variable name for the object (optional)"}}},
              {"arguments",
               {{"type", "array"}, {"description", "Object arguments (e.g., [440] for 'cycle~')"}}},
              {"attributes",
               {{"type", "object"},
                {"description",
                 "Object attributes (e.g., {\"bgcolor\": [1.0, 0.5, 0.0, 1.0]})"}}}}},
            {"required", json::array({"patch_id", "obj_type", "position"})}}}},

         {{"name", "remove_max_object"},
          {"description", "Remove a Max object from a patch by varname"},
          {"inputSchema",
           {{"type", "object"},
            {"properties",
             {{"patch_id", {{"type", "string"}, {"description", "Patch ID containing the object"}}},
              {"varname",
               {{"type", "string"}, {"description", "Variable name of the object to remove"}}}}},
            {"required", json::array({"patch_id", "varname"})}}}},

         {{"name", "get_objects_in_patch"},
          {"description", "List all objects in a patch with metadata"},
          {"inputSchema",
           {{"type", "object"},
            {"properties",
             {{"patch_id", {{"type", "string"}, {"description", "Patch ID to query"}}}}},
            {"required", json::array({"patch_id"})}}}},

         {{"name", "set_object_attribute"},
          {"description", "Set an attribute of a Max object"},
          {"inputSchema",
           {{"type", "object"},
            {"properties",
             {{"patch_id", {{"type", "string"}, {"description", "Patch ID containing the object"}}},
              {"varname", {{"type", "string"}, {"description", "Variable name of the object"}}},
              {"attribute", {{"type", "string"}, {"description", "Attribute name to set"}}},
              {"value", {{"description", "Attribute value (number, string, or array)"}}}}},
            {"required", json::array({"patch_id", "varname", "attribute", "value"})}}}},

         {{"name", "get_object_io_info"},
          {"description", "Get inlet and outlet count for an object"},
          {"inputSchema",
           {{"type", "object"},
            {"properties",
             {{"patch_id", {{"type", "string"}, {"description", "Patch ID containing the object"}}},
              {"varname", {{"type", "string"}, {"description", "Variable name of the object"}}}}},
            {"required", json::array({"patch_id", "varname"})}}}},

         {{"name", "get_object_hidden"},
          {"description", "Check if an object is hidden"},
          {"inputSchema",
           {{"type", "object"},
            {"properties",
             {{"patch_id", {{"type", "string"}, {"description", "Patch ID"}}},
              {"varname", {{"type", "string"}, {"description", "Variable name of the object"}}}}},
            {"required", json::array({"patch_id", "varname"})}}}},

         {{"name", "set_object_hidden"},
          {"description", "Set the visibility of an object in a patch"},
          {"inputSchema",
           {{"type", "object"},
            {"properties",
             {{"patch_id", {{"type", "string"}, {"description", "Patch ID"}}},
              {"varname", {{"type", "string"}, {"description", "Variable name of the object"}}},
              {"hidden", {{"type", "boolean"}, {"description", "true=hide, false=show"}}}}},
            {"required", json::array({"patch_id", "varname", "hidden"})}}}},

         {{"name", "redraw_object"},
          {"description", "Force redraw of a specific object"},
          {"inputSchema",
           {{"type", "object"},
            {"properties",
             {{"patch_id", {{"type", "string"}, {"description", "Patch ID"}}},
              {"varname", {{"type", "string"}, {"description", "Variable name of the object"}}}}},
            {"required", json::array({"patch_id", "varname"})}}}}});
}

// ============================================================================
// Tool Execution
// ============================================================================

json execute(const std::string& tool, const json& params) {
#ifdef MAXMCP_TEST_MODE
    return ToolCommon::make_error(-32603, "Tool execution not available in test mode");
#else
    if (tool == "add_max_object") {
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
                    return ToolCommon::make_error(-32602, "Invalid attributes JSON: " +
                                                              std::string(e.what()));
                }
            } else if (params["attributes"].is_object()) {
                attributes = params["attributes"];
            }
        }

        if (patch_id.empty() || obj_type.empty()) {
            return ToolCommon::make_error(-32602,
                                          "Missing required parameters: patch_id and obj_type");
        }

        if (!params.contains("position") || !params["position"].is_array() ||
            params["position"].size() < 2) {
            return ToolCommon::make_error(-32602,
                                          "Invalid position parameter: must be array [x, y]");
        }

        double x = params["position"][0].get<double>();
        double y = params["position"][1].get<double>();

        t_maxmcp* patch = PatchRegistry::find_patch(patch_id);
        if (!patch) {
            return ToolCommon::patch_not_found_error(patch_id);
        }

        t_add_object_data* data =
            new t_add_object_data{patch, obj_type, x, y, varname, arguments, attributes};

        t_atom a;
        atom_setobj(&a, data);
        defer(patch, (method)add_object_deferred, gensym("add_object"), 1, &a);

        return {{"result",
                 {{"status", "success"},
                  {"patch_id", patch_id},
                  {"obj_type", obj_type},
                  {"position", json::array({x, y})},
                  {"arguments", arguments}}}};

    } else if (tool == "remove_max_object") {
        std::string patch_id = params.value("patch_id", "");
        std::string varname = params.value("varname", "");

        if (patch_id.empty() || varname.empty()) {
            return ToolCommon::make_error(-32602,
                                          "Missing required parameters: patch_id and varname");
        }

        t_maxmcp* patch = PatchRegistry::find_patch(patch_id);
        if (!patch) {
            return ToolCommon::patch_not_found_error(patch_id);
        }

        t_remove_object_data* data = new t_remove_object_data{patch, varname};

        t_atom a;
        atom_setobj(&a, data);
        defer(patch, (method)remove_object_deferred, gensym("remove_object"), 1, &a);

        return {{"result", {{"status", "success"}, {"patch_id", patch_id}, {"varname", varname}}}};

    } else if (tool == "get_objects_in_patch") {
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

        if (!deferred_result->wait_for(ToolCommon::DEFAULT_DEFER_TIMEOUT)) {
            delete deferred_result;
            return ToolCommon::timeout_error("waiting for patch object list");
        }

        json result = deferred_result->result;
        delete deferred_result;
        return {{"result", result}};

    } else if (tool == "set_object_attribute") {
        std::string patch_id = params.value("patch_id", "");
        std::string varname = params.value("varname", "");
        std::string attribute = params.value("attribute", "");

        if (patch_id.empty() || varname.empty() || attribute.empty()) {
            return ToolCommon::make_error(
                -32602, "Missing required parameters: patch_id, varname, and attribute");
        }

        if (!params.contains("value")) {
            return ToolCommon::missing_param_error("value");
        }

        json value = params["value"];

        t_maxmcp* patch = PatchRegistry::find_patch(patch_id);
        if (!patch) {
            return ToolCommon::patch_not_found_error(patch_id);
        }

        t_set_attribute_data* data = new t_set_attribute_data{patch, varname, attribute, value};

        t_atom a;
        atom_setobj(&a, data);
        defer(patch, (method)set_attribute_deferred, gensym("set_attribute"), 1, &a);

        return {{"result",
                 {{"status", "success"},
                  {"patch_id", patch_id},
                  {"varname", varname},
                  {"attribute", attribute}}}};

    } else if (tool == "get_object_io_info") {
        std::string patch_id = params.value("patch_id", "");
        std::string varname = params.value("varname", "");

        if (patch_id.empty() || varname.empty()) {
            return ToolCommon::make_error(-32602,
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

    } else if (tool == "get_object_hidden") {
        std::string patch_id = params.value("patch_id", "");
        std::string varname = params.value("varname", "");

        if (patch_id.empty() || varname.empty()) {
            return ToolCommon::make_error(-32602,
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

    } else if (tool == "set_object_hidden") {
        std::string patch_id = params.value("patch_id", "");
        std::string varname = params.value("varname", "");

        if (patch_id.empty() || varname.empty()) {
            return ToolCommon::make_error(-32602,
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

    } else if (tool == "redraw_object") {
        std::string patch_id = params.value("patch_id", "");
        std::string varname = params.value("varname", "");

        if (patch_id.empty() || varname.empty()) {
            return ToolCommon::make_error(-32602,
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

    // Tool not handled by this module - return nullptr to signal routing should continue
    return nullptr;
#endif
}

}  // namespace ObjectTools
