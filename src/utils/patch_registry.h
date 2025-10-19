/**
    @file patch_registry.h
    MaxMCP - Patch Registry

    Global registry for tracking all active maxmcp client instances.

    @ingroup maxmcp
*/

#ifndef PATCH_REGISTRY_H
#define PATCH_REGISTRY_H

#include <vector>
#include <string>
#include <mutex>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

// Forward declaration
struct _maxmcp;
typedef struct _maxmcp t_maxmcp;

/**
 * @brief Global registry for maxmcp client patches
 *
 * Thread-safe singleton registry for tracking all active [maxmcp] instances.
 * Enables list_active_patches MCP tool.
 */
class PatchRegistry {
private:
    static std::vector<t_maxmcp*> patches_;
    static std::mutex mutex_;

public:
    /**
     * @brief Register a patch in the global registry
     *
     * Thread-safe. Called from maxmcp_new().
     *
     * @param patch Pointer to maxmcp object
     */
    static void register_patch(t_maxmcp* patch);

    /**
     * @brief Unregister a patch from the global registry
     *
     * Thread-safe. Called from maxmcp_free().
     *
     * @param patch Pointer to maxmcp object
     */
    static void unregister_patch(t_maxmcp* patch);

    /**
     * @brief List all active patches
     *
     * Thread-safe. Returns JSON array of patch metadata.
     *
     * @return JSON object with patches array and count
     */
    static json list_patches();

    /**
     * @brief Find patch by ID
     *
     * Thread-safe. Returns nullptr if not found.
     *
     * @param patch_id Patch ID to search for
     * @return Pointer to maxmcp object, or nullptr
     */
    static t_maxmcp* find_patch(const std::string& patch_id);

    /**
     * @brief Get detailed information about a patch
     *
     * @param patch_id Patch ID to query
     * @return JSON object with patch information, or error
     */
    static json get_patch_info(const std::string& patch_id);

    /**
     * @brief Get the frontmost (focused) patch
     *
     * Returns information about the currently focused patch window.
     * Note: This is a simplified implementation that returns the first
     * registered patch. Full implementation would require Max SDK
     * window management APIs.
     *
     * @return JSON object with frontmost patch info, or error if no patches
     */
    static json get_frontmost_patch();
};

#endif // PATCH_REGISTRY_H
