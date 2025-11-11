/**
 * @file websocket_server.cpp
 * @brief WebSocket server implementation
 */

#include "websocket_server.h"

#include "utils/console_logger.h"

#include <cstring>
#include <iomanip>
#include <random>
#include <sstream>

#include <nlohmann/json.hpp>

using json = nlohmann::json;

// libwebsockets protocol definition
static const struct lws_protocols protocols[] = {
    {
        "mcp",  // Protocol name
        WebSocketServer::lws_callback,
        0,     // per_session_data_size
        4096,  // rx_buffer_size
    },
    {nullptr, nullptr, 0, 0}  // Terminator
};

// ============================================================================
// Constructor / Destructor
// ============================================================================

WebSocketServer::WebSocketServer(int port, const std::string& auth_token)
    : port_(port), auth_token_(auth_token), require_auth_(!auth_token.empty()), running_(false),
      context_(nullptr) {
    ConsoleLogger::log("WebSocketServer created");
}

WebSocketServer::~WebSocketServer() {
    stop();
}

// ============================================================================
// Server Lifecycle
// ============================================================================

bool WebSocketServer::start() {
    if (running_) {
        ConsoleLogger::log("Server already running");
        return false;
    }

    // Create libwebsockets context
    struct lws_context_creation_info info;
    memset(&info, 0, sizeof(info));

    info.port = port_;
    info.protocols = protocols;
    info.user = this;  // Store pointer to this instance
    info.options = LWS_SERVER_OPTION_VALIDATE_UTF8;

    context_ = lws_create_context(&info);
    if (!context_) {
        ConsoleLogger::log("ERROR: Failed to create libwebsockets context");
        return false;
    }

    running_ = true;

    // Start service thread
    service_thread_ = std::thread(&WebSocketServer::service_loop, this);

    // Start queue processor thread
    queue_processor_thread_ = std::thread(&WebSocketServer::process_queue_loop, this);

    ConsoleLogger::log(("WebSocket server started on port " + std::to_string(port_)).c_str());

    return true;
}

void WebSocketServer::stop() {
    if (!running_) {
        return;
    }

    running_ = false;

    // Wake up queue processor thread
    queue_cv_.notify_one();

    // Cancel any blocking lws_service() calls to speed up thread termination
    if (context_) {
        lws_cancel_service(context_);
    }

    // Wait for threads to finish
    if (service_thread_.joinable()) {
        service_thread_.join();
    }

    if (queue_processor_thread_.joinable()) {
        queue_processor_thread_.join();
    }

    // Destroy context
    if (context_) {
        lws_context_destroy(context_);
        context_ = nullptr;
    }

    // Clear clients
    {
        std::lock_guard<std::mutex> lock(clients_mutex_);
        clients_.clear();
    }

    ConsoleLogger::log("WebSocket server stopped");
}

// ============================================================================
// Service Loop
// ============================================================================

void WebSocketServer::service_loop() {
    ConsoleLogger::log("Service thread started");

    while (running_) {
        lws_service(context_, 50);  // Service with 50ms timeout
    }

    ConsoleLogger::log("Service thread stopped");
}

// ============================================================================
// Client Management
// ============================================================================

size_t WebSocketServer::get_client_count() const {
    std::lock_guard<std::mutex> lock(clients_mutex_);
    return clients_.size();
}

std::vector<std::string> WebSocketServer::get_connected_clients() const {
    std::lock_guard<std::mutex> lock(clients_mutex_);
    std::vector<std::string> client_ids;
    for (const auto& pair : clients_) {
        client_ids.push_back(pair.first);
    }
    return client_ids;
}

std::string WebSocketServer::generate_client_id() {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 15);

    std::stringstream ss;
    ss << "client_";
    for (int i = 0; i < 8; i++) {
        ss << std::hex << dis(gen);
    }
    return ss.str();
}

void WebSocketServer::add_client(struct lws* wsi, const std::string& ip) {
    std::lock_guard<std::mutex> lock(clients_mutex_);

    std::string client_id = generate_client_id();

    auto info = std::make_shared<ClientInfo>();
    info->client_id = client_id;
    info->wsi = wsi;
    info->ip_address = ip;
    info->authenticated = !require_auth_;  // Auto-authenticated if no auth required

    clients_[client_id] = info;

    ConsoleLogger::log(("Client connected: " + client_id + " from " + ip +
                        " (total: " + std::to_string(clients_.size()) + ")")
                           .c_str());
}

void WebSocketServer::remove_client(struct lws* wsi) {
    std::lock_guard<std::mutex> lock(clients_mutex_);

    for (auto it = clients_.begin(); it != clients_.end(); ++it) {
        if (it->second->wsi == wsi) {
            ConsoleLogger::log(("Client disconnected: " + it->first +
                                " (remaining: " + std::to_string(clients_.size() - 1) + ")")
                                   .c_str());
            clients_.erase(it);
            return;
        }
    }
}

std::shared_ptr<ClientInfo> WebSocketServer::find_client_by_wsi(struct lws* wsi) {
    std::lock_guard<std::mutex> lock(clients_mutex_);

    for (auto& pair : clients_) {
        if (pair.second->wsi == wsi) {
            return pair.second;
        }
    }
    return nullptr;
}

// ============================================================================
// Message Sending
// ============================================================================

bool WebSocketServer::send_to_client(const std::string& client_id, const std::string& message) {
    std::lock_guard<std::mutex> lock(clients_mutex_);

    auto it = clients_.find(client_id);
    if (it == clients_.end()) {
        return false;
    }

    // Add message to client's send queue
    {
        std::lock_guard<std::mutex> send_lock(it->second->send_mutex);
        it->second->send_queue.push(message);
    }

    // Request callback on writable (triggers LWS_CALLBACK_SERVER_WRITEABLE)
    lws_callback_on_writable(it->second->wsi);

    // Wake up service thread
    lws_cancel_service(context_);

    return true;
}

void WebSocketServer::broadcast(const std::string& message) {
    std::lock_guard<std::mutex> lock(clients_mutex_);

    for (const auto& pair : clients_) {
        struct lws* wsi = pair.second->wsi;

        size_t len = message.length();
        unsigned char* buf = new unsigned char[LWS_PRE + len];

        memcpy(buf + LWS_PRE, message.c_str(), len);
        lws_write(wsi, buf + LWS_PRE, len, LWS_WRITE_TEXT);

        delete[] buf;
    }
}

// ============================================================================
// Request Queue
// ============================================================================

void WebSocketServer::enqueue_request(const std::string& client_id, const std::string& message) {
    std::lock_guard<std::mutex> lock(queue_mutex_);

    QueuedRequest req;
    req.client_id = client_id;
    req.message = message;
    req.timestamp = std::chrono::system_clock::now();

    request_queue_.push(req);
    queue_cv_.notify_one();  // Notify processor thread immediately
}

void WebSocketServer::process_queue_loop() {
    ConsoleLogger::log("Queue processor thread started");

    while (running_) {
        QueuedRequest req;

        {
            std::unique_lock<std::mutex> lock(queue_mutex_);

            // Wait for request to be available or stop signal
            queue_cv_.wait(lock, [this] { return !request_queue_.empty() || !running_; });

            // Check if we're stopping
            if (!running_) {
                break;
            }

            // Get request from queue
            if (!request_queue_.empty()) {
                req = request_queue_.front();
                request_queue_.pop();
            } else {
                continue;
            }
        }

        // Process request
        process_request(req);
    }

    ConsoleLogger::log("Queue processor thread stopped");
}

void WebSocketServer::process_request(const QueuedRequest& request) {
    // Try to parse as JSON to validate format
    try {
        auto j = json::parse(request.message);
        // Valid JSON - pass to message callback
        if (message_callback_) {
            message_callback_(request.client_id, request.message);
        }
    } catch (const json::exception& e) {
        // Invalid JSON - call error callback
        if (error_callback_) {
            std::string error_msg = "Parse error: " + std::string(e.what());
            error_callback_(request.client_id, error_msg);
        }
    }
}

// ============================================================================
// Authentication
// ============================================================================

bool WebSocketServer::check_authentication(const std::string& auth_header) {
    if (!require_auth_) {
        return true;  // No auth required
    }

    // Check for "Bearer <token>" format
    const std::string bearer_prefix = "Bearer ";
    if (auth_header.substr(0, bearer_prefix.length()) != bearer_prefix) {
        return false;
    }

    std::string token = auth_header.substr(bearer_prefix.length());
    return token == auth_token_;
}

// ============================================================================
// libwebsockets Callback
// ============================================================================

int WebSocketServer::lws_callback(struct lws* wsi, enum lws_callback_reasons reason, void* user,
                                  void* in, size_t len) {
    WebSocketServer* server = static_cast<WebSocketServer*>(lws_context_user(lws_get_context(wsi)));

    if (!server) {
        return 0;
    }

    switch (reason) {
    case LWS_CALLBACK_FILTER_PROTOCOL_CONNECTION: {
        // Called before WebSocket upgrade - check authentication here
        if (server->require_auth_) {
            char ip[128];
            lws_get_peer_simple(wsi, ip, sizeof(ip));

            // Get Authorization header during HTTP upgrade
            char auth_buf[256];
            int hdr_len =
                lws_hdr_copy(wsi, auth_buf, sizeof(auth_buf), WSI_TOKEN_HTTP_AUTHORIZATION);

            if (hdr_len <= 0 || !server->check_authentication(std::string(auth_buf, hdr_len))) {
                ConsoleLogger::log(("Authentication failed for " + std::string(ip)).c_str());
                return -1;  // Reject connection (< 0 = reject)
            }
        }
        break;
    }

    case LWS_CALLBACK_ESTABLISHED: {
        // Client connected (authentication already passed)
        char ip[128];
        lws_get_peer_simple(wsi, ip, sizeof(ip));
        server->add_client(wsi, ip);
        break;
    }

    case LWS_CALLBACK_RECEIVE: {
        // Message received
        auto client = server->find_client_by_wsi(wsi);
        if (!client) {
            return 0;
        }

        if (!client->authenticated && server->require_auth_) {
            ConsoleLogger::log("Unauthenticated client attempted to send message");
            return -1;
        }

        std::string message(static_cast<const char*>(in), len);

        // If sync callback is set, use synchronous response
        if (server->sync_message_callback_) {
            ConsoleLogger::log(("RECEIVE: Processing message from " + client->client_id + " (" +
                                std::to_string(message.length()) + " bytes)")
                                   .c_str());
            ConsoleLogger::log(("REQUEST: " + message.substr(0, 200)).c_str());

            std::string response = server->sync_message_callback_(client->client_id, message);

            // Check if response is empty (for notifications, which should not receive a response)
            if (response.empty()) {
                ConsoleLogger::log("RECEIVE: Empty response (notification), no response sent");
            } else {
                ConsoleLogger::log(("RECEIVE: Got response (" + std::to_string(response.length()) +
                                    " bytes), adding to send queue")
                                       .c_str());
                ConsoleLogger::log(("RESPONSE PREVIEW: " + response.substr(0, 300)).c_str());

                // Add response to client's send queue (direct access to avoid mutex nesting)
                {
                    std::lock_guard<std::mutex> send_lock(client->send_mutex);
                    client->send_queue.push(response);
                    ConsoleLogger::log(
                        ("RECEIVE: Queue size now: " + std::to_string(client->send_queue.size()))
                            .c_str());
                }

                // Request writable callback to trigger send
                lws_callback_on_writable(wsi);
                ConsoleLogger::log("RECEIVE: Requested WRITEABLE callback");

                // Wake up service thread
                lws_cancel_service(server->context_);
                ConsoleLogger::log("RECEIVE: Woke service thread");
            }

        } else {
            // Fallback to async queue
            server->enqueue_request(client->client_id, message);
        }
        break;
    }

    case LWS_CALLBACK_CLOSED: {
        // Client disconnected
        server->remove_client(wsi);
        break;
    }

    case LWS_CALLBACK_SERVER_WRITEABLE: {
        ConsoleLogger::log("WRITEABLE: Callback triggered");

        // Find client WITHOUT locking clients_mutex_ (to avoid deadlock)
        std::shared_ptr<ClientInfo> client;
        {
            std::lock_guard<std::mutex> lock(server->clients_mutex_);
            for (auto& pair : server->clients_) {
                if (pair.second->wsi == wsi) {
                    client = pair.second;
                    break;
                }
            }
        }

        if (!client) {
            ConsoleLogger::log("WRITEABLE: client not found");
            break;
        }

        // Check if there are messages to send
        std::string message;
        bool has_message = false;
        {
            std::lock_guard<std::mutex> send_lock(client->send_mutex);
            if (!client->send_queue.empty()) {
                message = client->send_queue.front();
                client->send_queue.pop();
                has_message = true;
                ConsoleLogger::log(("WRITEABLE: Dequeued message for " + client->client_id + " (" +
                                    std::to_string(message.length()) + " bytes)")
                                       .c_str());
            }
        }

        if (!has_message) {
            ConsoleLogger::log(("WRITEABLE: No messages for " + client->client_id).c_str());
            break;
        }

        // Allocate buffer with LWS_PRE padding
        size_t msg_len = message.length();
        unsigned char* buf = new unsigned char[LWS_PRE + msg_len];
        memcpy(buf + LWS_PRE, message.c_str(), msg_len);

        // Write message
        ConsoleLogger::log(("WRITEABLE: Calling lws_write for " + client->client_id).c_str());
        int written = lws_write(wsi, buf + LWS_PRE, msg_len, LWS_WRITE_TEXT);
        delete[] buf;

        if (written < 0) {
            ConsoleLogger::log(("WRITEABLE: lws_write FAILED for " + client->client_id).c_str());
        } else {
            ConsoleLogger::log(("WRITEABLE: Successfully sent " + std::to_string(written) +
                                " bytes to " + client->client_id)
                                   .c_str());
        }

        // If more messages in queue, request another callback
        bool has_more = false;
        {
            std::lock_guard<std::mutex> send_lock(client->send_mutex);
            has_more = !client->send_queue.empty();
            if (has_more) {
                ConsoleLogger::log(("WRITEABLE: Queue has " +
                                    std::to_string(client->send_queue.size()) +
                                    " more messages, requesting callback")
                                       .c_str());
            }
        }

        if (has_more) {
            lws_callback_on_writable(wsi);
        }

        break;
    }

    default:
        break;
    }

    return 0;
}
