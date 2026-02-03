/**
    @file tool_common.h
    MaxMCP - Common Tool Infrastructure

    Shared types, structures, and utilities for MCP tools.

    @ingroup maxmcp
*/

#ifndef TOOL_COMMON_H
#define TOOL_COMMON_H

#include <nlohmann/json.hpp>

#include <chrono>
#include <condition_variable>
#include <map>
#include <mutex>
#include <string>

// Forward declarations
struct _maxmcp;
typedef struct _maxmcp t_maxmcp;

namespace ToolCommon {

using json = nlohmann::json;

// ============================================================================
// DeferredResult - Thread synchronization for deferred callbacks
// ============================================================================

/**
 * Helper structure for synchronizing deferred results between threads.
 * Used when a tool needs to wait for a deferred callback to complete
 * before returning a result.
 *
 * Usage:
 *   1. Create DeferredResult on heap: new DeferredResult()
 *   2. Pass pointer to deferred callback via data structure
 *   3. Call wait_for() in tool executor
 *   4. Callback stores result and calls notify()
 *   5. Delete after result is copied
 */
struct DeferredResult {
    json result;
    std::mutex mutex;
    std::condition_variable cv;
    bool completed = false;

    // Wait for completion with timeout
    bool wait_for(std::chrono::milliseconds timeout) {
        std::unique_lock<std::mutex> lock(mutex);
        return cv.wait_for(lock, timeout, [this] { return completed; });
    }

    // Notify waiting thread of completion
    void notify() {
        {
            std::lock_guard<std::mutex> lock(mutex);
            completed = true;
        }
        cv.notify_one();
    }
};

// ============================================================================
// Undo Transaction Management
// ============================================================================

/**
 * Check if an undo transaction is active for a patch.
 * @param patch_id The patch ID to check
 * @return true if an undo transaction is active
 */
bool has_active_undo_transaction(const std::string& patch_id);

/**
 * Get the name of the active undo transaction for a patch.
 * @param patch_id The patch ID to query
 * @return The undo transaction name, or empty string if none active
 */
std::string get_active_undo_name(const std::string& patch_id);

/**
 * Start an undo transaction for a patch.
 * @param patch_id The patch ID
 * @param undo_name The name for the undo action
 * @return true if successful, false if transaction already active
 */
bool start_undo_transaction(const std::string& patch_id, const std::string& undo_name);

/**
 * End an undo transaction for a patch.
 * @param patch_id The patch ID
 * @return true if successful, false if no transaction active
 */
bool end_undo_transaction(const std::string& patch_id);

// ============================================================================
// Helper Macros for Deferred Callbacks
// ============================================================================
//
// NOTE: Files using these macros must include the following headers:
//   - "ext.h" (Max SDK) for atom_getobj()
//   - "utils/console_logger.h" for ConsoleLogger::log()
//
// The macros assume these are available in the compilation context.
// ============================================================================

/**
 * Validate argc/argv at start of deferred callback.
 * Use at the beginning of any deferred callback function.
 *
 * Requires: ConsoleLogger::log() to be available
 *
 * @param func_name String literal for error logging
 */
#define VALIDATE_DEFERRED_ARGS(func_name)                                   \
    do {                                                                    \
        if (argc < 1 || !argv) {                                            \
            ConsoleLogger::log("ERROR: " func_name " called with no data"); \
            return;                                                         \
        }                                                                   \
    } while (0)

/**
 * Extract data pointer from atom and validate basic fields.
 * Assumes data struct has 'patch' member with 'patcher' field.
 *
 * Requires: atom_getobj() from Max SDK
 *
 * Note: This macro declares a variable, so cannot be wrapped in do-while.
 * Use only at the start of a callback function body.
 *
 * @param DataType The type of the data structure
 * @param data_ptr Variable name for the extracted pointer
 * @param argv The t_atom array from callback
 */
#define EXTRACT_DEFERRED_DATA(DataType, data_ptr, argv)                \
    DataType* data_ptr = static_cast<DataType*>(atom_getobj(argv));    \
    if (!data_ptr || !data_ptr->patch || !data_ptr->patch->patcher) {  \
        if (data_ptr)                                                  \
            delete data_ptr;                                           \
        return;                                                        \
    }

/**
 * Extract data pointer with deferred_result validation.
 * Use when callback needs to return a result.
 *
 * Requires: atom_getobj() from Max SDK
 *
 * Note: This macro declares a variable, so cannot be wrapped in do-while.
 * Use only at the start of a callback function body.
 *
 * @param DataType The type of the data structure
 * @param data_ptr Variable name for the extracted pointer
 * @param argv The t_atom array from callback
 */
#define EXTRACT_DEFERRED_DATA_WITH_RESULT(DataType, data_ptr, argv)       \
    DataType* data_ptr = static_cast<DataType*>(atom_getobj(argv));       \
    if (!data_ptr || !data_ptr->patch || !data_ptr->patch->patcher ||     \
        !data_ptr->deferred_result) {                                     \
        if (data_ptr)                                                     \
            delete data_ptr;                                              \
        return;                                                           \
    }

/**
 * Complete callback with result and cleanup.
 * Stores result, notifies waiting thread, and deletes data.
 *
 * @param data_ptr Pointer to data structure (must have deferred_result member)
 * @param result_json The JSON result to return
 */
#define COMPLETE_DEFERRED(data_ptr, result_json)             \
    do {                                                     \
        data_ptr->deferred_result->result = result_json;     \
        data_ptr->deferred_result->notify();                 \
        delete data_ptr;                                     \
    } while (0)

// ============================================================================
// Standard Timeout
// ============================================================================

constexpr auto DEFAULT_DEFER_TIMEOUT = std::chrono::milliseconds(5000);

// ============================================================================
// Error Response Helpers
// ============================================================================

/**
 * Create a JSON-RPC error response.
 * @param code Error code (e.g., -32602 for invalid params)
 * @param message Error message
 * @return JSON error object
 */
inline json make_error(int code, const std::string& message) {
    return {{"error", {{"code", code}, {"message", message}}}};
}

/**
 * Create a missing parameter error response.
 * @param param_name Name of the missing parameter
 * @return JSON error object
 */
inline json missing_param_error(const std::string& param_name) {
    return make_error(-32602, "Missing required parameter: " + param_name);
}

/**
 * Create a patch not found error response.
 * @param patch_id The patch ID that was not found
 * @return JSON error object
 */
inline json patch_not_found_error(const std::string& patch_id) {
    return make_error(-32602, "Patch not found: " + patch_id);
}

/**
 * Create a timeout error response.
 * @param operation Description of what timed out
 * @return JSON error object
 */
inline json timeout_error(const std::string& operation) {
    return make_error(-32603, "Timeout " + operation);
}

/**
 * Create an object not found error response.
 * @param varname The varname that was not found
 * @return JSON error object
 */
inline json object_not_found_error(const std::string& varname) {
    return make_error(-32602, "Object not found: " + varname);
}

}  // namespace ToolCommon

#endif  // TOOL_COMMON_H
