/**
 * @file test_console_logger.cpp
 * Unit tests for ConsoleLogger
 */

#include "console_logger.h"

#include <thread>
#include <vector>

#include <gtest/gtest.h>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

/**
 * Test Fixture for ConsoleLogger
 */
class ConsoleLoggerTest : public ::testing::Test {
  protected:
    void SetUp() override {
        // Clear log before each test
        ConsoleLogger::clear();
    }

    void TearDown() override {
        // Clean up after each test
        ConsoleLogger::clear();
    }
};

/**
 * Test: Log single message
 */
TEST_F(ConsoleLoggerTest, LogSingleMessage) {
    ConsoleLogger::log("Test message");

    auto result = ConsoleLogger::get_logs(10, false);
    EXPECT_TRUE(result.contains("logs"));
    EXPECT_TRUE(result.contains("count"));

    auto logs = result["logs"];
    EXPECT_EQ(logs.size(), 1);
    EXPECT_EQ(logs[0], "Test message");
}

/**
 * Test: Log multiple messages
 */
TEST_F(ConsoleLoggerTest, LogMultipleMessages) {
    ConsoleLogger::log("Message 1");
    ConsoleLogger::log("Message 2");
    ConsoleLogger::log("Message 3");

    auto result = ConsoleLogger::get_logs(10, false);
    auto logs = result["logs"];

    EXPECT_EQ(logs.size(), 3);
    EXPECT_EQ(logs[0], "Message 1");
    EXPECT_EQ(logs[1], "Message 2");
    EXPECT_EQ(logs[2], "Message 3");
}

/**
 * Test: Ring buffer behavior (MAX_BUFFER_SIZE = 1000)
 */
TEST_F(ConsoleLoggerTest, RingBuffer) {
    // Log more than buffer size
    for (int i = 0; i < 1500; i++) {
        ConsoleLogger::log(("Message " + std::to_string(i)).c_str());
    }

    auto result = ConsoleLogger::get_logs(2000, false);
    auto logs = result["logs"];

    // Should have only last 1000 messages
    EXPECT_EQ(logs.size(), 1000);

    // First message should be "Message 500" (1500 - 1000)
    EXPECT_EQ(logs[0], "Message 500");

    // Last message should be "Message 1499"
    EXPECT_EQ(logs[999], "Message 1499");
}

/**
 * Test: Get logs with limit
 */
TEST_F(ConsoleLoggerTest, GetLogsWithLimit) {
    for (int i = 0; i < 100; i++) {
        ConsoleLogger::log(("Message " + std::to_string(i)).c_str());
    }

    // Get only last 10 messages
    auto result = ConsoleLogger::get_logs(10, false);
    auto logs = result["logs"];

    EXPECT_EQ(logs.size(), 10);
    EXPECT_EQ(logs[0], "Message 90");
    EXPECT_EQ(logs[9], "Message 99");
}

/**
 * Test: Clear logs
 */
TEST_F(ConsoleLoggerTest, ClearLogs) {
    ConsoleLogger::log("Message 1");
    ConsoleLogger::log("Message 2");

    // Clear
    ConsoleLogger::clear();

    auto result = ConsoleLogger::get_logs(10, false);
    auto logs = result["logs"];

    EXPECT_EQ(logs.size(), 0);
}

/**
 * Test: Get logs with clear flag
 */
TEST_F(ConsoleLoggerTest, GetLogsWithClear) {
    ConsoleLogger::log("Message 1");
    ConsoleLogger::log("Message 2");

    // Get logs and clear
    auto result = ConsoleLogger::get_logs(10, true);
    auto logs = result["logs"];

    EXPECT_EQ(logs.size(), 2);

    // Verify logs are cleared
    result = ConsoleLogger::get_logs(10, false);
    logs = result["logs"];

    EXPECT_EQ(logs.size(), 0);
}

/**
 * Test: Empty log retrieval
 */
TEST_F(ConsoleLoggerTest, EmptyLogRetrieval) {
    auto result = ConsoleLogger::get_logs(10, false);
    auto logs = result["logs"];

    EXPECT_EQ(logs.size(), 0);
}

/**
 * Test: Thread safety (basic)
 */
TEST_F(ConsoleLoggerTest, ThreadSafety) {
    // Create multiple threads logging simultaneously
    std::vector<std::thread> threads;

    for (int i = 0; i < 10; i++) {
        threads.push_back(std::thread([i]() {
            for (int j = 0; j < 100; j++) {
                ConsoleLogger::log(
                    ("Thread " + std::to_string(i) + " Message " + std::to_string(j)).c_str());
            }
        }));
    }

    // Wait for all threads
    for (auto& t : threads) {
        t.join();
    }

    // Should have 1000 messages (10 threads * 100 messages)
    auto result = ConsoleLogger::get_logs(2000, false);
    auto logs = result["logs"];

    EXPECT_EQ(logs.size(), 1000);
}
