/**
    @file hierarchy_tools.cpp
    MaxMCP - Patcher Hierarchy MCP Tools Implementation

    @ingroup maxmcp
*/

#include "hierarchy_tools.h"

#include "tool_common.h"

#ifndef MAXMCP_TEST_MODE
#include "ext.h"

#include "jpatcher_api.h"
#endif

#include "maxmcp.h"
#include "utils/console_logger.h"
#include "utils/patch_registry.h"

namespace HierarchyTools {

using ToolCommon::DeferredResult;

// ============================================================================
// Data Structures for Deferred Callbacks
// ============================================================================

struct t_get_parent_data {
    t_maxmcp* patch;
    DeferredResult* deferred_result;
};

struct t_get_subpatchers_data {
    t_maxmcp* patch;
    DeferredResult* deferred_result;
};

// ============================================================================
// Deferred Callbacks (executed on Max main thread)
// ============================================================================

#ifndef MAXMCP_TEST_MODE

// Defer callback for getting parent patcher
static void get_parent_deferred(t_maxmcp* patch, t_symbol* s, long argc, t_atom* argv) {
    VALIDATE_DEFERRED_ARGS("get_parent_deferred");
    EXTRACT_DEFERRED_DATA_WITH_RESULT(t_get_parent_data, data, argv);

    // Get parent patcher
    t_object* parent = jpatcher_get_parentpatcher(data->patch->patcher);

    if (!parent) {
        COMPLETE_DEFERRED(data, ToolCommon::make_error(ToolCommon::ErrorCode::INVALID_PARAMS,
                                                       "No parent patcher (top-level patch)"));
        return;
    }

    // Get parent name
    t_symbol* parent_name = jpatcher_get_name(parent);
    std::string name_str = (parent_name && parent_name->s_name) ? parent_name->s_name : "untitled";

    // Note: We don't have direct access to find patch by patcher pointer
    // Return the parent's name instead
    COMPLETE_DEFERRED(data, (json{{"parent_name", name_str}, {"has_parent", true}}));
}

// Defer callback for getting subpatchers
static void get_subpatchers_deferred(t_maxmcp* patch, t_symbol* s, long argc, t_atom* argv) {
    VALIDATE_DEFERRED_ARGS("get_subpatchers_deferred");
    EXTRACT_DEFERRED_DATA_WITH_RESULT(t_get_subpatchers_data, data, argv);

    json subpatchers = json::array();

    // Iterate through all boxes to find subpatchers (patcher, bpatcher, poly~)
    for (t_object* box = jpatcher_get_firstobject(data->patch->patcher); box != nullptr;
         box = jbox_get_nextobject(box)) {
        t_symbol* maxclass = jbox_get_maxclass(box);
        if (!maxclass || !maxclass->s_name) {
            continue;
        }

        std::string class_name = maxclass->s_name;

        // Check if this is a subpatcher type
        // Max may report "jpatcher" (jbox variant) instead of "patcher"
        if (class_name == "patcher" || class_name == "jpatcher" || class_name == "bpatcher" ||
            class_name == "poly~") {
            t_symbol* varname = object_attr_getsym(box, gensym("varname"));
            std::string varname_str = (varname && varname->s_name) ? varname->s_name : "";

            // Get subpatcher name if available
            std::string name_str = "";
            t_object* subpatcher = nullptr;  // Explicitly initialized

            if (class_name == "patcher" || class_name == "bpatcher") {
                // Try to get the subpatcher object (subpatcher may remain null if method fails)
                object_method(box, gensym("subpatcher"), &subpatcher);
                if (subpatcher) {
                    t_symbol* sub_name = jpatcher_get_name(subpatcher);
                    name_str = (sub_name && sub_name->s_name) ? sub_name->s_name : "";
                } else {
                    ConsoleLogger::log(
                        ("Warning: Could not get subpatcher for " + varname_str).c_str());
                }
            }

            json sub_info = {{"type", class_name}};

            if (!varname_str.empty()) {
                sub_info["varname"] = varname_str;
            }
            if (!name_str.empty()) {
                sub_info["name"] = name_str;
            }

            subpatchers.push_back(sub_info);
        }
    }

    COMPLETE_DEFERRED(data, (json{{"subpatchers", subpatchers}, {"count", subpatchers.size()}}));
}

#endif  // MAXMCP_TEST_MODE

// ============================================================================
// Tool Schemas
// ============================================================================

json get_tool_schemas() {
    return json::array(
        {// get_parent_patcher
         {{"name", "get_parent_patcher"},
          {"description", "Get the parent patcher of a subpatcher"},
          {"inputSchema",
           {{"type", "object"},
            {"properties",
             {{"patch_id", {{"type", "string"}, {"description", "Child patch ID to query"}}}}},
            {"required", json::array({"patch_id"})}}}},

         // get_subpatchers
         {{"name", "get_subpatchers"},
          {"description", "List all subpatchers in a patch"},
          {"inputSchema",
           {{"type", "object"},
            {"properties",
             {{"patch_id", {{"type", "string"}, {"description", "Parent patch ID to query"}}}}},
            {"required", json::array({"patch_id"})}}}}});
}

// ============================================================================
// Tool Execution
// ============================================================================

json execute(const std::string& tool, const json& params) {
#ifdef MAXMCP_TEST_MODE
    // Test mode: return mock results
    if (tool == "get_parent_patcher" || tool == "get_subpatchers") {
        return ToolCommon::test_mode_error();
    }
    return nullptr;
#else

    if (tool == "get_parent_patcher") {
        std::string patch_id = params.value("patch_id", "");

        if (patch_id.empty()) {
            return ToolCommon::missing_param_error("patch_id");
        }

        t_maxmcp* patch = PatchRegistry::find_patch(patch_id);
        if (!patch) {
            return ToolCommon::patch_not_found_error(patch_id);
        }

        DeferredResult* deferred_result = new DeferredResult();
        t_get_parent_data* data = new t_get_parent_data{patch, deferred_result};

        t_atom a;
        atom_setobj(&a, data);
        defer(patch, (method)get_parent_deferred, gensym("get_parent"), 1, &a);

        if (!deferred_result->wait_for(ToolCommon::DEFAULT_DEFER_TIMEOUT)) {
            delete deferred_result;
            return ToolCommon::timeout_error("getting parent patcher");
        }

        json result = deferred_result->result;
        delete deferred_result;

        if (result.contains("error")) {
            return result;
        }
        return {{"result", result}};

    } else if (tool == "get_subpatchers") {
        std::string patch_id = params.value("patch_id", "");

        if (patch_id.empty()) {
            return ToolCommon::missing_param_error("patch_id");
        }

        t_maxmcp* patch = PatchRegistry::find_patch(patch_id);
        if (!patch) {
            return ToolCommon::patch_not_found_error(patch_id);
        }

        DeferredResult* deferred_result = new DeferredResult();
        t_get_subpatchers_data* data = new t_get_subpatchers_data{patch, deferred_result};

        t_atom a;
        atom_setobj(&a, data);
        defer(patch, (method)get_subpatchers_deferred, gensym("get_subpatchers"), 1, &a);

        if (!deferred_result->wait_for(ToolCommon::HEAVY_OPERATION_TIMEOUT)) {
            delete deferred_result;
            return ToolCommon::timeout_error("getting subpatchers");
        }

        json result = deferred_result->result;
        delete deferred_result;
        return {{"result", result}};
    }

    // Unknown tool - return nullptr to signal not handled
    return nullptr;

#endif  // MAXMCP_TEST_MODE
}

}  // namespace HierarchyTools
