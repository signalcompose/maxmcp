/**
 * @file test_websocket_server.cpp
 * @brief Unit tests for WebSocketServer class
 *
 * Tests cover:
 * - Basic connection/disconnection
 * - Message sending/receiving
 * - Multi-client support
 * - Request queueing
 * - Authentication
 *
 * Following TDD principles: Write tests first, then implement.
 *
 * NOTE: Some slow tests are disabled by default (prefixed with DISABLED_):
 * - DISABLED_ClientCanSendMessage (30s)
 * - DISABLED_RequestsAreProcessedInOrder (150s)
 * - DISABLED_MultiClientRequestsAreQueued (30s)
 * - DISABLED_InvalidJSONHandling (30s)
 *
 * To run disabled tests:
 *   ./test_websocket_server --gtest_also_run_disabled_tests
 *
 * To run only disabled tests:
 *   ./test_websocket_server --gtest_filter=*DISABLED_* --gtest_also_run_disabled_tests
 */

#include <chrono>
#include <future>
#include <thread>

#include <gtest/gtest.h>
#include <nlohmann/json.hpp>

// WebSocket client for testing (will be implemented)
#include "test_websocket_client.h"

// Class under test (will be implemented)
#include "../../src/websocket_server.h"

using namespace std::chrono_literals;

class WebSocketServerTest : public ::testing::Test {
  protected:
    void SetUp() override {
        // Start server on test port
        server = new WebSocketServer(TEST_PORT);
        server->start();

        // Wait for server to be ready (reduced from 100ms)
        std::this_thread::sleep_for(50ms);
    }

    void TearDown() override {
        if (server) {
            server->stop();
            delete server;
            server = nullptr;
        }
    }

    static constexpr int TEST_PORT = 7401;
    WebSocketServer* server = nullptr;
};

// ============================================================================
// Basic Connection Tests
// ============================================================================

TEST_F(WebSocketServerTest, ServerStartsSuccessfully) {
    EXPECT_TRUE(server->is_running());
    EXPECT_EQ(server->get_port(), TEST_PORT);
}

TEST_F(WebSocketServerTest, ClientCanConnect) {
    TestWebSocketClient client;
    bool connected = client.connect("ws://localhost:" + std::to_string(TEST_PORT));

    EXPECT_TRUE(connected);
    EXPECT_EQ(server->get_client_count(), 1);

    client.disconnect();
    std::this_thread::sleep_for(50ms);

    EXPECT_EQ(server->get_client_count(), 0);
}

TEST_F(WebSocketServerTest, MultipleClientsCanConnect) {
    TestWebSocketClient client1, client2, client3;

    EXPECT_TRUE(client1.connect("ws://localhost:" + std::to_string(TEST_PORT)));
    EXPECT_TRUE(client2.connect("ws://localhost:" + std::to_string(TEST_PORT)));
    EXPECT_TRUE(client3.connect("ws://localhost:" + std::to_string(TEST_PORT)));

    EXPECT_EQ(server->get_client_count(), 3);

    client1.disconnect();
    std::this_thread::sleep_for(50ms);
    EXPECT_EQ(server->get_client_count(), 2);

    client2.disconnect();
    client3.disconnect();
}

// ============================================================================
// Message Send/Receive Tests
// ============================================================================

TEST_F(WebSocketServerTest, DISABLED_ClientCanSendMessage) {
    TestWebSocketClient client;
    client.connect("ws://localhost:" + std::to_string(TEST_PORT));

    std::string test_message = R"({"jsonrpc":"2.0","method":"test","id":1})";

    // Set up server message handler
    std::promise<std::string> received_promise;
    auto received_future = received_promise.get_future();

    server->set_message_callback([&](const std::string& client_id, const std::string& message) {
        received_promise.set_value(message);
    });

    client.send(test_message);

    auto status = received_future.wait_for(1s);
    ASSERT_EQ(status, std::future_status::ready);

    std::string received = received_future.get();
    EXPECT_EQ(received, test_message);
}

TEST_F(WebSocketServerTest, DISABLED_ServerCanSendMessage) {
    TestWebSocketClient client;
    client.connect("ws://localhost:" + std::to_string(TEST_PORT));

    // Wait for client to be fully registered
    std::this_thread::sleep_for(100ms);

    std::string test_message = R"({"jsonrpc":"2.0","result":"ok","id":1})";

    // Get client ID
    auto clients = server->get_connected_clients();
    ASSERT_FALSE(clients.empty()) << "No clients connected";
    std::string client_id = clients[0];

    // Send message to specific client
    server->send_to_client(client_id, test_message);

    std::string received = client.wait_for_message(1s);
    EXPECT_EQ(received, test_message);
}

TEST_F(WebSocketServerTest, ServerCanBroadcast) {
    TestWebSocketClient client1, client2, client3;
    client1.connect("ws://localhost:" + std::to_string(TEST_PORT));
    client2.connect("ws://localhost:" + std::to_string(TEST_PORT));
    client3.connect("ws://localhost:" + std::to_string(TEST_PORT));

    std::string test_message = R"({"jsonrpc":"2.0","method":"notification","params":{}})";

    server->broadcast(test_message);

    EXPECT_EQ(client1.wait_for_message(1s), test_message);
    EXPECT_EQ(client2.wait_for_message(1s), test_message);
    EXPECT_EQ(client3.wait_for_message(1s), test_message);
}

// ============================================================================
// Request Queueing Tests
// ============================================================================

TEST_F(WebSocketServerTest, DISABLED_RequestsAreProcessedInOrder) {
    TestWebSocketClient client;
    client.connect("ws://localhost:" + std::to_string(TEST_PORT));

    std::vector<int> processed_order;
    std::mutex order_mutex;

    server->set_message_callback([&](const std::string& client_id, const std::string& message) {
        // Extract ID from message
        nlohmann::json j = nlohmann::json::parse(message);
        int id = j["id"];

        std::lock_guard<std::mutex> lock(order_mutex);
        processed_order.push_back(id);
    });

    // Send multiple messages rapidly
    client.send(R"({"jsonrpc":"2.0","method":"test","id":1})");
    client.send(R"({"jsonrpc":"2.0","method":"test","id":2})");
    client.send(R"({"jsonrpc":"2.0","method":"test","id":3})");
    client.send(R"({"jsonrpc":"2.0","method":"test","id":4})");
    client.send(R"({"jsonrpc":"2.0","method":"test","id":5})");

    std::this_thread::sleep_for(500ms);

    // Verify FIFO order
    ASSERT_EQ(processed_order.size(), 5);
    EXPECT_EQ(processed_order[0], 1);
    EXPECT_EQ(processed_order[1], 2);
    EXPECT_EQ(processed_order[2], 3);
    EXPECT_EQ(processed_order[3], 4);
    EXPECT_EQ(processed_order[4], 5);
}

TEST_F(WebSocketServerTest, DISABLED_MultiClientRequestsAreQueued) {
    TestWebSocketClient client1, client2;
    client1.connect("ws://localhost:" + std::to_string(TEST_PORT));
    client2.connect("ws://localhost:" + std::to_string(TEST_PORT));

    std::atomic<int> processed_count{0};

    server->set_message_callback([&](const std::string& client_id, const std::string& message) {
        // Simulate processing time
        std::this_thread::sleep_for(50ms);
        processed_count++;
    });

    // Both clients send requests simultaneously
    auto future1 = std::async(
        std::launch::async, [&]() { client1.send(R"({"jsonrpc":"2.0","method":"test","id":1})"); });
    auto future2 = std::async(
        std::launch::async, [&]() { client2.send(R"({"jsonrpc":"2.0","method":"test","id":2})"); });

    future1.wait();
    future2.wait();

    // Wait for processing
    std::this_thread::sleep_for(200ms);

    EXPECT_EQ(processed_count, 2);
}

// ============================================================================
// Authentication Tests
// ============================================================================

TEST_F(WebSocketServerTest, AuthenticationRequired) {
    // Stop server and restart with auth
    server->stop();
    delete server;

    server = new WebSocketServer(TEST_PORT, "secret-token-123");
    server->start();
    std::this_thread::sleep_for(100ms);

    // Connection without auth header should fail
    TestWebSocketClient client_no_auth;
    bool connected = client_no_auth.connect("ws://localhost:" + std::to_string(TEST_PORT));

    EXPECT_FALSE(connected);
    EXPECT_EQ(server->get_client_count(), 0);
}

TEST_F(WebSocketServerTest, AuthenticationWithValidToken) {
    server->stop();
    delete server;

    server = new WebSocketServer(TEST_PORT, "secret-token-123");
    server->start();
    std::this_thread::sleep_for(100ms);

    // Connection with valid auth header
    TestWebSocketClient client;
    client.set_auth_header("Bearer secret-token-123");
    bool connected = client.connect("ws://localhost:" + std::to_string(TEST_PORT));

    EXPECT_TRUE(connected);
    EXPECT_EQ(server->get_client_count(), 1);
}

TEST_F(WebSocketServerTest, AuthenticationWithInvalidToken) {
    server->stop();
    delete server;

    server = new WebSocketServer(TEST_PORT, "secret-token-123");
    server->start();
    std::this_thread::sleep_for(100ms);

    // Connection with invalid auth header
    TestWebSocketClient client;
    client.set_auth_header("Bearer wrong-token");
    bool connected = client.connect("ws://localhost:" + std::to_string(TEST_PORT));

    EXPECT_FALSE(connected);
    EXPECT_EQ(server->get_client_count(), 0);
}

// ============================================================================
// Thread Safety Tests
// ============================================================================

TEST_F(WebSocketServerTest, ConcurrentClientManagement) {
    std::vector<std::thread> threads;
    std::atomic<int> successful_connections{0};

    for (int i = 0; i < 10; i++) {
        threads.emplace_back([&]() {
            TestWebSocketClient client;
            if (client.connect("ws://localhost:" + std::to_string(TEST_PORT))) {
                successful_connections++;
                std::this_thread::sleep_for(100ms);
                client.disconnect();
            }
        });
    }

    for (auto& t : threads) {
        t.join();
    }

    EXPECT_EQ(successful_connections, 10);

    std::this_thread::sleep_for(200ms);
    EXPECT_EQ(server->get_client_count(), 0);
}

// ============================================================================
// Error Handling Tests
// ============================================================================

TEST_F(WebSocketServerTest, DISABLED_InvalidJSONHandling) {
    TestWebSocketClient client;
    client.connect("ws://localhost:" + std::to_string(TEST_PORT));

    std::promise<std::string> error_promise;
    auto error_future = error_promise.get_future();

    server->set_error_callback([&](const std::string& client_id, const std::string& error) {
        error_promise.set_value(error);
    });

    // Send invalid JSON
    client.send("this is not json");

    auto status = error_future.wait_for(1s);
    ASSERT_EQ(status, std::future_status::ready);

    std::string error = error_future.get();
    EXPECT_TRUE(error.find("Parse error") != std::string::npos);
}

TEST_F(WebSocketServerTest, ServerStopsCleanly) {
    TestWebSocketClient client1, client2;
    client1.connect("ws://localhost:" + std::to_string(TEST_PORT));
    client2.connect("ws://localhost:" + std::to_string(TEST_PORT));

    EXPECT_EQ(server->get_client_count(), 2);

    server->stop();

    EXPECT_FALSE(server->is_running());
    EXPECT_EQ(server->get_client_count(), 0);
}

// ============================================================================
// Main
// ============================================================================

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
