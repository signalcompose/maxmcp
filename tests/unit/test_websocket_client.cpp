/**
 * @file test_websocket_client.cpp
 * @brief Implementation of TestWebSocketClient
 */

#include "test_websocket_client.h"

#include <cstring>
#include <iostream>

// Callback implementation
int TestWebSocketClient::callback(struct lws* wsi, enum lws_callback_reasons reason, void* user,
                                  void* in, size_t len) {
    TestWebSocketClient* client =
        static_cast<TestWebSocketClient*>(lws_context_user(lws_get_context(wsi)));

    if (!client)
        return 0;

    switch (reason) {
    case LWS_CALLBACK_CLIENT_APPEND_HANDSHAKE_HEADER: {
        // Add Authorization header if set
        if (!client->auth_header_.empty()) {
            unsigned char** p = (unsigned char**)in;
            unsigned char* end = (*p) + len;

            std::string header = "Authorization: " + client->auth_header_ + "\r\n";

            if (end - (*p) < (int)header.length()) {
                std::cerr << "[TestClient] Not enough space for auth header\n";
                return 1;
            }

            memcpy(*p, header.c_str(), header.length());
            *p += header.length();
        }
        break;
    }

    case LWS_CALLBACK_CLIENT_ESTABLISHED:
        client->connected_ = true;
        std::cerr << "[TestClient] Connected\n";
        break;

    case LWS_CALLBACK_CLIENT_RECEIVE:
        if (in && len > 0) {
            std::string message(static_cast<const char*>(in), len);
            client->on_message_received(message);
        }
        break;

    case LWS_CALLBACK_CLIENT_CLOSED:
        client->connected_ = false;
        std::cerr << "[TestClient] Connection closed\n";
        break;

    case LWS_CALLBACK_CLIENT_CONNECTION_ERROR:
        client->connected_ = false;
        client->wsi_ = nullptr;  // Mark wsi as null so connect() loop exits
        if (in) {
            std::cerr << "[TestClient] Connection error: " << (const char*)in << "\n";
        } else {
            std::cerr << "[TestClient] Connection error\n";
        }
        break;

    default:
        break;
    }

    return 0;
}

bool TestWebSocketClient::connect(const std::string& url) {
    // Set up protocols (must be static)
    static const struct lws_protocols protocols[] = {{
                                                         "mcp",
                                                         TestWebSocketClient::callback,
                                                         0,
                                                         4096,
                                                     },
                                                     {nullptr, nullptr, 0, 0}};

    // Create context
    struct lws_context_creation_info info = {0};
    info.port = CONTEXT_PORT_NO_LISTEN;
    info.protocols = protocols;  // Use defined protocols
    info.user = this;

    context_ = lws_create_context(&info);
    if (!context_) {
        std::cerr << "[TestClient] Failed to create context\n";
        return false;
    }

    // Parse URL (simple parsing for ws://localhost:PORT format)
    int port = 7401;  // Default test port

    // Set up connection info
    struct lws_client_connect_info ccinfo = {0};
    ccinfo.context = context_;
    ccinfo.port = port;
    ccinfo.address = "localhost";
    ccinfo.path = "/";
    ccinfo.host = "localhost";
    ccinfo.origin = "localhost";
    ccinfo.protocol = "mcp";
    ccinfo.pwsi = &wsi_;
    // Auth header will be added in LWS_CALLBACK_CLIENT_APPEND_HANDSHAKE_HEADER

    wsi_ = lws_client_connect_via_info(&ccinfo);
    if (!wsi_) {
        std::cerr << "[TestClient] Failed to connect\n";
        lws_context_destroy(context_);
        context_ = nullptr;
        return false;
    }

    // Service the connection until connected, error, or timeout
    int n = 0;
    int retries = 100;  // 100 * 50ms = 5 seconds max
    bool error_occurred = false;

    while (n >= 0 && !connected_ && !error_occurred && retries-- > 0) {
        n = lws_service(context_, 50);
        // Check if wsi_ is still valid (becomes null on connection error)
        if (!wsi_ || n < 0) {
            error_occurred = true;
            break;
        }
    }

    return connected_;
}

void TestWebSocketClient::disconnect() {
    if (wsi_) {
        lws_close_reason(wsi_, LWS_CLOSE_STATUS_NORMAL, nullptr, 0);
        wsi_ = nullptr;
    }

    if (context_) {
        lws_context_destroy(context_);
        context_ = nullptr;
    }

    connected_ = false;
}

bool TestWebSocketClient::send(const std::string& message) {
    if (!connected_ || !wsi_) {
        return false;
    }

    // Allocate buffer with LWS_PRE padding
    size_t len = message.length();
    unsigned char* buf = new unsigned char[LWS_PRE + len];

    memcpy(buf + LWS_PRE, message.c_str(), len);

    int written = lws_write(wsi_, buf + LWS_PRE, len, LWS_WRITE_TEXT);

    delete[] buf;

    if (written < 0) {
        std::cerr << "[TestClient] Write failed\n";
        return false;
    }

    // Service to actually send
    lws_service(context_, 0);

    return true;
}

std::string TestWebSocketClient::wait_for_message(std::chrono::milliseconds timeout) {
    std::unique_lock<std::mutex> lock(queue_mutex_);

    // Use condition_variable for efficient waiting
    auto deadline = std::chrono::steady_clock::now() + timeout;

    while (message_queue_.empty()) {
        // Wait with timeout on condition variable
        auto remaining = deadline - std::chrono::steady_clock::now();
        if (remaining <= std::chrono::milliseconds(0)) {
            return "";  // Timeout
        }

        // Wait for notification or short timeout (to service libwebsockets)
        auto remaining_ms = std::chrono::duration_cast<std::chrono::milliseconds>(remaining);
        auto wait_time = std::min(remaining_ms, std::chrono::milliseconds(50));

        lock.unlock();
        lws_service(context_, wait_time.count());
        lock.lock();

        // Check again after servicing
        if (!message_queue_.empty()) {
            break;
        }
    }

    std::string message = message_queue_.front();
    message_queue_.pop();
    return message;
}

void TestWebSocketClient::on_message_received(const std::string& message) {
    std::lock_guard<std::mutex> lock(queue_mutex_);
    message_queue_.push(message);
    queue_cv_.notify_one();
}
