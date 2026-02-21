/**
    @file patch_helpers.cpp
    MaxMCP - Patch Helper Functions Implementation

    @ingroup maxmcp
*/

#include "patch_helpers.h"

#ifndef MAXMCP_TEST_MODE
#include "jpatcher_api.h"
#endif

namespace PatchHelpers {

// Max SDK independent - shared between test and production

bool is_textfield_content_type(const std::string& obj_type) {
    return kTextfieldContentTypes.count(obj_type) > 0;
}

std::string build_text_from_arguments(const nlohmann::json& arguments) {
    if (arguments.is_null() || !arguments.is_array() || arguments.empty()) {
        return "";
    }

    std::string text;
    for (size_t i = 0; i < arguments.size(); ++i) {
        if (i > 0)
            text += " ";
        const auto& arg = arguments[i];
        if (arg.is_string())
            text += arg.get<std::string>();
        else if (arg.is_number_integer())
            text += std::to_string(arg.get<int>());
        else if (arg.is_number_float())
            text += std::to_string(arg.get<double>());
    }
    return text;
}

#ifdef MAXMCP_TEST_MODE

// Test mode stubs - return nullptr/error values
t_object* find_box_by_varname(t_object* patcher, const std::string& varname) {
    (void)patcher;
    (void)varname;
    return nullptr;
}

long get_inlet_count(t_object* box) {
    (void)box;
    return -1;
}

long get_outlet_count(t_object* box) {
    (void)box;
    return -1;
}

std::vector<t_atom> json_to_atoms(const nlohmann::json& value) {
    return {};
}

bool set_box_attribute(t_object* box, const std::string& attr_name, const nlohmann::json& value) {
    (void)box;
    (void)attr_name;
    (void)value;
    return false;
}

bool set_textfield_content(t_object* box, const std::string& text) {
    (void)box;
    (void)text;
    return false;
}

std::string get_box_text(t_object* box) {
    (void)box;
    return "";
}

std::vector<SavedAttribute> save_box_attributes(t_object* box) {
    (void)box;
    return {};
}

std::vector<SavedConnection> save_box_connections(t_object* patcher, t_object* box) {
    (void)patcher;
    (void)box;
    return {};
}

void restore_box_attributes(t_object* new_box, const std::vector<SavedAttribute>& attrs) {
    (void)new_box;
    (void)attrs;
}

long restore_box_connections(t_object* patcher, t_object* new_box,
                             const std::vector<SavedConnection>& connections) {
    (void)patcher;
    (void)new_box;
    (void)connections;
    return 0;
}

#else

t_object* find_box_by_varname(t_object* patcher, const std::string& varname) {
    if (!patcher || varname.empty()) {
        return nullptr;
    }

    for (t_object* box = jpatcher_get_firstobject(patcher); box != nullptr;
         box = jbox_get_nextobject(box)) {
        t_symbol* box_varname = object_attr_getsym(box, gensym("varname"));
        if (box_varname && box_varname->s_name && varname == box_varname->s_name) {
            return box;
        }
    }

    return nullptr;
}

long get_inlet_count(t_object* box) {
    if (!box) {
        return -1;
    }
    // Get inlet count via attribute (jbox_get_numins doesn't exist in current SDK)
    return object_attr_getlong(box, gensym("numinlets"));
}

long get_outlet_count(t_object* box) {
    if (!box) {
        return -1;
    }
    // Get outlet count via attribute (jbox_get_numouts doesn't exist in current SDK)
    return object_attr_getlong(box, gensym("numoutlets"));
}

static void json_value_to_atom(t_atom& atom, const nlohmann::json& value) {
    if (value.is_number_integer())
        atom_setlong(&atom, value.get<long>());
    else if (value.is_number_float())
        atom_setfloat(&atom, value.get<double>());
    else if (value.is_string())
        atom_setsym(&atom, gensym(value.get<std::string>().c_str()));
}

std::vector<t_atom> json_to_atoms(const nlohmann::json& value) {
    if (value.is_array()) {
        std::vector<t_atom> atoms(value.size());
        for (size_t i = 0; i < value.size(); ++i) {
            json_value_to_atom(atoms[i], value[i]);
        }
        return atoms;
    }

    if (value.is_number() || value.is_string()) {
        std::vector<t_atom> atoms(1);
        json_value_to_atom(atoms[0], value);
        return atoms;
    }

    return {};
}

bool set_box_attribute(t_object* box, const std::string& attr_name, const nlohmann::json& value) {
    if (!box) {
        return false;
    }

    auto atoms = json_to_atoms(value);
    if (atoms.empty()) {
        return false;
    }

    object_attr_setvalueof(box, gensym(attr_name.c_str()), static_cast<long>(atoms.size()),
                           atoms.data());
    return true;
}

bool set_textfield_content(t_object* box, const std::string& text) {
    if (!box || text.empty()) {
        return false;
    }

    t_object* textfield = jbox_get_textfield(box);
    if (!textfield) {
        return false;
    }

    object_method(textfield, gensym("settext"), text.c_str());
    return true;
}

std::string get_box_text(t_object* box) {
    if (!box)
        return "";

    t_object* textfield = jbox_get_textfield(box);
    if (!textfield)
        return "";

    const char* text = nullptr;
    object_method(textfield, gensym("gettext"), &text);
    return text ? std::string(text) : "";
}

std::vector<SavedAttribute> save_box_attributes(t_object* box) {
    std::vector<SavedAttribute> saved;
    if (!box)
        return saved;

    long attr_count = 0;
    t_symbol** attr_names = nullptr;
    if (object_attr_getnames(box, &attr_count, &attr_names) != MAX_ERR_NONE)
        return saved;

    for (long i = 0; i < attr_count; i++) {
        t_object* attr_obj = (t_object*)object_attr_get(box, attr_names[i]);
        if (!attr_obj)
            continue;

        // Only save writable attributes
        if (!object_attr_usercanset(box, attr_names[i]))
            continue;

        // Save if: has save=1 meta OR is in visual whitelist
        bool has_save_meta = (object_attr_getlong(attr_obj, gensym("save")) == 1);
        std::string attr_name = attr_names[i]->s_name ? attr_names[i]->s_name : "";
        bool is_whitelisted = kBoxAttributeWhitelist.count(attr_name) > 0;
        if (!has_save_meta && !is_whitelisted)
            continue;

        long ac = 0;
        t_atom* av = nullptr;
        if (object_attr_getvalueof(box, attr_names[i], &ac, &av) == MAX_ERR_NONE && ac > 0) {
            saved.push_back({attr_names[i], std::vector<t_atom>(av, av + ac)});
            sysmem_freeptr(av);
        }
    }
    sysmem_freeptr(attr_names);

    return saved;
}

std::vector<SavedConnection> save_box_connections(t_object* patcher, t_object* box) {
    std::vector<SavedConnection> saved;
    if (!patcher || !box)
        return saved;

    for (t_object* line = jpatcher_get_firstline(patcher); line;
         line = jpatchline_get_nextline(line)) {
        t_object* box1 = (t_object*)jpatchline_get_box1(line);
        t_object* box2 = (t_object*)jpatchline_get_box2(line);

        if (box1 != box && box2 != box)
            continue;

        SavedConnection conn;
        conn.outlet = jpatchline_get_outletnum(line);
        conn.inlet = jpatchline_get_inletnum(line);
        conn.hidden = jpatchline_get_hidden(line);
        conn.target_is_src = (box1 == box);
        conn.other_box = conn.target_is_src ? box2 : box1;

        conn.color = {0.0, 0.0, 0.0, 1.0};
        jpatchline_get_color(line, &conn.color);

        long num_midpoints = jpatchline_get_nummidpoints(line);
        if (num_midpoints > 0) {
            long num_values = num_midpoints * 2;
            conn.midpoints.resize(num_values, 0.0);
            object_attr_getdouble_array(line, gensym("midpoints"), num_values,
                                        conn.midpoints.data());
        }

        saved.push_back(std::move(conn));
    }

    return saved;
}

void restore_box_attributes(t_object* new_box, const std::vector<SavedAttribute>& attrs) {
    if (!new_box)
        return;

    for (const auto& sa : attrs) {
        std::vector<t_atom> values_copy(sa.values);
        object_attr_setvalueof(new_box, sa.name, (long)values_copy.size(), values_copy.data());
    }
}

long restore_box_connections(t_object* patcher, t_object* new_box,
                             const std::vector<SavedConnection>& connections) {
    if (!patcher || !new_box)
        return 0;

    long reconnected = 0;
    for (const auto& conn : connections) {
        t_object* src_box = conn.target_is_src ? new_box : conn.other_box;
        t_object* dst_box = conn.target_is_src ? conn.other_box : new_box;

        t_atom connect_args[4];
        atom_setobj(&connect_args[0], src_box);
        atom_setlong(&connect_args[1], conn.outlet);
        atom_setobj(&connect_args[2], dst_box);
        atom_setlong(&connect_args[3], conn.inlet);

        t_atom result;
        t_object* new_line =
            (t_object*)object_method_typed(patcher, gensym("connect"), 4, connect_args, &result);

        if (new_line) {
            jpatchline_set_hidden(new_line, conn.hidden);
            t_jrgba color_copy = conn.color;
            jpatchline_set_color(new_line, &color_copy);
            if (!conn.midpoints.empty()) {
                std::vector<double> midpts_copy(conn.midpoints);
                object_attr_setdouble_array(new_line, gensym("midpoints"), (long)midpts_copy.size(),
                                            midpts_copy.data());
            }
        }
        reconnected++;
    }

    return reconnected;
}

#endif  // MAXMCP_TEST_MODE

}  // namespace PatchHelpers
