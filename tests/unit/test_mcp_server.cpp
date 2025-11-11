/**
 * @file test_mcp_server.cpp
 * Unit tests for MCP Server request/response handling
 */

#include <gtest/gtest.h>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

// Note: Since MCPServer has dependencies on Max API and threading,
// we'll test the JSON-RPC protocol logic independently

/**
 * Test Fixture for MCP Server
 */
class MCPServerTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Setup test environment
    }

    void TearDown() override {
        // Clean up
    }

    /**
     * Helper: Simulate handle_request logic without Max API dependencies
     */
    json simulate_handle_request(const json& req) {
        std::string method = req.value("method", "");

        if (method == "tools/list") {
            return {
                {"jsonrpc", "2.0"},
                {"result", {
                    {"tools", json::array({
                        {
                            {"name", "get_console_log"},
                            {"description", "Retrieve recent Max Console messages"}
                        },
                        {
                            {"name", "list_active_patches"},
                            {"description", "List all registered MaxMCP client patches"}
                        },
                        {
                            {"name", "add_max_object"},
                            {"description", "Add a Max object to a patch"}
                        }
                    })}
                }}
            };
        } else if (method == "tools/call") {
            // Safely check for required parameter
            if (!req.contains("params") || !req["params"].contains("name")) {
                return {
                    {"jsonrpc", "2.0"},
                    {"error", {
                        {"code", -32602},
                        {"message", "Invalid params: missing 'name' field"}
                    }}
                };
            }

            std::string tool_name = req["params"]["name"];

            if (tool_name == "get_console_log") {
                return {
                    {"jsonrpc", "2.0"},
                    {"result", {
                        {"logs", json::array()},
                        {"count", 0}
                    }}
                };
            } else if (tool_name == "list_active_patches") {
                return {
                    {"jsonrpc", "2.0"},
                    {"result", {
                        {"patches", json::array()},
                        {"count", 0}
                    }}
                };
            } else {
                return {
                    {"jsonrpc", "2.0"},
                    {"error", {
                        {"code", -32602},
                        {"message", "Unknown tool: " + tool_name}
                    }}
                };
            }
        } else {
            return {
                {"jsonrpc", "2.0"},
                {"error", {
                    {"code", -32601},
                    {"message", "Method not found"}
                }}
            };
        }
    }
};

/**
 * Test: tools/list request
 */
TEST_F(MCPServerTest, ToolsListRequest) {
    json request = {
        {"jsonrpc", "2.0"},
        {"id", 1},
        {"method", "tools/list"}
    };

    auto response = simulate_handle_request(request);

    EXPECT_EQ(response["jsonrpc"], "2.0");
    EXPECT_TRUE(response.contains("result"));
    EXPECT_TRUE(response["result"].contains("tools"));

    auto tools = response["result"]["tools"];
    EXPECT_TRUE(tools.is_array());
    EXPECT_GE(tools.size(), 3); // At least 3 tools

    // Verify tool names
    bool has_console_log = false;
    bool has_list_patches = false;
    bool has_add_object = false;

    for (const auto& tool : tools) {
        std::string name = tool["name"];
        if (name == "get_console_log") has_console_log = true;
        if (name == "list_active_patches") has_list_patches = true;
        if (name == "add_max_object") has_add_object = true;
    }

    EXPECT_TRUE(has_console_log);
    EXPECT_TRUE(has_list_patches);
    EXPECT_TRUE(has_add_object);
}

/**
 * Test: tools/call request with get_console_log
 */
TEST_F(MCPServerTest, ToolsCallGetConsoleLog) {
    json request = {
        {"jsonrpc", "2.0"},
        {"id", 2},
        {"method", "tools/call"},
        {"params", {
            {"name", "get_console_log"},
            {"arguments", {
                {"lines", 50},
                {"clear", false}
            }}
        }}
    };

    auto response = simulate_handle_request(request);

    EXPECT_EQ(response["jsonrpc"], "2.0");
    EXPECT_TRUE(response.contains("result"));
    EXPECT_TRUE(response["result"].contains("logs"));
    EXPECT_TRUE(response["result"]["logs"].is_array());
}

/**
 * Test: tools/call request with list_active_patches
 */
TEST_F(MCPServerTest, ToolsCallListPatches) {
    json request = {
        {"jsonrpc", "2.0"},
        {"id", 3},
        {"method", "tools/call"},
        {"params", {
            {"name", "list_active_patches"},
            {"arguments", {}}
        }}
    };

    auto response = simulate_handle_request(request);

    EXPECT_EQ(response["jsonrpc"], "2.0");
    EXPECT_TRUE(response.contains("result"));
    EXPECT_TRUE(response["result"].contains("patches"));
    EXPECT_TRUE(response["result"]["patches"].is_array());
}

/**
 * Test: Unknown method error
 */
TEST_F(MCPServerTest, UnknownMethod) {
    json request = {
        {"jsonrpc", "2.0"},
        {"id", 4},
        {"method", "unknown/method"}
    };

    auto response = simulate_handle_request(request);

    EXPECT_EQ(response["jsonrpc"], "2.0");
    EXPECT_TRUE(response.contains("error"));
    EXPECT_EQ(response["error"]["code"], -32601);
    EXPECT_EQ(response["error"]["message"], "Method not found");
}

/**
 * Test: Unknown tool error
 */
TEST_F(MCPServerTest, UnknownTool) {
    json request = {
        {"jsonrpc", "2.0"},
        {"id", 5},
        {"method", "tools/call"},
        {"params", {
            {"name", "unknown_tool"},
            {"arguments", {}}
        }}
    };

    auto response = simulate_handle_request(request);

    EXPECT_EQ(response["jsonrpc"], "2.0");
    EXPECT_TRUE(response.contains("error"));
    EXPECT_EQ(response["error"]["code"], -32602);
}

/**
 * Test: JSON parsing (valid JSON)
 */
TEST_F(MCPServerTest, ValidJSONParsing) {
    std::string line = R"({"jsonrpc":"2.0","id":1,"method":"tools/list"})";

    EXPECT_NO_THROW({
        auto request = json::parse(line);
        EXPECT_EQ(request["method"], "tools/list");
    });
}

/**
 * Test: JSON parsing (invalid JSON)
 */
TEST_F(MCPServerTest, InvalidJSONParsing) {
    std::string line = "invalid json {";

    EXPECT_THROW({
        auto request = json::parse(line);
    }, json::parse_error);
}

/**
 * Test: JSON-RPC response format
 */
TEST_F(MCPServerTest, ResponseFormat) {
    json request = {
        {"jsonrpc", "2.0"},
        {"id", 1},
        {"method", "tools/list"}
    };

    auto response = simulate_handle_request(request);

    // Must have jsonrpc field
    EXPECT_TRUE(response.contains("jsonrpc"));
    EXPECT_EQ(response["jsonrpc"], "2.0");

    // Must have either result or error
    EXPECT_TRUE(response.contains("result") || response.contains("error"));
}

/**
 * Test: Parameter validation
 */
TEST_F(MCPServerTest, ParameterValidation) {
    // Missing required parameter
    json request = {
        {"jsonrpc", "2.0"},
        {"id", 6},
        {"method", "tools/call"},
        {"params", {
            // Missing "name" field
            {"arguments", {}}
        }}
    };

    // Should handle gracefully (implementation dependent)
    EXPECT_NO_THROW({
        auto response = simulate_handle_request(request);
    });
}
