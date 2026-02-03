/**
    @file state_tools.cpp
    MaxMCP - State and Undo MCP Tools Implementation

    @ingroup maxmcp
*/

#ifndef MAXMCP_TEST_MODE
#include "ext.h"
#include "jpatcher_api.h"
#endif

#include "tools/state_tools.h"
#include "tools/tool_common.h"

#include "maxmcp.h"
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

struct t_begin_undo_data {
    t_maxmcp* patch;
    std::string undo_name;
    DeferredResult* deferred_result;
};

struct t_end_undo_data {
    t_maxmcp* patch;
    DeferredResult* deferred_result;
};

// ============================================================================
// Deferred Callbacks
// ============================================================================

#ifndef MAXMCP_TEST_MODE

// Deferred callback for getting patch lock state
static void get_lock_state_deferred(t_maxmcp* patch, t_symbol* s, long argc, t_atom* argv) {
    VALIDATE_DEFERRED_ARGS("get_lock_state_deferred");
    EXTRACT_DEFERRED_DATA_WITH_RESULT(t_get_lock_state_data, data, argv);

    // Get lock state using jpatcher_get_locked
    long locked = jpatcher_get_locked(data->patch->patcher);

    COMPLETE_DEFERRED(data, json({{"locked", locked != 0}, {"patch_id", data->patch->patch_id}}));
}

// Deferred callback for setting patch lock state
static void set_lock_state_deferred(t_maxmcp* patch, t_symbol* s, long argc, t_atom* argv) {
    VALIDATE_DEFERRED_ARGS("set_lock_state_deferred");
    EXTRACT_DEFERRED_DATA_WITH_RESULT(t_set_lock_state_data, data, argv);

    // Set lock state
    jpatcher_set_locked(data->patch->patcher, data->locked ? 1 : 0);

    ConsoleLogger::log(
        ("Patch lock state set to: " + std::string(data->locked ? "locked" : "unlocked")).c_str());

    COMPLETE_DEFERRED(data, json({{"success", true}, {"locked", data->locked}}));
}

// Deferred callback for getting patch dirty state
static void get_dirty_deferred(t_maxmcp* patch, t_symbol* s, long argc, t_atom* argv) {
    VALIDATE_DEFERRED_ARGS("get_dirty_deferred");
    EXTRACT_DEFERRED_DATA_WITH_RESULT(t_get_dirty_data, data, argv);

    // Get dirty state using jpatcher_get_dirty
    char dirty = jpatcher_get_dirty(data->patch->patcher);

    COMPLETE_DEFERRED(data, json({{"dirty", dirty != 0}, {"patch_id", data->patch->patch_id}}));
}

// Deferred callback for beginning undo group
static void begin_undo_deferred(t_maxmcp* patch, t_symbol* s, long argc, t_atom* argv) {
    VALIDATE_DEFERRED_ARGS("begin_undo_deferred");
    EXTRACT_DEFERRED_DATA_WITH_RESULT(t_begin_undo_data, data, argv);

    // Begin undo group
    jpatcher_beginundo(data->patch->patcher, gensym(data->undo_name.c_str()));

    ConsoleLogger::log(("Undo group started: " + data->undo_name).c_str());

    COMPLETE_DEFERRED(data, json({{"success", true}, {"undo_name", data->undo_name}}));
}

// Deferred callback for ending undo group
static void end_undo_deferred(t_maxmcp* patch, t_symbol* s, long argc, t_atom* argv) {
    VALIDATE_DEFERRED_ARGS("end_undo_deferred");
    EXTRACT_DEFERRED_DATA_WITH_RESULT(t_end_undo_data, data, argv);

    // End undo group
    jpatcher_endundo(data->patch->patcher);

    ConsoleLogger::log("Undo group ended");

    COMPLETE_DEFERRED(data, json({{"success", true}}));
}

#endif  // MAXMCP_TEST_MODE

// ============================================================================
// Tool Schemas
// ============================================================================

json get_tool_schemas() {
    return json::array({
        // Query tools
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
           {"required", json::array({"patch_id", "locked"})}}}},
        // Undo/Redo tools
        {{"name", "begin_undo_group"},
         {"description",
          "Begin an undo transaction group (multiple operations become one undo step)"},
         {"inputSchema",
          {{"type", "object"},
           {"properties",
            {{"patch_id", {{"type", "string"}, {"description", "Patch ID"}}},
             {"name",
              {{"type", "string"},
               {"description", "Name for the undo action (e.g., 'Add synthesizer')"}}}}},
           {"required", json::array({"patch_id", "name"})}}}},
        {{"name", "end_undo_group"},
         {"description", "End an undo transaction group"},
         {"inputSchema",
          {{"type", "object"},
           {"properties", {{"patch_id", {{"type", "string"}, {"description", "Patch ID"}}}}},
           {"required", json::array({"patch_id"})}}}}});
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

static json execute_begin_undo_group(const json& params) {
    std::string patch_id = params.value("patch_id", "");
    std::string name = params.value("name", "");

    if (patch_id.empty() || name.empty()) {
        return ToolCommon::make_error(-32602, "Missing required parameters: patch_id and name");
    }

    // Check for existing active undo transaction using ToolCommon
    if (ToolCommon::has_active_undo_transaction(patch_id)) {
        std::string active_name = ToolCommon::get_active_undo_name(patch_id);
        return ToolCommon::make_error(-32600, "Undo group already active: " + active_name);
    }

    // Start the undo transaction
    if (!ToolCommon::start_undo_transaction(patch_id, name)) {
        return ToolCommon::make_error(-32600, "Failed to start undo transaction");
    }

    t_maxmcp* patch = PatchRegistry::find_patch(patch_id);
    if (!patch) {
        ToolCommon::end_undo_transaction(patch_id);  // Cleanup on error
        return ToolCommon::patch_not_found_error(patch_id);
    }

    DeferredResult* deferred_result = new DeferredResult();
    t_begin_undo_data* data = new t_begin_undo_data{patch, name, deferred_result};

    t_atom a;
    atom_setobj(&a, data);
    defer(patch, (method)begin_undo_deferred, gensym("begin_undo"), 1, &a);

    if (!deferred_result->wait_for(ToolCommon::DEFAULT_DEFER_TIMEOUT)) {
        delete deferred_result;
        ToolCommon::end_undo_transaction(patch_id);  // Cleanup on timeout
        return ToolCommon::timeout_error("starting undo group");
    }

    json result = deferred_result->result;
    delete deferred_result;
    return {{"result", result}};
}

static json execute_end_undo_group(const json& params) {
    std::string patch_id = params.value("patch_id", "");

    if (patch_id.empty()) {
        return ToolCommon::missing_param_error("patch_id");
    }

    // Check for active undo transaction using ToolCommon
    if (!ToolCommon::has_active_undo_transaction(patch_id)) {
        return ToolCommon::make_error(-32600, "No active undo group");
    }

    // End the undo transaction in tracking
    ToolCommon::end_undo_transaction(patch_id);

    t_maxmcp* patch = PatchRegistry::find_patch(patch_id);
    if (!patch) {
        return ToolCommon::patch_not_found_error(patch_id);
    }

    DeferredResult* deferred_result = new DeferredResult();
    t_end_undo_data* data = new t_end_undo_data{patch, deferred_result};

    t_atom a;
    atom_setobj(&a, data);
    defer(patch, (method)end_undo_deferred, gensym("end_undo"), 1, &a);

    if (!deferred_result->wait_for(ToolCommon::DEFAULT_DEFER_TIMEOUT)) {
        delete deferred_result;
        return ToolCommon::timeout_error("ending undo group");
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
#ifndef MAXMCP_TEST_MODE
    if (tool == "get_patch_lock_state") {
        return execute_get_patch_lock_state(params);
    } else if (tool == "set_patch_lock_state") {
        return execute_set_patch_lock_state(params);
    } else if (tool == "get_patch_dirty") {
        return execute_get_patch_dirty(params);
    } else if (tool == "begin_undo_group") {
        return execute_begin_undo_group(params);
    } else if (tool == "end_undo_group") {
        return execute_end_undo_group(params);
    }
#endif

    // Tool not handled by this module - return nullptr to signal routing should continue
    return nullptr;
}

}  // namespace StateTools
