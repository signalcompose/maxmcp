/**
    @file mcp_server.cpp
    MaxMCP - MCP Server Implementation

    This file contains the core MCP server routing logic.
    Tool implementations are in separate modules under src/tools/.

    @ingroup maxmcp
*/

#include "mcp_server.h"

#include "utils/console_logger.h"

// Tool module headers
#include "tools/connection_tools.h"
#include "tools/hierarchy_tools.h"
#include "tools/object_tools.h"
#include "tools/patch_tools.h"
#include "tools/state_tools.h"
#include "tools/tool_common.h"
#include "tools/utility_tools.h"

#include <fstream>
#include <iostream>
#include <sstream>

// Static member initialization
MCPServer* MCPServer::instance_ = nullptr;

// ============================================================================
// MCPServer Singleton Management
// ============================================================================

MCPServer* MCPServer::get_instance() {
    if (instance_ == nullptr) {
        instance_ = new MCPServer();
    }
    return instance_;
}

void MCPServer::destroy_instance() {
    if (instance_ != nullptr) {
        instance_->stop();
        delete instance_;
        instance_ = nullptr;
    }
}

// ============================================================================
// Server Lifecycle
// ============================================================================

void MCPServer::start() {
    if (running_.load()) {
        ConsoleLogger::log("MCP Server already running");
        return;
    }

    running_.store(true);
    ConsoleLogger::log("MCP Server initialized (ready for requests)");

    // Note: stdio communication is handled by maxmcp.server.mxo
    // This external object manages stdin/stdout through its own thread
    // MCPServer only provides request handling via handle_request_string()
}

void MCPServer::stop() {
    if (!running_.load()) {
        return;
    }

    ConsoleLogger::log("MCP Server stopping...");
    running_.store(false);

    // Note: No io_thread_ to join - stdio is managed by maxmcp.server.mxo

    ConsoleLogger::log("MCP Server stopped");
}

// ============================================================================
// Tool Schema Aggregation
// ============================================================================

/**
 * @brief Aggregate tool schemas from all tool modules.
 *
 * Collects schemas from:
 * - PatchTools (3 tools)
 * - ObjectTools (8 tools)
 * - ConnectionTools (2 tools)
 * - StateTools (3 tools)
 * - HierarchyTools (2 tools)
 * - UtilityTools (2 tools)
 *
 * @return JSON array of all tool schemas
 */
static json get_all_tool_schemas() {
    json all_tools = json::array();

    // Collect schemas from each module
    auto patch_schemas = PatchTools::get_tool_schemas();
    auto object_schemas = ObjectTools::get_tool_schemas();
    auto connection_schemas = ConnectionTools::get_tool_schemas();
    auto state_schemas = StateTools::get_tool_schemas();
    auto hierarchy_schemas = HierarchyTools::get_tool_schemas();
    auto utility_schemas = UtilityTools::get_tool_schemas();

    // Merge all schemas
    for (const auto& schema : patch_schemas) {
        all_tools.push_back(schema);
    }
    for (const auto& schema : object_schemas) {
        all_tools.push_back(schema);
    }
    for (const auto& schema : connection_schemas) {
        all_tools.push_back(schema);
    }
    for (const auto& schema : state_schemas) {
        all_tools.push_back(schema);
    }
    for (const auto& schema : hierarchy_schemas) {
        all_tools.push_back(schema);
    }
    for (const auto& schema : utility_schemas) {
        all_tools.push_back(schema);
    }

    return all_tools;
}

// ============================================================================
// MCP Protocol Handling
// ============================================================================

json MCPServer::handle_request(const json& req) {
    std::string method = req.value("method", "");

    ConsoleLogger::log(("MCP: Received request: " + method).c_str());

    if (method == "initialize") {
        // MCP initialization
        ConsoleLogger::log("MCP: Processing initialize request");

        // Get client's protocol version
        std::string client_protocol_version = "2024-11-05";  // Default
        if (req.contains("params") && req["params"].contains("protocolVersion")) {
            client_protocol_version = req["params"]["protocolVersion"].get<std::string>();
        }

        return {{"jsonrpc", "2.0"},
                {"id", req.contains("id") ? req["id"] : nullptr},
                {"result",
                 {{"protocolVersion", client_protocol_version},  // Echo client's version
                  {"capabilities", {{"tools", {{"listChanged", true}}}}},
                  {"serverInfo", {{"name", "maxmcp"}, {"version", "1.1.0"}}}}}};

    } else if (method == "tools/list") {
        // Return list of available tools (aggregated from all modules)
        ConsoleLogger::log("MCP: Handling tools/list request");

        auto tools = get_all_tool_schemas();

        json response = {{"jsonrpc", "2.0"},
                         {"id", req.contains("id") ? req["id"] : nullptr},
                         {"result", {{"tools", tools}}}};

        // Debug: Write full JSON to file for verification
        std::ofstream debug_file("/tmp/maxmcp_tools_list.json");
        if (debug_file.is_open()) {
            debug_file << response.dump(2);  // Pretty print with 2-space indent
            debug_file.close();
            ConsoleLogger::log("MCP: Full tools/list JSON written to /tmp/maxmcp_tools_list.json");
        }

        return response;

    } else if (method == "tools/call") {
        // Execute tool
        ConsoleLogger::log("MCP: Handling tools/call request");

        if (!req.contains("params") || !req["params"].contains("name")) {
            return {{"jsonrpc", "2.0"},
                    {"id", req.contains("id") ? req["id"] : nullptr},
                    {"error", {{"code", -32602}, {"message", "Missing tool name"}}}};
        }

        std::string tool_name = req["params"]["name"].get<std::string>();
        json arguments = req["params"].value("arguments", json::object());

        ConsoleLogger::log(("MCP: Calling tool: " + tool_name).c_str());

        json result = execute_tool(tool_name, arguments);

        // Handle errors
        if (result.contains("error")) {
            return {{"jsonrpc", "2.0"},
                    {"id", req.contains("id") ? req["id"] : nullptr},
                    {"error", result["error"]}};
        }

        // Return result as text content for MCP
        std::string result_text;
        if (result.contains("result")) {
            result_text = result["result"].dump();
        } else {
            result_text = result.dump();
        }

        return {{"jsonrpc", "2.0"},
                {"id", req.contains("id") ? req["id"] : nullptr},
                {"result",
                 {{"content", json::array({{{"type", "text"}, {"text", result_text}}})},
                  {"isError", false}}}};

    } else if (method == "notifications/initialized") {
        // Handle notifications/initialized - no response required
        ConsoleLogger::log("MCP: Received initialized notification");
        return nullptr;  // Return null to signal no response

    } else {
        // Unknown method
        ConsoleLogger::log(("MCP: Unknown method: " + method).c_str());
        return {{"jsonrpc", "2.0"},
                {"id", req.contains("id") ? req["id"] : nullptr},
                {"error", {{"code", -32601}, {"message", "Method not found: " + method}}}};
    }
}

// ============================================================================
// Tool Execution Router
// ============================================================================

json MCPServer::execute_tool(const std::string& tool, const json& params) {
    ConsoleLogger::log(("MCP: execute_tool called: " + tool).c_str());

    // Route to appropriate tool module
    json result;

    // Try PatchTools (list_active_patches, get_patch_info, get_frontmost_patch)
    result = PatchTools::execute(tool, params);
    if (!result.is_null()) {
        return result;
    }

    // Try ObjectTools (add_max_object, remove_max_object, get_objects_in_patch, etc.)
    result = ObjectTools::execute(tool, params);
    if (!result.is_null()) {
        return result;
    }

    // Try ConnectionTools (connect_max_objects, disconnect_max_objects)
    result = ConnectionTools::execute(tool, params);
    if (!result.is_null()) {
        return result;
    }

    // Try StateTools (get_patch_lock_state, set_patch_lock_state, etc.)
    result = StateTools::execute(tool, params);
    if (!result.is_null()) {
        return result;
    }

    // Try HierarchyTools (get_parent_patcher, get_subpatchers)
    result = HierarchyTools::execute(tool, params);
    if (!result.is_null()) {
        return result;
    }

    // Try UtilityTools (get_console_log, get_avoid_rect_position)
    result = UtilityTools::execute(tool, params);
    if (!result.is_null()) {
        return result;
    }

    // Tool not found in any module
    return ToolCommon::make_error(-32602, "Unknown tool: " + tool);
}

// ============================================================================
// String Interface (for WebSocket/stdio)
// ============================================================================

std::string MCPServer::handle_request_string(const std::string& request_str) {
    try {
        ConsoleLogger::log(("Received message (" + std::to_string(request_str.length()) +
                            " bytes): " + request_str)
                               .c_str());
        json req = json::parse(request_str);
        json response = handle_request(req);

        // Check if response is null (for notifications that should not receive a response)
        if (response.is_null()) {
            ConsoleLogger::log("MCP: No response required (notification)");
            return std::string();  // Return truly empty string (0 bytes)
        }

        return response.dump();
    } catch (const std::exception& e) {
        // Return JSON-RPC error
        ConsoleLogger::log(("Parse error on message: " + request_str).c_str());
        ConsoleLogger::log(("Exception: " + std::string(e.what())).c_str());
        json error_response = {
            {"jsonrpc", "2.0"},
            {"error", {{"code", -32700}, {"message", std::string("Parse error: ") + e.what()}}},
            {"id", nullptr}};
        return error_response.dump();
    }
}
