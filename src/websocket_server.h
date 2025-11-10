/**
 * @file websocket_server.h
 * @brief WebSocket server for MaxMCP
 *
 * Implements WebSocket server using libwebsockets for MCP JSON-RPC communication.
 * Supports multiple clients, authentication, and request queueing.
 */

#ifndef WEBSOCKET_SERVER_H
#define WEBSOCKET_SERVER_H

#include <string>
#include <vector>
#include <queue>
#include <map>
#include <mutex>
#include <thread>
#include <functional>
#include <atomic>
#include <libwebsockets.h>

/**
 * Client connection information
 */
struct ClientInfo {
    std::string client_id;
    struct lws* wsi;
    std::string ip_address;
    bool authenticated;
    std::queue<std::string> send_queue;  // Queue for outgoing messages
    std::mutex send_mutex;               // Mutex for send_queue
};

/**
 * Queued request
 */
struct QueuedRequest {
    std::string client_id;
    std::string message;
    std::chrono::system_clock::time_point timestamp;
};

/**
 * WebSocket server class
 */
class WebSocketServer {
public:
    /**
     * Constructor
     * @param port Port to listen on
     * @param auth_token Optional authentication token (empty = no auth)
     */
    WebSocketServer(int port, const std::string& auth_token = "");

    /**
     * Destructor
     */
    ~WebSocketServer();

    /**
     * Start the server
     * @return true if started successfully
     */
    bool start();

    /**
     * Stop the server
     */
    void stop();

    /**
     * Check if server is running
     */
    bool is_running() const { return running_; }

    /**
     * Get listening port
     */
    int get_port() const { return port_; }

    /**
     * Get number of connected clients
     */
    size_t get_client_count() const;

    /**
     * Get list of connected client IDs
     */
    std::vector<std::string> get_connected_clients() const;

    /**
     * Send message to specific client
     * @param client_id Client ID
     * @param message Message to send
     * @return true if sent successfully
     */
    bool send_to_client(const std::string& client_id, const std::string& message);

    /**
     * Broadcast message to all clients
     * @param message Message to broadcast
     */
    void broadcast(const std::string& message);

    /**
     * Set message callback
     * Called when message is received from client
     */
    void set_message_callback(std::function<void(const std::string&, const std::string&)> callback) {
        message_callback_ = callback;
    }

    /**
     * Set error callback
     * Called when error occurs
     */
    void set_error_callback(std::function<void(const std::string&, const std::string&)> callback) {
        error_callback_ = callback;
    }

    /**
     * Set synchronous message callback
     * Called from LWS_CALLBACK_RECEIVE, should return response immediately
     */
    void set_sync_message_callback(std::function<std::string(const std::string&, const std::string&)> callback) {
        sync_message_callback_ = callback;
    }

    /**
     * libwebsockets callback (public for protocol registration)
     */
    static int lws_callback(struct lws* wsi, enum lws_callback_reasons reason,
                           void* user, void* in, size_t len);

private:
    // Configuration
    int port_;
    std::string auth_token_;
    bool require_auth_;

    // Server state
    std::atomic<bool> running_;
    struct lws_context* context_;
    std::thread service_thread_;

    // Client management
    std::map<std::string, std::shared_ptr<ClientInfo>> clients_;
    mutable std::mutex clients_mutex_;

    // Request queue
    std::queue<QueuedRequest> request_queue_;
    std::mutex queue_mutex_;
    std::condition_variable queue_cv_;
    std::thread queue_processor_thread_;

    // Callbacks
    std::function<void(const std::string&, const std::string&)> message_callback_;
    std::function<void(const std::string&, const std::string&)> error_callback_;
    std::function<std::string(const std::string&, const std::string&)> sync_message_callback_;

    // Internal methods
    void service_loop();
    void process_queue_loop();
    std::string generate_client_id();
    void add_client(struct lws* wsi, const std::string& ip);
    void remove_client(struct lws* wsi);
    std::shared_ptr<ClientInfo> find_client_by_wsi(struct lws* wsi);
    void enqueue_request(const std::string& client_id, const std::string& message);
    void process_request(const QueuedRequest& request);
    bool check_authentication(const std::string& auth_header);
};

#endif // WEBSOCKET_SERVER_H
