/**
    @file state_tools.cpp
    MaxMCP - State MCP Tools Implementation

    @ingroup maxmcp
*/

#ifndef MAXMCP_TEST_MODE
#include "ext.h"

#include "jpatcher_api.h"
#endif

#include "maxmcp.h"
#include "state_tools.h"
#include "tool_common.h"
#include "utils/console_logger.h"
#include "utils/patch_registry.h"

namespace StateTools {

using DeferredResult = ToolCommon::DeferredResult;

// ============================================================================
// Data Structures for Deferred Callbacks
// ============================================================================

struct t_get_lock_state_data {
    t_maxmcp* patch;
    DeferredResult* deferred_result;
};

struct t_set_lock_state_data {
    t_maxmcp* patch;
    bool locked;
    DeferredResult* deferred_result;
};

struct t_get_dirty_data {
    t_maxmcp* patch;
    DeferredResult* deferred_result;
};

// ============================================================================
// Deferred Callbacks
// ============================================================================
// NOTE: Undo tools (begin_undo_group, end_undo_group) were removed because
// jpatcher_beginundo/jpatcher_endundo APIs do not exist in Max SDK.

#ifndef MAXMCP_TEST_MODE

// Deferred callback for getting patch lock state
static void get_lock_state_deferred(t_maxmcp* patch, t_symbol* s, long argc, t_atom* argv) {
    VALIDATE_DEFERRED_ARGS("get_lock_state_deferred");
    EXTRACT_DEFERRED_DATA_WITH_RESULT(t_get_lock_state_data, data, argv);

    // Get lock state using patcherview
    // Note: jpatcher_get_locked exists in header but not in linker, use patcherview instead
    t_object* patcherview = jpatcher_get_firstview(data->patch->patcher);
    char locked = 0;
    bool state_verified = false;

    if (patcherview) {
        locked = patcherview_get_locked(patcherview);
        state_verified = true;
    } else {
        ConsoleLogger::log("Warning: No patcherview available, defaulting lock state to false");
    }

    COMPLETE_DEFERRED(data, (json{{"locked", locked != 0},
                                  {"patch_id", data->patch->patch_id},
                                  {"state_verified", state_verified}}));
}

// Deferred callback for setting patch lock state
static void set_lock_state_deferred(t_maxmcp* patch, t_symbol* s, long argc, t_atom* argv) {
    VALIDATE_DEFERRED_ARGS("set_lock_state_deferred");
    EXTRACT_DEFERRED_DATA_WITH_RESULT(t_set_lock_state_data, data, argv);

    // Set lock state
    jpatcher_set_locked(data->patch->patcher, data->locked ? 1 : 0);

    ConsoleLogger::log(
        ("Patch lock state set to: " + std::string(data->locked ? "locked" : "unlocked")).c_str());

    COMPLETE_DEFERRED(data, (json{{"success", true}, {"locked", data->locked}}));
}

// Deferred callback for getting patch dirty state
static void get_dirty_deferred(t_maxmcp* patch, t_symbol* s, long argc, t_atom* argv) {
    VALIDATE_DEFERRED_ARGS("get_dirty_deferred");
    EXTRACT_DEFERRED_DATA_WITH_RESULT(t_get_dirty_data, data, argv);

    // Get dirty state using jpatcher_get_dirty
    char dirty = jpatcher_get_dirty(data->patch->patcher);

    COMPLETE_DEFERRED(data, (json{{"dirty", dirty != 0}, {"patch_id", data->patch->patch_id}}));
}

#endif  // MAXMCP_TEST_MODE

// ============================================================================
// Tool Schemas
// ============================================================================

json get_tool_schemas() {
    return json::array(
        {// Query tools
         {{"name", "get_patch_lock_state"},
          {"description",
           "Get the lock/edit state of a patch (locked=presentation, unlocked=edit)"},
          {"inputSchema",
           {{"type", "object"},
            {"properties",
             {{"patch_id", {{"type", "string"}, {"description", "Patch ID to query"}}}}},
            {"required", json::array({"patch_id"})}}}},
         {{"name", "get_patch_dirty"},
          {"description", "Check if a patch has unsaved changes"},
          {"inputSchema",
           {{"type", "object"},
            {"properties",
             {{"patch_id", {{"type", "string"}, {"description", "Patch ID to query"}}}}},
            {"required", json::array({"patch_id"})}}}},
         // State mutation tools
         {{"name", "set_patch_lock_state"},
          {"description", "Set the lock/edit state of a patch"},
          {"inputSchema",
           {{"type", "object"},
            {"properties",
             {{"patch_id", {{"type", "string"}, {"description", "Patch ID"}}},
              {"locked",
               {{"type", "boolean"},
                {"description", "true=lock (presentation), false=unlock (edit)"}}}}},
            {"required", json::array({"patch_id", "locked"})}}}}});
}

// ============================================================================
// Tool Executors
// ============================================================================

#ifndef MAXMCP_TEST_MODE

static json execute_get_patch_lock_state(const json& params) {
    std::string patch_id = params.value("patch_id", "");

    if (patch_id.empty()) {
        return ToolCommon::missing_param_error("patch_id");
    }

    t_maxmcp* patch = PatchRegistry::find_patch(patch_id);
    if (!patch) {
        return ToolCommon::patch_not_found_error(patch_id);
    }

    DeferredResult* deferred_result = new DeferredResult();
    t_get_lock_state_data* data = new t_get_lock_state_data{patch, deferred_result};

    t_atom a;
    atom_setobj(&a, data);
    defer(patch, (method)get_lock_state_deferred, gensym("get_lock_state"), 1, &a);

    if (!deferred_result->wait_for(ToolCommon::DEFAULT_DEFER_TIMEOUT)) {
        delete deferred_result;
        return ToolCommon::timeout_error("getting lock state");
    }

    json result = deferred_result->result;
    delete deferred_result;
    return {{"result", result}};
}

static json execute_set_patch_lock_state(const json& params) {
    std::string patch_id = params.value("patch_id", "");

    if (patch_id.empty()) {
        return ToolCommon::missing_param_error("patch_id");
    }

    if (!params.contains("locked") || !params["locked"].is_boolean()) {
        return ToolCommon::missing_param_error("locked");
    }

    bool locked = params["locked"].get<bool>();

    t_maxmcp* patch = PatchRegistry::find_patch(patch_id);
    if (!patch) {
        return ToolCommon::patch_not_found_error(patch_id);
    }

    DeferredResult* deferred_result = new DeferredResult();
    t_set_lock_state_data* data = new t_set_lock_state_data{patch, locked, deferred_result};

    t_atom a;
    atom_setobj(&a, data);
    defer(patch, (method)set_lock_state_deferred, gensym("set_lock_state"), 1, &a);

    if (!deferred_result->wait_for(ToolCommon::DEFAULT_DEFER_TIMEOUT)) {
        delete deferred_result;
        return ToolCommon::timeout_error("setting lock state");
    }

    json result = deferred_result->result;
    delete deferred_result;
    return {{"result", result}};
}

static json execute_get_patch_dirty(const json& params) {
    std::string patch_id = params.value("patch_id", "");

    if (patch_id.empty()) {
        return ToolCommon::missing_param_error("patch_id");
    }

    t_maxmcp* patch = PatchRegistry::find_patch(patch_id);
    if (!patch) {
        return ToolCommon::patch_not_found_error(patch_id);
    }

    DeferredResult* deferred_result = new DeferredResult();
    t_get_dirty_data* data = new t_get_dirty_data{patch, deferred_result};

    t_atom a;
    atom_setobj(&a, data);
    defer(patch, (method)get_dirty_deferred, gensym("get_dirty"), 1, &a);

    if (!deferred_result->wait_for(ToolCommon::DEFAULT_DEFER_TIMEOUT)) {
        delete deferred_result;
        return ToolCommon::timeout_error("getting dirty state");
    }

    json result = deferred_result->result;
    delete deferred_result;
    return {{"result", result}};
}

#endif  // MAXMCP_TEST_MODE

// ============================================================================
// Tool Dispatcher
// ============================================================================

json execute(const std::string& tool, const json& params) {
#ifdef MAXMCP_TEST_MODE
    // In test mode, return explicit error for known tools (consistent with other modules)
    if (tool == "get_patch_lock_state" || tool == "set_patch_lock_state" ||
        tool == "get_patch_dirty") {
        return ToolCommon::test_mode_error();
    }
#else
    if (tool == "get_patch_lock_state") {
        return execute_get_patch_lock_state(params);
    } else if (tool == "set_patch_lock_state") {
        return execute_set_patch_lock_state(params);
    } else if (tool == "get_patch_dirty") {
        return execute_get_patch_dirty(params);
    }
#endif

    // Tool not handled by this module - return nullptr to signal routing should continue
    return nullptr;
}

}  // namespace StateTools
