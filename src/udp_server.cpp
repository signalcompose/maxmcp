/**
    @file udp_server.cpp
    UDP Server Implementation

    @ingroup maxmcp
*/

#include "udp_server.h"

#include "utils/console_logger.h"

#include <cstring>
#include <iomanip>
#include <sstream>
#include <unistd.h>

#include <arpa/inet.h>

// Fragment protocol constants
const size_t MAX_UDP_PAYLOAD = 60000;    // Leave room for headers
const char FRAGMENT_HEADER[] = "MCPF:";  // MCP Fragment header
const size_t FRAGMENT_HEADER_LEN = 5;

UDPServer::UDPServer(int port)
    : port_(port), socket_fd_(-1), running_(false), server_thread_(nullptr),
      client_addr_set_(false) {
    memset(&client_addr_, 0, sizeof(client_addr_));
}

UDPServer::~UDPServer() {
    stop();
}

bool UDPServer::start() {
    if (running_) {
        ConsoleLogger::log("UDP server already running");
        return false;
    }

    // Create UDP socket
    socket_fd_ = socket(AF_INET, SOCK_DGRAM, 0);
    if (socket_fd_ < 0) {
        ConsoleLogger::log("ERROR: Failed to create UDP socket");
        return false;
    }

    // Set socket options
    int reuse = 1;
    if (setsockopt(socket_fd_, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0) {
        ConsoleLogger::log("WARNING: Failed to set SO_REUSEADDR");
    }

    // Bind to port
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port_);

    if (bind(socket_fd_, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        ConsoleLogger::log(
            ("ERROR: Failed to bind UDP socket to port " + std::to_string(port_)).c_str());
        close(socket_fd_);
        socket_fd_ = -1;
        return false;
    }

    // Start server thread
    running_ = true;
    server_thread_ = new std::thread(&UDPServer::server_loop, this);

    ConsoleLogger::log(("UDP server started on port " + std::to_string(port_)).c_str());
    return true;
}

void UDPServer::stop() {
    if (!running_) {
        return;
    }

    running_ = false;

    // Close socket (this will wake up recvfrom)
    if (socket_fd_ >= 0) {
        close(socket_fd_);
        socket_fd_ = -1;
    }

    // Wait for thread to finish
    if (server_thread_ && server_thread_->joinable()) {
        server_thread_->join();
        delete server_thread_;
        server_thread_ = nullptr;
    }

    ConsoleLogger::log("UDP server stopped");
}

bool UDPServer::send_message(const std::string& message) {
    std::lock_guard<std::mutex> addr_lock(client_addr_mutex_);

    if (!client_addr_set_) {
        ConsoleLogger::log("WARNING: No client address set, cannot send message");
        return false;
    }

    // Check if fragmentation is needed
    if (message.length() <= MAX_UDP_PAYLOAD) {
        // Send as single packet
        ssize_t sent = sendto(socket_fd_, message.c_str(), message.length(), 0,
                              (struct sockaddr*)&client_addr_, sizeof(client_addr_));

        if (sent < 0) {
            ConsoleLogger::log("ERROR: Failed to send UDP message");
            return false;
        }

        ConsoleLogger::log(("UDP sent: " + std::to_string(message.length()) + " bytes").c_str());
        return true;
    } else {
        // Send as fragments
        send_fragmented(message, client_addr_);
        return true;
    }
}

bool UDPServer::get_received_message(std::string& message) {
    std::lock_guard<std::mutex> lock(receive_mutex_);
    if (receive_queue_.empty()) {
        return false;
    }

    message = receive_queue_.front();
    receive_queue_.pop();
    return true;
}

void UDPServer::server_loop() {
    char buffer[65536];
    struct sockaddr_in from_addr;
    socklen_t from_len = sizeof(from_addr);

    ConsoleLogger::log("UDP server loop started");

    while (running_) {
        ssize_t received = recvfrom(socket_fd_, buffer, sizeof(buffer) - 1, 0,
                                    (struct sockaddr*)&from_addr, &from_len);

        if (received < 0) {
            if (running_) {
                ConsoleLogger::log("ERROR: UDP recvfrom failed");
            }
            break;
        }

        if (received == 0) {
            continue;
        }

        // Save client address for responses
        {
            std::lock_guard<std::mutex> lock(client_addr_mutex_);
            client_addr_ = from_addr;
            client_addr_set_ = true;
        }

        // Check if this is a fragment
        if (received >= FRAGMENT_HEADER_LEN &&
            memcmp(buffer, FRAGMENT_HEADER, FRAGMENT_HEADER_LEN) == 0) {
            // Handle fragment
            if (handle_fragment(buffer, received, from_addr)) {
                // Complete message received, already added to queue
            }
        } else {
            // Complete message in single packet
            buffer[received] = '\0';
            std::string message(buffer, received);

            ConsoleLogger::log(("UDP received: " + std::to_string(received) + " bytes").c_str());

            // Add to receive queue
            {
                std::lock_guard<std::mutex> lock(receive_mutex_);
                receive_queue_.push(message);
            }

            // Call callback if set
            if (message_callback_) {
                message_callback_(message);
            }
        }
    }

    ConsoleLogger::log("UDP server loop ended");
}

bool UDPServer::handle_fragment(const char* data, size_t len, const struct sockaddr_in& from_addr) {
    // Fragment format: "MCPF:msg_id:frag_index:total_frags:data"
    std::string fragment(data, len);

    // Parse header
    size_t pos1 = fragment.find(':', FRAGMENT_HEADER_LEN);
    size_t pos2 = fragment.find(':', pos1 + 1);
    size_t pos3 = fragment.find(':', pos2 + 1);

    if (pos1 == std::string::npos || pos2 == std::string::npos || pos3 == std::string::npos) {
        ConsoleLogger::log("ERROR: Invalid fragment format");
        return false;
    }

    std::string msg_id = fragment.substr(FRAGMENT_HEADER_LEN, pos1 - FRAGMENT_HEADER_LEN);
    int frag_index = std::stoi(fragment.substr(pos1 + 1, pos2 - pos1 - 1));
    int total_frags = std::stoi(fragment.substr(pos2 + 1, pos3 - pos2 - 1));
    std::string frag_data = fragment.substr(pos3 + 1);

    ConsoleLogger::log(("UDP fragment received: " + msg_id + " (" + std::to_string(frag_index) +
                        "/" + std::to_string(total_frags) + ")")
                           .c_str());

    // Store fragment
    std::lock_guard<std::mutex> lock(fragment_mutex_);

    auto& info = fragment_map_[msg_id];
    if (info.total_fragments == 0) {
        info.total_fragments = total_frags;
        info.data.resize(MAX_UDP_PAYLOAD * total_frags);  // Pre-allocate
        info.received_fragments = 0;
    }

    // Copy fragment data
    size_t offset = frag_index * MAX_UDP_PAYLOAD;
    if (offset + frag_data.length() <= info.data.size()) {
        memcpy(&info.data[offset], frag_data.c_str(), frag_data.length());
        info.received_fragments++;
        info.last_update = time(nullptr);
    }

    // Check if all fragments received
    if (info.received_fragments == info.total_fragments) {
        // Reconstruct complete message
        std::string complete_message = info.data.substr(0, frag_data.length() + offset);

        ConsoleLogger::log(("UDP complete message reassembled: " +
                            std::to_string(complete_message.length()) + " bytes")
                               .c_str());

        // Add to receive queue
        {
            std::lock_guard<std::mutex> recv_lock(receive_mutex_);
            receive_queue_.push(complete_message);
        }

        // Call callback
        if (message_callback_) {
            message_callback_(complete_message);
        }

        // Remove from fragment map
        fragment_map_.erase(msg_id);
        return true;
    }

    return false;
}

void UDPServer::send_fragmented(const std::string& message, const struct sockaddr_in& to_addr) {
    size_t total_len = message.length();
    int total_frags = (total_len + MAX_UDP_PAYLOAD - 1) / MAX_UDP_PAYLOAD;

    // Generate message ID
    std::stringstream ss;
    ss << std::hex << std::setfill('0') << std::setw(8) << time(nullptr);
    std::string msg_id = ss.str();

    ConsoleLogger::log(("UDP sending fragmented message: " + std::to_string(total_len) +
                        " bytes in " + std::to_string(total_frags) + " fragments")
                           .c_str());

    for (int i = 0; i < total_frags; i++) {
        size_t offset = i * MAX_UDP_PAYLOAD;
        size_t frag_len = std::min(MAX_UDP_PAYLOAD, total_len - offset);

        // Build fragment: "MCPF:msg_id:frag_index:total_frags:data"
        std::string fragment = std::string(FRAGMENT_HEADER) + msg_id + ":" + std::to_string(i) +
                               ":" + std::to_string(total_frags) + ":" +
                               message.substr(offset, frag_len);

        ssize_t sent = sendto(socket_fd_, fragment.c_str(), fragment.length(), 0,
                              (struct sockaddr*)&to_addr, sizeof(to_addr));

        if (sent < 0) {
            ConsoleLogger::log(("ERROR: Failed to send fragment " + std::to_string(i)).c_str());
            return;
        }

        // Small delay between fragments to avoid packet loss
        usleep(1000);  // 1ms
    }

    ConsoleLogger::log("UDP fragmented message sent");
}
