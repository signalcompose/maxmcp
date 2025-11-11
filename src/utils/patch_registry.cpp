/**
    @file patch_registry.cpp
    MaxMCP - Patch Registry Implementation

    @ingroup maxmcp
*/

#include "patch_registry.h"

#include "console_logger.h"
#include "maxmcp.h"

#include <algorithm>

// Static member initialization
std::vector<t_maxmcp*> PatchRegistry::patches_;
std::mutex PatchRegistry::mutex_;

void PatchRegistry::register_patch(t_maxmcp* patch) {
    if (!patch)
        return;

    std::lock_guard<std::mutex> lock(mutex_);
    patches_.push_back(patch);

#ifndef MAXMCP_TEST_MODE
    std::string msg = "Patch registered: " + patch->patch_id;
    ConsoleLogger::log(msg.c_str());
#endif
}

void PatchRegistry::unregister_patch(t_maxmcp* patch) {
    if (!patch)
        return;

    std::lock_guard<std::mutex> lock(mutex_);

    auto it = std::find(patches_.begin(), patches_.end(), patch);
    if (it != patches_.end()) {
#ifndef MAXMCP_TEST_MODE
        std::string msg = "Patch unregistered: " + patch->patch_id;
        ConsoleLogger::log(msg.c_str());
#endif
        patches_.erase(it);
    }
}

json PatchRegistry::list_patches(const std::string& group_filter) {
    std::lock_guard<std::mutex> lock(mutex_);

#ifndef MAXMCP_TEST_MODE
    // Debug: Log registry size
    std::string debug_msg =
        "PatchRegistry::list_patches() - Registry size: " + std::to_string(patches_.size());
    ConsoleLogger::log(debug_msg.c_str());
#endif

    json patches = json::array();

    for (auto* patch : patches_) {
        if (!patch)
            continue;

        // Get group name (may be empty)
        std::string group_name = "";
#ifndef MAXMCP_TEST_MODE
        if (patch->group && patch->group->s_name) {
            group_name = patch->group->s_name;
        }
#endif

        // Apply group filter if specified
        if (!group_filter.empty() && group_name != group_filter) {
            continue;  // Skip patches that don't match the filter
        }

        // Build patch entry
        json patch_entry = {{"patch_id", patch->patch_id},
                            {"display_name", patch->display_name},
                            {"patcher_name", patch->patcher_name}};

        // Add group field if set
        if (!group_name.empty()) {
            patch_entry["group"] = group_name;
        }

        patches.push_back(patch_entry);
    }

    // Build result (without wrapping in "result" key - that's handled by MCP server)
    json result = {{"patches", patches}, {"count", patches.size()}};

    // Add filter info if filter was applied
    if (!group_filter.empty()) {
        result["filter"] = {{"group", group_filter}};
    }

    return result;
}

t_maxmcp* PatchRegistry::find_patch(const std::string& patch_id) {
    std::lock_guard<std::mutex> lock(mutex_);

    for (auto* patch : patches_) {
        if (patch && patch->patch_id == patch_id) {
            return patch;
        }
    }

    return nullptr;
}

json PatchRegistry::get_patch_info(const std::string& patch_id) {
    std::lock_guard<std::mutex> lock(mutex_);

    // Find patch
    t_maxmcp* patch = nullptr;
    for (auto* p : patches_) {
        if (p && p->patch_id == patch_id) {
            patch = p;
            break;
        }
    }

    if (!patch) {
        return {{"error", {{"code", -32602}, {"message", "Patch not found: " + patch_id}}}};
    }

    // Get group name (may be empty)
    std::string group_name = "";
#ifndef MAXMCP_TEST_MODE
    if (patch->group && patch->group->s_name) {
        group_name = patch->group->s_name;
    }
#endif

    // Build patch information
    json patch_info = {{"patch_id", patch->patch_id},
                       {"display_name", patch->display_name},
                       {"patcher_name", patch->patcher_name},
                       {"has_patcher_ref", patch->patcher != nullptr}};

    // Add group field if set
    if (!group_name.empty()) {
        patch_info["group"] = group_name;
    }

    return patch_info;
}

json PatchRegistry::get_frontmost_patch() {
    std::lock_guard<std::mutex> lock(mutex_);

    // Check if any patches are registered
    if (patches_.empty()) {
        return {{"error", {{"code", -32603}, {"message", "No active patches"}}}};
    }

    // Simplified implementation: return first patch
    // Full implementation would use Max SDK window management APIs
    // to determine the actual frontmost window
    t_maxmcp* patch = patches_[0];

    if (!patch) {
        return {{"error", {{"code", -32603}, {"message", "Invalid patch reference"}}}};
    }

    // Get group name (may be empty)
    std::string group_name = "";
#ifndef MAXMCP_TEST_MODE
    if (patch->group && patch->group->s_name) {
        group_name = patch->group->s_name;
    }
#endif

    // Build patch information
    json patch_info = {{"patch_id", patch->patch_id},
                       {"display_name", patch->display_name},
                       {"patcher_name", patch->patcher_name}};

    // Add group field if set
    if (!group_name.empty()) {
        patch_info["group"] = group_name;
    }

    return patch_info;
}
