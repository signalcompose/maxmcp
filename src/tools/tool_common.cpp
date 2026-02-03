/**
    @file tool_common.cpp
    MaxMCP - Common Tool Infrastructure Implementation

    @ingroup maxmcp
*/

#include "tool_common.h"

namespace ToolCommon {

// ============================================================================
// Undo Transaction Management - Implementation
// ============================================================================

// Static storage for undo transaction tracking
static std::map<std::string, std::string> active_undo_transactions_;  // patch_id -> undo_name
static std::mutex undo_mutex_;

bool has_active_undo_transaction(const std::string& patch_id) {
    std::lock_guard<std::mutex> lock(undo_mutex_);
    return active_undo_transactions_.count(patch_id) > 0;
}

std::string get_active_undo_name(const std::string& patch_id) {
    std::lock_guard<std::mutex> lock(undo_mutex_);
    auto it = active_undo_transactions_.find(patch_id);
    if (it != active_undo_transactions_.end()) {
        return it->second;
    }
    return "";
}

bool start_undo_transaction(const std::string& patch_id, const std::string& undo_name) {
    std::lock_guard<std::mutex> lock(undo_mutex_);
    if (active_undo_transactions_.count(patch_id)) {
        return false;  // Transaction already active
    }
    active_undo_transactions_[patch_id] = undo_name;
    return true;
}

bool end_undo_transaction(const std::string& patch_id) {
    std::lock_guard<std::mutex> lock(undo_mutex_);
    if (!active_undo_transactions_.count(patch_id)) {
        return false;  // No transaction active
    }
    active_undo_transactions_.erase(patch_id);
    return true;
}

}  // namespace ToolCommon
