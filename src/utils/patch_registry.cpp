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

size_t PatchRegistry::count() {
    std::lock_guard<std::mutex> lock(mutex_);
    return patches_.size();
}
