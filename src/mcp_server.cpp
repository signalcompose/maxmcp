/**
    @file mcp_server.cpp
    MaxMCP - MCP Server Implementation

    @ingroup maxmcp
*/

#include "mcp_server.h"
#include "utils/console_logger.h"
#include "utils/patch_registry.h"
#include "maxmcp.h"
#include "ext.h"
#include "ext_obex.h"
#include "jpatcher_api.h"
#include <iostream>
#include <sstream>

// Static member initialization
MCPServer* MCPServer::instance_ = nullptr;

// Structure for defer_low callback data
struct t_add_object_data {
    t_maxmcp* patch;
    std::string obj_type;
    double x;
    double y;
    std::string varname;
};

// Defer callback for adding Max object
static void add_object_deferred(t_maxmcp* patch, t_symbol* s, long argc, t_atom* argv) {
    t_add_object_data* data = (t_add_object_data*)argv;

    if (!data || !data->patch || !data->patch->patcher) {
        delete data;
        return;
    }

    // Create object with newobject_sprintf
    t_object* obj = (t_object*)newobject_sprintf(data->patch->patcher, "@maxclass %s @patching_rect %.2f %.2f 50.0 20.0",
                                                   data->obj_type.c_str(), data->x, data->y);

    if (obj) {
        // Set varname if provided
        if (!data->varname.empty()) {
            object_attr_setsym(obj, gensym("varname"), gensym(data->varname.c_str()));
        }

        ConsoleLogger::log(("Object created: " + data->obj_type).c_str());
    } else {
        ConsoleLogger::log(("Failed to create object: " + data->obj_type).c_str());
    }

    delete data;
}

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

void MCPServer::start() {
    if (running_.load()) {
        ConsoleLogger::log("MCP Server already running");
        return;
    }

    running_.store(true);
    ConsoleLogger::log("MCP Server starting IO thread...");

    io_thread_ = std::thread([this]() {
        while (running_.load()) {
            std::string line;

            // Read line from stdin
            if (!std::getline(std::cin, line)) {
                // EOF or error
                break;
            }

            try {
                // Parse JSON-RPC request
                auto request = json::parse(line);

                // Handle request
                auto response = handle_request(request);

                // Write response to stdout
                std::cout << response.dump() << "\n" << std::flush;

            } catch (const json::parse_error& e) {
                ConsoleLogger::log("MCP JSON parse error");
                // Send error response
                json error_response = {
                    {"jsonrpc", "2.0"},
                    {"error", {
                        {"code", -32700},
                        {"message", "Parse error"}
                    }}
                };
                std::cout << error_response.dump() << "\n" << std::flush;

            } catch (const std::exception& e) {
                std::string error_msg = "MCP error: ";
                error_msg += e.what();
                ConsoleLogger::log(error_msg.c_str());
            }
        }

        ConsoleLogger::log("MCP Server IO thread stopped");
    });
}

void MCPServer::stop() {
    if (!running_.load()) {
        return;
    }

    ConsoleLogger::log("MCP Server stopping...");
    running_.store(false);

    // Wait for IO thread to finish
    if (io_thread_.joinable()) {
        io_thread_.join();
    }

    ConsoleLogger::log("MCP Server stopped");
}

json MCPServer::handle_request(const json& req) {
    std::string method = req.value("method", "");

    if (method == "tools/list") {
        // Return list of available tools
        return {
            {"jsonrpc", "2.0"},
            {"result", {
                {"tools", json::array({
                    {
                        {"name", "get_console_log"},
                        {"description", "Retrieve recent Max Console messages"},
                        {"inputSchema", {
                            {"type", "object"},
                            {"properties", {
                                {"lines", {
                                    {"type", "number"},
                                    {"description", "Number of recent lines (default: 50, max: 1000)"}
                                }},
                                {"clear", {
                                    {"type", "boolean"},
                                    {"description", "Clear log after reading (default: false)"}
                                }}
                            }}
                        }}
                    },
                    {
                        {"name", "list_active_patches"},
                        {"description", "List all registered MaxMCP client patches"},
                        {"inputSchema", {
                            {"type", "object"},
                            {"properties", {}}
                        }}
                    },
                    {
                        {"name", "add_max_object"},
                        {"description", "Add a Max object to a patch"},
                        {"inputSchema", {
                            {"type", "object"},
                            {"properties", {
                                {"patch_id", {
                                    {"type", "string"},
                                    {"description", "Patch ID to add object to"}
                                }},
                                {"obj_type", {
                                    {"type", "string"},
                                    {"description", "Max object type (e.g., 'number', 'button', 'dac~')"}
                                }},
                                {"position", {
                                    {"type", "array"},
                                    {"items", {"type", "number"}},
                                    {"description", "Position [x, y] in patch"}
                                }},
                                {"varname", {
                                    {"type", "string"},
                                    {"description", "Variable name for the object (optional)"}
                                }}
                            }},
                            {"required", json::array({"patch_id", "obj_type", "position"})}
                        }}
                    }
                })}
            }}
        };

    } else if (method == "tools/call") {
        // Execute tool
        std::string tool_name = req["params"]["name"];
        json arguments = req.value("params", json::object()).value("arguments", json::object());

        return execute_tool(tool_name, arguments);

    } else {
        // Unknown method
        return {
            {"jsonrpc", "2.0"},
            {"error", {
                {"code", -32601},
                {"message", "Method not found"}
            }}
        };
    }
}

json MCPServer::execute_tool(const std::string& tool, const json& params) {
    if (tool == "get_console_log") {
        size_t lines = params.value("lines", 50);
        bool clear = params.value("clear", false);

        return ConsoleLogger::get_logs(lines, clear);

    } else if (tool == "list_active_patches") {
        // Get list of active patches from global registry
        return PatchRegistry::list_patches();

    } else if (tool == "add_max_object") {
        // Parse parameters
        std::string patch_id = params.value("patch_id", "");
        std::string obj_type = params.value("obj_type", "");
        std::string varname = params.value("varname", "");

        if (patch_id.empty() || obj_type.empty()) {
            return {
                {"error", {
                    {"code", -32602},
                    {"message", "Missing required parameters: patch_id and obj_type"}
                }}
            };
        }

        // Get position array
        if (!params.contains("position") || !params["position"].is_array() || params["position"].size() < 2) {
            return {
                {"error", {
                    {"code", -32602},
                    {"message", "Invalid position parameter: must be array [x, y]"}
                }}
            };
        }

        double x = params["position"][0].get<double>();
        double y = params["position"][1].get<double>();

        // Find patch
        t_maxmcp* patch = PatchRegistry::find_patch(patch_id);
        if (!patch) {
            return {
                {"error", {
                    {"code", -32602},
                    {"message", "Patch not found: " + patch_id}
                }}
            };
        }

        // Create defer data
        t_add_object_data* data = new t_add_object_data{
            patch,
            obj_type,
            x,
            y,
            varname
        };

        // Defer to main thread (CRITICAL for thread safety)
        defer(patch, (method)add_object_deferred, gensym("add_object"), 1, (t_atom*)data);

        return {
            {"result", {
                {"status", "success"},
                {"patch_id", patch_id},
                {"obj_type", obj_type},
                {"position", json::array({x, y})}
            }}
        };

    } else {
        return {
            {"error", {
                {"code", -32602},
                {"message", "Unknown tool: " + tool}
            }}
        };
    }
}
