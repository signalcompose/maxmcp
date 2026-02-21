/**
 * @file test_tool_routing.cpp
 * Unit tests for MCP tool schema definitions and routing logic.
 *
 * Tests the actual tool module code (not mocks) under MAXMCP_TEST_MODE:
 * - Schema completeness: all 24 tools registered with valid JSON Schema
 * - Routing: MCPServer dispatches to correct module
 * - Test mode consistency: all modules return proper errors in test mode
 * - Parameter validation: missing required params produce error responses
 */

#include "mcp_server.h"
#include "tools/connection_tools.h"
#include "tools/hierarchy_tools.h"
#include "tools/object_tools.h"
#include "tools/patch_tools.h"
#include "tools/state_tools.h"
#include "tools/tool_common.h"
#include "tools/utility_tools.h"

#include <gtest/gtest.h>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

// ============================================================================
// Schema Validation Tests
// ============================================================================

class ToolSchemaTest : public ::testing::Test {
  protected:
    // Validate that a single tool schema has the required MCP fields
    void validate_tool_schema(const json& schema) {
        ASSERT_TRUE(schema.contains("name")) << "Schema missing 'name': " << schema.dump();
        ASSERT_TRUE(schema["name"].is_string());
        ASSERT_FALSE(schema["name"].get<std::string>().empty());

        ASSERT_TRUE(schema.contains("description"))
            << "Schema missing 'description' for tool: " << schema["name"];
        ASSERT_TRUE(schema["description"].is_string());

        ASSERT_TRUE(schema.contains("inputSchema"))
            << "Schema missing 'inputSchema' for tool: " << schema["name"];
        auto& input = schema["inputSchema"];
        ASSERT_TRUE(input.is_object());
        EXPECT_EQ(input.value("type", ""), "object")
            << "inputSchema.type must be 'object' for tool: " << schema["name"];
    }
};

TEST_F(ToolSchemaTest, PatchToolsSchemaCount) {
    auto schemas = PatchTools::get_tool_schemas();
    ASSERT_EQ(schemas.size(), 3)
        << "PatchTools should have 3 tools (list_active_patches, get_patch_info, "
           "get_frontmost_patch)";
}

TEST_F(ToolSchemaTest, ObjectToolsSchemaCount) {
    auto schemas = ObjectTools::get_tool_schemas();
    ASSERT_EQ(schemas.size(), 10)
        << "ObjectTools should have 10 tools (add, remove, get_objects, set_attr, "
           "get_io, get_hidden, set_hidden, redraw, replace_text, assign_varnames)";
}

TEST_F(ToolSchemaTest, ConnectionToolsSchemaCount) {
    auto schemas = ConnectionTools::get_tool_schemas();
    ASSERT_EQ(schemas.size(), 4) << "ConnectionTools should have 4 tools (connect, disconnect, "
                                    "get_patchlines, set_patchline_midpoints)";
}

TEST_F(ToolSchemaTest, StateToolsSchemaCount) {
    auto schemas = StateTools::get_tool_schemas();
    ASSERT_EQ(schemas.size(), 3)
        << "StateTools should have 3 tools (get_lock, set_lock, get_dirty)";
}

TEST_F(ToolSchemaTest, HierarchyToolsSchemaCount) {
    auto schemas = HierarchyTools::get_tool_schemas();
    ASSERT_EQ(schemas.size(), 2)
        << "HierarchyTools should have 2 tools (get_parent, get_subpatchers)";
}

TEST_F(ToolSchemaTest, UtilityToolsSchemaCount) {
    auto schemas = UtilityTools::get_tool_schemas();
    ASSERT_EQ(schemas.size(), 2)
        << "UtilityTools should have 2 tools (get_console_log, get_avoid_rect)";
}

TEST_F(ToolSchemaTest, TotalToolCount) {
    size_t total =
        PatchTools::get_tool_schemas().size() + ObjectTools::get_tool_schemas().size() +
        ConnectionTools::get_tool_schemas().size() + StateTools::get_tool_schemas().size() +
        HierarchyTools::get_tool_schemas().size() + UtilityTools::get_tool_schemas().size();
    EXPECT_EQ(total, 24) << "Total tool count should be 24";
}

TEST_F(ToolSchemaTest, AllSchemasHaveRequiredFields) {
    auto modules = {PatchTools::get_tool_schemas(),      ObjectTools::get_tool_schemas(),
                    ConnectionTools::get_tool_schemas(), StateTools::get_tool_schemas(),
                    HierarchyTools::get_tool_schemas(),  UtilityTools::get_tool_schemas()};

    for (const auto& schemas : modules) {
        for (const auto& schema : schemas) {
            validate_tool_schema(schema);
        }
    }
}

TEST_F(ToolSchemaTest, AllToolNamesAreUnique) {
    std::set<std::string> names;
    auto modules = {PatchTools::get_tool_schemas(),      ObjectTools::get_tool_schemas(),
                    ConnectionTools::get_tool_schemas(), StateTools::get_tool_schemas(),
                    HierarchyTools::get_tool_schemas(),  UtilityTools::get_tool_schemas()};

    for (const auto& schemas : modules) {
        for (const auto& schema : schemas) {
            std::string name = schema["name"];
            EXPECT_TRUE(names.find(name) == names.end()) << "Duplicate tool name: " << name;
            names.insert(name);
        }
    }
}

TEST_F(ToolSchemaTest, ExpectedToolNamesPresent) {
    std::set<std::string> names;
    auto modules = {PatchTools::get_tool_schemas(),      ObjectTools::get_tool_schemas(),
                    ConnectionTools::get_tool_schemas(), StateTools::get_tool_schemas(),
                    HierarchyTools::get_tool_schemas(),  UtilityTools::get_tool_schemas()};

    for (const auto& schemas : modules) {
        for (const auto& schema : schemas) {
            names.insert(schema["name"]);
        }
    }

    // Verify all expected tool names
    std::vector<std::string> expected = {
        "list_active_patches",     "get_patch_info",          "get_frontmost_patch",
        "add_max_object",          "remove_max_object",       "get_objects_in_patch",
        "set_object_attribute",    "get_object_io_info",      "get_object_hidden",
        "set_object_hidden",       "redraw_object",           "connect_max_objects",
        "disconnect_max_objects",  "get_patch_lock_state",    "set_patch_lock_state",
        "get_patch_dirty",         "get_parent_patcher",      "get_subpatchers",
        "get_console_log",         "get_avoid_rect_position", "get_patchlines",
        "set_patchline_midpoints", "replace_object_text",     "assign_varnames"};

    for (const auto& name : expected) {
        EXPECT_TRUE(names.count(name)) << "Missing expected tool: " << name;
    }
}

TEST_F(ToolSchemaTest, InputSchemaHasPropertiesField) {
    auto modules = {PatchTools::get_tool_schemas(),      ObjectTools::get_tool_schemas(),
                    ConnectionTools::get_tool_schemas(), StateTools::get_tool_schemas(),
                    HierarchyTools::get_tool_schemas(),  UtilityTools::get_tool_schemas()};

    for (const auto& schemas : modules) {
        for (const auto& schema : schemas) {
            auto& input = schema["inputSchema"];
            EXPECT_TRUE(input.contains("properties"))
                << "inputSchema missing 'properties' for tool: " << schema["name"];
        }
    }
}

// ============================================================================
// MCPServer Routing Tests
// ============================================================================

class MCPServerRoutingTest : public ::testing::Test {
  protected:
    MCPServer* server;

    void SetUp() override {
        server = MCPServer::get_instance();
        server->start();
    }

    void TearDown() override {
        MCPServer::destroy_instance();
    }

    json call_tool(const std::string& name, const json& arguments = json::object()) {
        json request = {{"jsonrpc", "2.0"},
                        {"id", 1},
                        {"method", "tools/call"},
                        {"params", {{"name", name}, {"arguments", arguments}}}};
        std::string response_str = server->handle_request_string(request.dump());
        return json::parse(response_str);
    }

    json send_request(const json& request) {
        std::string response_str = server->handle_request_string(request.dump());
        return json::parse(response_str);
    }
};

TEST_F(MCPServerRoutingTest, ToolsListReturnsAllTools) {
    json request = {{"jsonrpc", "2.0"}, {"id", 1}, {"method", "tools/list"}};
    auto response = send_request(request);

    ASSERT_TRUE(response.contains("result"));
    ASSERT_TRUE(response["result"].contains("tools"));

    auto& tools = response["result"]["tools"];
    ASSERT_TRUE(tools.is_array());
    EXPECT_EQ(tools.size(), 24) << "tools/list should return all 24 tools";
}

TEST_F(MCPServerRoutingTest, ToolsListResponseFormat) {
    json request = {{"jsonrpc", "2.0"}, {"id", 42}, {"method", "tools/list"}};
    auto response = send_request(request);

    EXPECT_EQ(response["jsonrpc"], "2.0");
    EXPECT_EQ(response["id"], 42);
    ASSERT_TRUE(response.contains("result"));
}

TEST_F(MCPServerRoutingTest, InitializeResponse) {
    json request = {{"jsonrpc", "2.0"},
                    {"id", 1},
                    {"method", "initialize"},
                    {"params", {{"protocolVersion", "2024-11-05"}}}};
    auto response = send_request(request);

    ASSERT_TRUE(response.contains("result"));
    EXPECT_EQ(response["result"]["protocolVersion"], "2024-11-05");
    EXPECT_EQ(response["result"]["serverInfo"]["name"], "maxmcp");
    EXPECT_TRUE(response["result"]["capabilities"].contains("tools"));
}

TEST_F(MCPServerRoutingTest, UnknownMethodError) {
    json request = {{"jsonrpc", "2.0"}, {"id", 1}, {"method", "unknown/method"}};
    auto response = send_request(request);

    ASSERT_TRUE(response.contains("error"));
    EXPECT_EQ(response["error"]["code"], ToolCommon::ErrorCode::METHOD_NOT_FOUND);
}

TEST_F(MCPServerRoutingTest, MissingToolNameError) {
    json request = {
        {"jsonrpc", "2.0"}, {"id", 1}, {"method", "tools/call"}, {"params", json::object()}};
    auto response = send_request(request);

    ASSERT_TRUE(response.contains("error"));
    EXPECT_EQ(response["error"]["code"], ToolCommon::ErrorCode::INVALID_PARAMS);
}

TEST_F(MCPServerRoutingTest, UnknownToolError) {
    auto response = call_tool("nonexistent_tool");

    // In test mode, ObjectTools::execute returns ToolCommon::ErrorCode::INTERNAL_ERROR for all
    // tools (including unknown ones), so the routing stops there. The error code depends on test
    // mode behavior.
    ASSERT_TRUE(response.contains("error"));
}

TEST_F(MCPServerRoutingTest, InvalidJSONParsing) {
    std::string response_str = server->handle_request_string("invalid json {");
    auto response = json::parse(response_str);

    ASSERT_TRUE(response.contains("error"));
    EXPECT_EQ(response["error"]["code"], ToolCommon::ErrorCode::PARSE_ERROR);
}

TEST_F(MCPServerRoutingTest, NotificationReturnsEmpty) {
    json request = {{"jsonrpc", "2.0"}, {"method", "notifications/initialized"}};
    std::string response_str = server->handle_request_string(request.dump());

    EXPECT_TRUE(response_str.empty()) << "Notification should return empty string";
}

// ============================================================================
// Test Mode Execute Tests
// ============================================================================

class TestModeExecuteTest : public ::testing::Test {};

TEST_F(TestModeExecuteTest, ObjectToolsReturnTestModeError) {
    auto result = ObjectTools::execute("add_max_object", json::object());
    ASSERT_TRUE(result.contains("error"));
    EXPECT_EQ(result["error"]["code"], ToolCommon::ErrorCode::INTERNAL_ERROR);
}

TEST_F(TestModeExecuteTest, StateToolsReturnTestModeError) {
    std::vector<std::string> tools = {"get_patch_lock_state", "set_patch_lock_state",
                                      "get_patch_dirty"};
    for (const auto& tool : tools) {
        auto result = StateTools::execute(tool, json::object());
        ASSERT_TRUE(result.contains("error"))
            << "StateTools::" << tool << " should return error in test mode";
        EXPECT_EQ(result["error"]["code"], ToolCommon::ErrorCode::INTERNAL_ERROR);
    }
}

TEST_F(TestModeExecuteTest, HierarchyToolsReturnTestModeError) {
    std::vector<std::string> tools = {"get_parent_patcher", "get_subpatchers"};
    for (const auto& tool : tools) {
        auto result = HierarchyTools::execute(tool, json::object());
        ASSERT_TRUE(result.contains("error"))
            << "HierarchyTools::" << tool << " should return error in test mode";
        EXPECT_EQ(result["error"]["code"], ToolCommon::ErrorCode::INTERNAL_ERROR);
    }
}

TEST_F(TestModeExecuteTest, UnknownToolReturnsNull) {
    // Modules should return nullptr for tools they don't handle
    EXPECT_TRUE(PatchTools::execute("nonexistent", json::object()).is_null());
    EXPECT_TRUE(ConnectionTools::execute("nonexistent", json::object()).is_null());
    EXPECT_TRUE(StateTools::execute("nonexistent", json::object()).is_null());
    EXPECT_TRUE(HierarchyTools::execute("nonexistent", json::object()).is_null());
    EXPECT_TRUE(UtilityTools::execute("nonexistent", json::object()).is_null());
}

// ============================================================================
// Parameter Validation Tests (PatchTools - works in test mode via PatchRegistry)
// ============================================================================

class PatchToolsValidationTest : public ::testing::Test {};

TEST_F(PatchToolsValidationTest, GetPatchInfoMissingPatchId) {
    auto result = PatchTools::execute("get_patch_info", json::object());
    ASSERT_TRUE(result.contains("error"));
    EXPECT_EQ(result["error"]["code"], ToolCommon::ErrorCode::INVALID_PARAMS);
}

TEST_F(PatchToolsValidationTest, GetPatchInfoNotFound) {
    json params = {{"patch_id", "nonexistent_patch_id"}};
    auto result = PatchTools::execute("get_patch_info", params);
    ASSERT_TRUE(result.contains("error"));
    EXPECT_EQ(result["error"]["code"], ToolCommon::ErrorCode::INVALID_PARAMS);
}

TEST_F(PatchToolsValidationTest, ListActivePatchesEmpty) {
    auto result = PatchTools::execute("list_active_patches", json::object());
    // Should succeed even with no patches
    ASSERT_FALSE(result.is_null());
    EXPECT_FALSE(result.contains("error"));
}

// ============================================================================
// Connection Tools Test Mode (returns test_mode_error for all known tools)
// ============================================================================

class ConnectionToolsTestModeTest : public ::testing::Test {};

TEST_F(ConnectionToolsTestModeTest, ConnectReturnsTestModeError) {
    auto result = ConnectionTools::execute("connect_max_objects", json::object());
    ASSERT_TRUE(result.contains("error"));
    EXPECT_EQ(result["error"]["code"], ToolCommon::ErrorCode::INTERNAL_ERROR);
}

TEST_F(ConnectionToolsTestModeTest, DisconnectReturnsTestModeError) {
    auto result = ConnectionTools::execute("disconnect_max_objects", json::object());
    ASSERT_TRUE(result.contains("error"));
    EXPECT_EQ(result["error"]["code"], ToolCommon::ErrorCode::INTERNAL_ERROR);
}

TEST_F(ConnectionToolsTestModeTest, GetPatchlinesReturnsTestModeError) {
    auto result = ConnectionTools::execute("get_patchlines", json::object());
    ASSERT_TRUE(result.contains("error"));
    EXPECT_EQ(result["error"]["code"], ToolCommon::ErrorCode::INTERNAL_ERROR);
}

TEST_F(ConnectionToolsTestModeTest, SetPatchlineMidpointsReturnsTestModeError) {
    auto result = ConnectionTools::execute("set_patchline_midpoints", json::object());
    ASSERT_TRUE(result.contains("error"));
    EXPECT_EQ(result["error"]["code"], ToolCommon::ErrorCode::INTERNAL_ERROR);
}

TEST_F(ConnectionToolsTestModeTest, UnknownToolReturnsNull) {
    auto result = ConnectionTools::execute("unknown_tool", json::object());
    EXPECT_TRUE(result.is_null());
}

// ============================================================================
// Utility Tools Tests (get_console_log works in test mode)
// ============================================================================

class UtilityToolsTest : public ::testing::Test {};

TEST_F(UtilityToolsTest, GetConsoleLogWorks) {
    auto result = UtilityTools::execute("get_console_log", json::object());
    ASSERT_FALSE(result.is_null());
    // ConsoleLogger::get_logs returns {count, logs} directly (not wrapped in "result")
    EXPECT_TRUE(result.contains("count"));
    EXPECT_TRUE(result.contains("logs"));
}

TEST_F(UtilityToolsTest, GetAvoidRectReturnsTestModeError) {
    auto result = UtilityTools::execute("get_avoid_rect_position", json::object());
    ASSERT_TRUE(result.contains("error"));
    EXPECT_EQ(result["error"]["code"], ToolCommon::ErrorCode::INTERNAL_ERROR);
}
