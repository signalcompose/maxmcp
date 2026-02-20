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

#endif  // MAXMCP_TEST_MODE

}  // namespace PatchHelpers
