/**
    @file tool_common.h
    MaxMCP - Common Tool Infrastructure

    Shared types, structures, and utilities for MCP tools.

    This file follows the TRY principle:
    - Testable: Clear error codes and Result type for easy testing
    - Readable: Named constants instead of magic numbers
    - Yieldable: RAII patterns for efficient resource management

    @ingroup maxmcp
*/

#ifndef TOOL_COMMON_H
#define TOOL_COMMON_H

#include <chrono>
#include <condition_variable>
#include <functional>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <variant>

#include <nlohmann/json.hpp>

// Forward declarations
struct _maxmcp;
typedef struct _maxmcp t_maxmcp;

namespace ToolCommon {

using json = nlohmann::json;

// ============================================================================
// JSON-RPC Error Codes (Readable: Named constants instead of magic numbers)
// ============================================================================
// Standard JSON-RPC 2.0 error codes
// See: https://www.jsonrpc.org/specification#error_object

namespace ErrorCode {
// Parse error: Invalid JSON was received
constexpr int PARSE_ERROR = -32700;
// Invalid Request: The JSON sent is not a valid Request object
constexpr int INVALID_REQUEST = -32600;
// Method not found: The method does not exist / is not available
constexpr int METHOD_NOT_FOUND = -32601;
// Invalid params: Invalid method parameter(s)
constexpr int INVALID_PARAMS = -32602;
// Internal error: Internal JSON-RPC error
constexpr int INTERNAL_ERROR = -32603;
}  // namespace ErrorCode

// ============================================================================
// Result Type (Testable: Type-safe error handling)
// ============================================================================

/**
 * @brief Result type for operations that can fail.
 *
 * Provides type-safe error handling that is easy to test.
 * Use std::variant to represent either success (json) or error (json).
 *
 * Example:
 *   auto result = validate_patch_id(params);
 *   if (auto* error = std::get_if<Error>(&result)) {
 *       return error->to_json();
 *   }
 *   auto& success = std::get<Success>(result);
 */
struct Success {
    json data;
    explicit Success(json d = json::object()) : data(std::move(d)) {}
};

struct Error {
    int code;
    std::string message;

    Error(int c, std::string m) : code(c), message(std::move(m)) {}

    json to_json() const {
        return {{"error", {{"code", code}, {"message", message}}}};
    }
};

using Result = std::variant<Success, Error>;

/// Check if result is an error
inline bool is_error(const Result& r) {
    return std::holds_alternative<Error>(r);
}

/// Check if result is success
inline bool is_success(const Result& r) {
    return std::holds_alternative<Success>(r);
}

/// Get error from result (throws if not error)
inline const Error& get_error(const Result& r) {
    return std::get<Error>(r);
}

/// Get success from result (throws if not success)
inline const Success& get_success(const Result& r) {
    return std::get<Success>(r);
}

/// Convert Result to JSON response
inline json result_to_json(const Result& r) {
    if (is_error(r)) {
        return get_error(r).to_json();
    }
    return {{"result", get_success(r).data}};
}

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
// DeferredResultPtr - RAII wrapper (Yieldable: Automatic resource cleanup)
// ============================================================================

/**
 * @brief RAII smart pointer for DeferredResult.
 *
 * Ensures proper cleanup of DeferredResult even if exceptions occur
 * or early returns happen. Automatically deletes the DeferredResult
 * when going out of scope.
 *
 * Usage:
 *   DeferredResultPtr deferred = make_deferred_result();
 *   // ... use deferred.get() ...
 *   // Automatically cleaned up when scope exits
 */
using DeferredResultPtr = std::unique_ptr<DeferredResult>;

/// Create a new DeferredResult with RAII management
inline DeferredResultPtr make_deferred_result() {
    return std::make_unique<DeferredResult>();
}

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
#define VALIDATE_DEFERRED_ARGS(func_name)                                                          \
    do {                                                                                           \
        if (argc < 1 || !argv) {                                                                   \
            ConsoleLogger::log("ERROR: " func_name " called with no data");                        \
            return;                                                                                \
        }                                                                                          \
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
#define EXTRACT_DEFERRED_DATA(DataType, data_ptr, argv)                                            \
    DataType* data_ptr = static_cast<DataType*>(atom_getobj(argv));                                \
    if (!data_ptr || !data_ptr->patch || !data_ptr->patch->patcher) {                              \
        ConsoleLogger::log("ERROR: Invalid deferred callback data (data/patch/patcher null)");     \
        if (data_ptr)                                                                              \
            delete data_ptr;                                                                       \
        return;                                                                                    \
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
#define EXTRACT_DEFERRED_DATA_WITH_RESULT(DataType, data_ptr, argv)                                \
    DataType* data_ptr = static_cast<DataType*>(atom_getobj(argv));                                \
    if (!data_ptr || !data_ptr->patch || !data_ptr->patch->patcher ||                              \
        !data_ptr->deferred_result) {                                                              \
        ConsoleLogger::log(                                                                        \
            "ERROR: Invalid deferred callback data (data/patch/patcher/deferred_result null)");    \
        if (data_ptr && data_ptr->deferred_result) {                                               \
            data_ptr->deferred_result->result = ToolCommon::make_error(                            \
                ToolCommon::ErrorCode::INTERNAL_ERROR, "Internal error: invalid callback data");   \
            data_ptr->deferred_result->notify();                                                   \
        }                                                                                          \
        if (data_ptr)                                                                              \
            delete data_ptr;                                                                       \
        return;                                                                                    \
    }

/**
 * Complete callback with result and cleanup.
 * Stores result, notifies waiting thread, and deletes data.
 *
 * @param data_ptr Pointer to data structure (must have deferred_result member)
 * @param result_json The JSON result to return
 */
#define COMPLETE_DEFERRED(data_ptr, result_json)                                                   \
    do {                                                                                           \
        data_ptr->deferred_result->result = result_json;                                           \
        data_ptr->deferred_result->notify();                                                       \
        delete data_ptr;                                                                           \
    } while (0)

// ============================================================================
// Standard Timeout
// ============================================================================

constexpr auto DEFAULT_DEFER_TIMEOUT = std::chrono::milliseconds(5000);

// Extended timeout for heavy read operations that iterate all objects/patchlines
// in a patch (e.g., get_patchlines, get_subpatchers, get_objects_in_patch)
constexpr auto HEAVY_OPERATION_TIMEOUT = std::chrono::milliseconds(30000);

// ============================================================================
// Error Response Helpers (Readable: Using named constants)
// ============================================================================

/**
 * Create a JSON-RPC error response.
 * @param code Error code (use ErrorCode constants)
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
    return make_error(ErrorCode::INVALID_PARAMS, "Missing required parameter: " + param_name);
}

/**
 * Create a patch not found error response.
 * @param patch_id The patch ID that was not found
 * @return JSON error object
 */
inline json patch_not_found_error(const std::string& patch_id) {
    return make_error(ErrorCode::INVALID_PARAMS, "Patch not found: " + patch_id);
}

/**
 * Create a timeout error response.
 * @param operation Description of what timed out
 * @return JSON error object
 */
inline json timeout_error(const std::string& operation) {
    return make_error(ErrorCode::INTERNAL_ERROR, "Timeout " + operation);
}

/**
 * Create an object not found error response.
 * @param varname The varname that was not found
 * @return JSON error object
 */
inline json object_not_found_error(const std::string& varname) {
    return make_error(ErrorCode::INVALID_PARAMS, "Object not found: " + varname);
}

/**
 * Create an unknown tool error response.
 * @param tool_name The tool name that was not found
 * @return JSON error object
 */
inline json unknown_tool_error(const std::string& tool_name) {
    return make_error(ErrorCode::INVALID_PARAMS, "Unknown tool: " + tool_name);
}

/**
 * Create a method not found error response.
 * @param method The method that was not found
 * @return JSON error object
 */
inline json method_not_found_error(const std::string& method) {
    return make_error(ErrorCode::METHOD_NOT_FOUND, "Method not found: " + method);
}

/**
 * Create a test mode error response.
 * @return JSON error object
 */
inline json test_mode_error() {
    return make_error(ErrorCode::INTERNAL_ERROR, "Not available in test mode");
}

// ============================================================================
// Validation Helpers (Testable: Composable validation functions)
// ============================================================================

/**
 * @brief Validate that a required string parameter exists and is non-empty.
 *
 * @param params The JSON parameters object
 * @param param_name The name of the parameter to validate
 * @return std::optional<std::string> The value if valid, std::nullopt if invalid
 */
inline std::optional<std::string> get_required_string(const json& params,
                                                      const std::string& param_name) {
    std::string value = params.value(param_name, "");
    if (value.empty()) {
        return std::nullopt;
    }
    return value;
}

/**
 * @brief Validate patch_id parameter and return error JSON if invalid.
 *
 * Common pattern used by most tools. Returns error JSON or nullptr.
 *
 * @param params The JSON parameters object
 * @return json Error JSON if validation fails, nullptr if valid
 */
inline json validate_patch_id_param(const json& params) {
    auto patch_id = get_required_string(params, "patch_id");
    if (!patch_id) {
        return missing_param_error("patch_id");
    }
    return nullptr;
}

/**
 * @brief Get validated patch_id from params.
 *
 * @param params The JSON parameters object
 * @return std::string The patch_id (empty if not present)
 */
inline std::string get_patch_id(const json& params) {
    return params.value("patch_id", "");
}

}  // namespace ToolCommon

#endif  // TOOL_COMMON_H
