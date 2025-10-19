/**
    @file console_logger.cpp
    MaxMCP - Console Logger Implementation

    @ingroup maxmcp
*/

#include "console_logger.h"
#include "ext.h"  // For post()

// Static member initialization
std::deque<std::string> ConsoleLogger::log_buffer_;
std::mutex ConsoleLogger::mutex_;
const size_t ConsoleLogger::MAX_BUFFER_SIZE;

void ConsoleLogger::log(const char* message) {
    std::lock_guard<std::mutex> lock(mutex_);

    // Add to buffer
    log_buffer_.push_back(std::string(message));

    // Ring buffer: keep only latest MAX_BUFFER_SIZE entries
    if (log_buffer_.size() > MAX_BUFFER_SIZE) {
        log_buffer_.pop_front();
    }

    // Also output to Max Console
    post("%s", message);
}

json ConsoleLogger::get_logs(size_t count, bool clear_after) {
    std::lock_guard<std::mutex> lock(mutex_);

    // Limit count to buffer size
    size_t actual_count = std::min(count, log_buffer_.size());
    actual_count = std::min(actual_count, MAX_BUFFER_SIZE);

    json logs = json::array();

    // Get most recent 'actual_count' entries
    auto start_it = log_buffer_.end() - actual_count;
    for (auto it = start_it; it != log_buffer_.end(); ++it) {
        logs.push_back(*it);
    }

    // Clear if requested
    if (clear_after) {
        log_buffer_.clear();
    }

    return {
        {"result", {
            {"logs", logs},
            {"count", actual_count}
        }}
    };
}

void ConsoleLogger::clear() {
    std::lock_guard<std::mutex> lock(mutex_);
    log_buffer_.clear();
}
