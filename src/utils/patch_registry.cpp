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
    if (!patch) return;

    std::lock_guard<std::mutex> lock(mutex_);
    patches_.push_back(patch);

    std::string msg = "Patch registered: " + patch->patch_id;
    ConsoleLogger::log(msg.c_str());
}

void PatchRegistry::unregister_patch(t_maxmcp* patch) {
    if (!patch) return;

    std::lock_guard<std::mutex> lock(mutex_);

    auto it = std::find(patches_.begin(), patches_.end(), patch);
    if (it != patches_.end()) {
        std::string msg = "Patch unregistered: " + patch->patch_id;
        ConsoleLogger::log(msg.c_str());
        patches_.erase(it);
    }
}

json PatchRegistry::list_patches() {
    std::lock_guard<std::mutex> lock(mutex_);

    json patches = json::array();

    for (auto* patch : patches_) {
        if (!patch) continue;

        patches.push_back({
            {"patch_id", patch->patch_id},
            {"display_name", patch->display_name},
            {"patcher_name", patch->patcher_name}
        });
    }

    return {
        {"result", {
            {"patches", patches},
            {"count", patches.size()}
        }}
    };
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
        return {
            {"error", {
                {"code", -32602},
                {"message", "Patch not found: " + patch_id}
            }}
        };
    }

    // Return detailed patch information
    return {
        {"result", {
            {"patch_id", patch->patch_id},
            {"display_name", patch->display_name},
            {"patcher_name", patch->patcher_name},
            {"has_patcher_ref", patch->patcher != nullptr}
        }}
    };
}

json PatchRegistry::get_frontmost_patch() {
    std::lock_guard<std::mutex> lock(mutex_);

    // Check if any patches are registered
    if (patches_.empty()) {
        return {
            {"error", {
                {"code", -32603},
                {"message", "No active patches"}
            }}
        };
    }

    // Simplified implementation: return first patch
    // Full implementation would use Max SDK window management APIs
    // to determine the actual frontmost window
    t_maxmcp* patch = patches_[0];

    if (!patch) {
        return {
            {"error", {
                {"code", -32603},
                {"message", "Invalid patch reference"}
            }}
        };
    }

    return {
        {"result", {
            {"patch_id", patch->patch_id},
            {"display_name", patch->display_name},
            {"patcher_name", patch->patcher_name}
        }}
    };
}
