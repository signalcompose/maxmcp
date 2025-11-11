/**
    @file udp_server.h
    UDP Server for MaxMCP

    Simple UDP server for MCP protocol communication.
    Supports message fragmentation for large JSON messages.

    @ingroup maxmcp
*/

#ifndef UDP_SERVER_H
#define UDP_SERVER_H

#include <atomic>
#include <functional>
#include <map>
#include <mutex>
#include <queue>
#include <string>
#include <thread>

#include <netinet/in.h>
#include <sys/socket.h>

/**
 * @brief UDP Server for MCP communication
 *
 * Handles UDP communication with message fragmentation support.
 * Thread-safe implementation with message queues.
 */
class UDPServer {
  public:
    using MessageCallback = std::function<void(const std::string&)>;

    /**
     * @brief Constructor
     * @param port UDP port number
     */
    UDPServer(int port);

    /**
     * @brief Destructor
     */
    ~UDPServer();

    /**
     * @brief Start UDP server
     * @return true if started successfully
     */
    bool start();

    /**
     * @brief Stop UDP server
     */
    void stop();

    /**
     * @brief Check if server is running
     * @return true if running
     */
    bool is_running() const {
        return running_;
    }

    /**
     * @brief Send message to client
     * @param message Message to send
     * @return true if sent successfully
     */
    bool send_message(const std::string& message);

    /**
     * @brief Set message callback
     * @param callback Callback function called when message is received
     */
    void set_message_callback(MessageCallback callback) {
        message_callback_ = callback;
    }

    /**
     * @brief Get received message from queue
     * @param message Output parameter for message
     * @return true if message was available
     */
    bool get_received_message(std::string& message);

  private:
    int port_;
    int socket_fd_;
    std::atomic<bool> running_;
    std::thread* server_thread_;

    // Client address (for sending responses)
    struct sockaddr_in client_addr_;
    bool client_addr_set_;
    std::mutex client_addr_mutex_;

    // Message queues
    std::mutex receive_mutex_;
    std::queue<std::string> receive_queue_;

    std::mutex send_mutex_;
    std::queue<std::string> send_queue_;

    // Message callback
    MessageCallback message_callback_;

    // Fragmentation support
    struct FragmentInfo {
        std::string data;
        int total_fragments;
        int received_fragments;
        time_t last_update;
    };
    std::map<std::string, FragmentInfo> fragment_map_;
    std::mutex fragment_mutex_;

    // Server loop
    void server_loop();

    // Fragment handling
    bool handle_fragment(const char* data, size_t len, const struct sockaddr_in& from_addr);
    void send_fragmented(const std::string& message, const struct sockaddr_in& to_addr);
};

#endif  // UDP_SERVER_H
