/**
 * @file test_websocket_client.h
 * @brief Test WebSocket client for unit testing
 *
 * Simple WebSocket client implementation for testing WebSocketServer.
 * Uses libwebsockets client API.
 */

#ifndef TEST_WEBSOCKET_CLIENT_H
#define TEST_WEBSOCKET_CLIENT_H

#include <string>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <chrono>
#include <libwebsockets.h>

class TestWebSocketClient {
public:
    TestWebSocketClient() : wsi_(nullptr), context_(nullptr), connected_(false) {}

    ~TestWebSocketClient() {
        disconnect();
    }

    /**
     * Connect to WebSocket server
     * @param url WebSocket URL (e.g., "ws://localhost:7400")
     * @return true if connection successful
     */
    bool connect(const std::string& url);

    /**
     * Disconnect from server
     */
    void disconnect();

    /**
     * Send message to server
     * @param message Message to send
     * @return true if send successful
     */
    bool send(const std::string& message);

    /**
     * Wait for incoming message
     * @param timeout Maximum wait time
     * @return Received message (empty if timeout)
     */
    std::string wait_for_message(std::chrono::milliseconds timeout);

    /**
     * Set authentication header
     * @param auth Authorization header value (e.g., "Bearer token")
     */
    void set_auth_header(const std::string& auth) {
        auth_header_ = auth;
    }

    /**
     * Check if connected
     */
    bool is_connected() const {
        return connected_;
    }

private:
    struct lws* wsi_;
    struct lws_context* context_;
    bool connected_;
    std::string auth_header_;

    // Message queue
    std::queue<std::string> message_queue_;
    std::mutex queue_mutex_;
    std::condition_variable queue_cv_;

    // libwebsockets callback
    static int callback(struct lws* wsi, enum lws_callback_reasons reason,
                       void* user, void* in, size_t len);

    // Internal message handler
    void on_message_received(const std::string& message);
};

#endif // TEST_WEBSOCKET_CLIENT_H
