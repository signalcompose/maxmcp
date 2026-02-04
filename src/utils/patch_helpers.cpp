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

}  // namespace PatchHelpers
