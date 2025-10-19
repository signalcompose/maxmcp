/**
    @file console_logger.h
    MaxMCP - Console Logger

    Thread-safe ring buffer for capturing Max Console output.
    Provides feedback loop for Claude Code via get_console_log MCP tool.

    @ingroup maxmcp
*/

#ifndef CONSOLE_LOGGER_H
#define CONSOLE_LOGGER_H

#include <deque>
#include <string>
#include <mutex>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

/**
 * @brief Thread-safe console log capture with ring buffer
 *
 * Captures all Max Console messages in a ring buffer (1000 lines max).
 * Thread-safe access via mutex. Available to Claude Code via get_console_log tool.
 */
class ConsoleLogger {
private:
    static std::deque<std::string> log_buffer_;
    static const size_t MAX_BUFFER_SIZE = 1000;
    static std::mutex mutex_;

public:
    /**
     * @brief Log a message to buffer and Max Console
     *
     * Thread-safe. Adds message to ring buffer and outputs to Max Console.
     * Old entries automatically discarded when buffer exceeds MAX_BUFFER_SIZE.
     *
     * @param message Message to log
     */
    static void log(const char* message);

    /**
     * @brief Retrieve recent log entries
     *
     * Thread-safe. Returns up to 'count' most recent log entries.
     *
     * @param count Number of lines to retrieve (default: 50, max: 1000)
     * @param clear Clear buffer after reading (default: false)
     * @return JSON object with logs array and count
     */
    static json get_logs(size_t count = 50, bool clear = false);

    /**
     * @brief Clear all log entries
     *
     * Thread-safe. Empties the log buffer.
     */
    static void clear();
};

#endif // CONSOLE_LOGGER_H
