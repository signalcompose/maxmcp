/**
    @file mcp_server.h
    MaxMCP - MCP Server

    Singleton MCP (Model Context Protocol) server for stdio-based JSON-RPC communication.

    @ingroup maxmcp
*/

#ifndef MCP_SERVER_H
#define MCP_SERVER_H

#include <thread>
#include <atomic>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

/**
 * @brief MCP Server for stdio-based JSON-RPC communication
 *
 * Singleton class managing MCP protocol communication with Claude Code.
 * Runs IO thread for stdin/stdout, handles JSON-RPC requests, and executes tools.
 */
class MCPServer {
private:
    static MCPServer* instance_;

    std::thread io_thread_;
    std::atomic<bool> running_{false};

    /**
     * @brief Handle MCP JSON-RPC request
     *
     * Routes request to appropriate handler (tools/list, tools/call, etc.)
     *
     * @param req JSON-RPC request object
     * @return JSON-RPC response object
     */
    json handle_request(const json& req);

    /**
     * @brief Execute MCP tool
     *
     * Routes tool execution to specific tool implementation.
     *
     * @param tool Tool name
     * @param params Tool parameters
     * @return Tool result (JSON)
     */
    json execute_tool(const std::string& tool, const json& params);

    // Private constructor for singleton
    MCPServer() = default;

public:
    // Delete copy/move constructors
    MCPServer(const MCPServer&) = delete;
    MCPServer& operator=(const MCPServer&) = delete;

    /**
     * @brief Get singleton instance
     *
     * @return Pointer to MCPServer singleton
     */
    static MCPServer* get_instance();

    /**
     * @brief Destroy singleton instance
     *
     * Thread-safe. Stops server before destruction.
     */
    static void destroy_instance();

    /**
     * @brief Start MCP server IO thread
     *
     * Starts background thread reading from stdin and writing to stdout.
     * Non-blocking. Thread-safe.
     */
    void start();

    /**
     * @brief Stop MCP server IO thread
     *
     * Gracefully stops IO thread and waits for completion.
     * Thread-safe.
     */
    void stop();

    /**
     * @brief Check if server is running
     *
     * @return true if IO thread is active
     */
    bool is_running() const { return running_.load(); }

    /**
     * @brief Handle MCP request from string
     *
     * Public API for external objects (maxmcp.server) to send requests.
     * Parses JSON string, routes to internal handler, returns JSON string response.
     *
     * @param request_str JSON-RPC request string
     * @return JSON-RPC response string
     */
    std::string handle_request_string(const std::string& request_str);
};

#endif // MCP_SERVER_H
