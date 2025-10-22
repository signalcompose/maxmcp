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

// Structures for defer callback data
struct t_add_object_data {
    t_maxmcp* patch;
    std::string obj_type;
    double x;
    double y;
    std::string varname;
    json arguments;  // Object arguments (e.g., [440] for cycle~)
};

struct t_remove_object_data {
    t_maxmcp* patch;
    std::string varname;
};

struct t_set_attribute_data {
    t_maxmcp* patch;
    std::string varname;
    std::string attribute;
    json value;
};

struct t_connect_objects_data {
    t_maxmcp* patch;
    std::string src_varname;
    long outlet;
    std::string dst_varname;
    long inlet;
};

struct t_disconnect_objects_data {
    t_maxmcp* patch;
    std::string src_varname;
    long outlet;
    std::string dst_varname;
    long inlet;
};

struct t_get_objects_data {
    t_maxmcp* patch;
    json* result;  // Pointer to store result
};

struct t_get_position_data {
    t_maxmcp* patch;
    double width;
    double height;
    json* result;  // Pointer to store result
};

// Defer callback for adding Max object
static void add_object_deferred(t_maxmcp* patch, t_symbol* s, long argc, t_atom* argv) {
    t_add_object_data* data = (t_add_object_data*)argv;

    if (!data || !data->patch || !data->patch->patcher) {
        delete data;
        return;
    }

    // Build object string with arguments
    std::string obj_string = data->obj_type;

    if (!data->arguments.is_null() && data->arguments.is_array()) {
        for (const auto& arg : data->arguments) {
            obj_string += " ";

            if (arg.is_number()) {
                obj_string += std::to_string(arg.get<double>());
            } else if (arg.is_string()) {
                obj_string += arg.get<std::string>();
            }
        }
    }

    // Create object with newobject_sprintf
    t_object* obj = (t_object*)newobject_sprintf(data->patch->patcher, "@maxclass %s @patching_rect %.2f %.2f 50.0 20.0",
                                                   obj_string.c_str(), data->x, data->y);

    if (obj) {
        // Set varname if provided
        if (!data->varname.empty()) {
            object_attr_setsym(obj, gensym("varname"), gensym(data->varname.c_str()));
        }

        ConsoleLogger::log(("Object created: " + obj_string).c_str());
    } else {
        ConsoleLogger::log(("Failed to create object: " + obj_string).c_str());
    }

    delete data;
}

// Defer callback for removing Max object
static void remove_object_deferred(t_maxmcp* patch, t_symbol* s, long argc, t_atom* argv) {
    t_remove_object_data* data = (t_remove_object_data*)argv;

    if (!data || !data->patch || !data->patch->patcher) {
        delete data;
        return;
    }

    // Iterate through box objects in patch to find object with varname
    t_object* patcher = data->patch->patcher;
    t_object* box = nullptr;
    t_object* found_box = nullptr;

    for (box = jpatcher_get_firstobject(patcher); box; box = jbox_get_nextobject(box)) {
        // Get box's varname attribute
        t_symbol* box_varname = object_attr_getsym(box, gensym("varname"));

        if (box_varname && box_varname->s_name && data->varname == box_varname->s_name) {
            found_box = box;
            break;
        }
    }

    if (found_box) {
        // Remove box object
        object_free(found_box);
        ConsoleLogger::log(("Object removed: " + data->varname).c_str());
    } else {
        ConsoleLogger::log(("Object not found: " + data->varname).c_str());
    }

    delete data;
}

// Defer callback for setting object attribute
static void set_attribute_deferred(t_maxmcp* patch, t_symbol* s, long argc, t_atom* argv) {
    t_set_attribute_data* data = (t_set_attribute_data*)argv;

    if (!data || !data->patch || !data->patch->patcher) {
        delete data;
        return;
    }

    // Find box with varname
    t_object* patcher = data->patch->patcher;
    t_object* box = nullptr;
    t_object* found_box = nullptr;

    for (box = jpatcher_get_firstobject(patcher); box; box = jbox_get_nextobject(box)) {
        t_symbol* box_varname = object_attr_getsym(box, gensym("varname"));

        if (box_varname && box_varname->s_name && data->varname == box_varname->s_name) {
            found_box = box;
            break;
        }
    }

    if (!found_box) {
        ConsoleLogger::log(("Object not found: " + data->varname).c_str());
        delete data;
        return;
    }

    // Set attribute based on value type
    t_symbol* attr_sym = gensym(data->attribute.c_str());

    if (data->value.is_number()) {
        // Try double first
        double val = data->value.get<double>();

        // Check if it's an integer
        if (val == (long)val) {
            object_attr_setlong(found_box, attr_sym, (long)val);
        } else {
            object_attr_setfloat(found_box, attr_sym, val);
        }

        ConsoleLogger::log(("Attribute set: " + data->varname + "." + data->attribute).c_str());

    } else if (data->value.is_string()) {
        std::string str_val = data->value.get<std::string>();
        object_attr_setsym(found_box, attr_sym, gensym(str_val.c_str()));

        ConsoleLogger::log(("Attribute set: " + data->varname + "." + data->attribute).c_str());

    } else {
        ConsoleLogger::log(("Unsupported value type for attribute: " + data->attribute).c_str());
    }

    delete data;
}

// Defer callback for connecting Max objects
static void connect_objects_deferred(t_maxmcp* patch, t_symbol* s, long argc, t_atom* argv) {
    t_connect_objects_data* data = (t_connect_objects_data*)argv;

    if (!data || !data->patch || !data->patch->patcher) {
        delete data;
        return;
    }

    // Find source and destination boxes
    t_object* patcher = data->patch->patcher;
    t_object* src_box = nullptr;
    t_object* dst_box = nullptr;

    for (t_object* box = jpatcher_get_firstobject(patcher); box; box = jbox_get_nextobject(box)) {
        t_symbol* varname = object_attr_getsym(box, gensym("varname"));

        if (varname && varname->s_name) {
            if (data->src_varname == varname->s_name) {
                src_box = box;
            }
            if (data->dst_varname == varname->s_name) {
                dst_box = box;
            }

            if (src_box && dst_box) break;
        }
    }

    if (!src_box) {
        ConsoleLogger::log(("Source object not found: " + data->src_varname).c_str());
        delete data;
        return;
    }

    if (!dst_box) {
        ConsoleLogger::log(("Destination object not found: " + data->dst_varname).c_str());
        delete data;
        return;
    }

    // Create connection using object_method
    // patcher connect src_box outlet dst_box inlet
    t_atom connect_args[4];
    atom_setobj(&connect_args[0], src_box);
    atom_setlong(&connect_args[1], data->outlet);
    atom_setobj(&connect_args[2], dst_box);
    atom_setlong(&connect_args[3], data->inlet);

    object_method_typed(patcher, gensym("connect"), 4, connect_args, nullptr);

    std::string msg = "Connected: " + data->src_varname + "[" + std::to_string(data->outlet) + "] -> " +
                      data->dst_varname + "[" + std::to_string(data->inlet) + "]";
    ConsoleLogger::log(msg.c_str());

    delete data;
}

// Defer callback for disconnecting Max objects
static void disconnect_objects_deferred(t_maxmcp* patch, t_symbol* s, long argc, t_atom* argv) {
    t_disconnect_objects_data* data = (t_disconnect_objects_data*)argv;

    if (!data || !data->patch || !data->patch->patcher) {
        delete data;
        return;
    }

    // Find source and destination boxes
    t_object* patcher = data->patch->patcher;
    t_object* src_box = nullptr;
    t_object* dst_box = nullptr;

    for (t_object* box = jpatcher_get_firstobject(patcher); box; box = jbox_get_nextobject(box)) {
        t_symbol* varname = object_attr_getsym(box, gensym("varname"));

        if (varname && varname->s_name) {
            if (data->src_varname == varname->s_name) {
                src_box = box;
            }
            if (data->dst_varname == varname->s_name) {
                dst_box = box;
            }

            if (src_box && dst_box) break;
        }
    }

    if (!src_box || !dst_box) {
        ConsoleLogger::log("Source or destination object not found for disconnect");
        delete data;
        return;
    }

    // Find and remove matching patchline
    t_object* line = jpatcher_get_firstline(patcher);
    bool found = false;

    while (line) {
        t_object* line_box1 = (t_object*)jpatchline_get_box1(line);
        long line_outlet = jpatchline_get_outletnum(line);
        t_object* line_box2 = (t_object*)jpatchline_get_box2(line);
        long line_inlet = jpatchline_get_inletnum(line);

        if (line_box1 == src_box && line_outlet == data->outlet &&
            line_box2 == dst_box && line_inlet == data->inlet) {

            object_free(line);
            found = true;

            std::string msg = "Disconnected: " + data->src_varname + "[" + std::to_string(data->outlet) + "] -> " +
                              data->dst_varname + "[" + std::to_string(data->inlet) + "]";
            ConsoleLogger::log(msg.c_str());
            break;
        }

        line = jpatchline_get_nextline(line);
    }

    if (!found) {
        ConsoleLogger::log("Connection not found for disconnect");
    }

    delete data;
}

// Defer callback for getting objects in patch
static void get_objects_deferred(t_maxmcp* patch, t_symbol* s, long argc, t_atom* argv) {
    t_get_objects_data* data = (t_get_objects_data*)argv;

    if (!data || !data->patch || !data->patch->patcher || !data->result) {
        if (data) delete data;
        return;
    }

    json objects = json::array();
    t_object* patcher = data->patch->patcher;

    // Iterate through all boxes in patch
    for (t_object* box = jpatcher_get_firstobject(patcher); box; box = jbox_get_nextobject(box)) {
        // Get varname
        t_symbol* varname = object_attr_getsym(box, gensym("varname"));
        std::string varname_str = (varname && varname->s_name) ? varname->s_name : "";

        // Get maxclass
        t_symbol* maxclass = jbox_get_maxclass(box);
        std::string maxclass_str = (maxclass && maxclass->s_name) ? maxclass->s_name : "unknown";

        // Get position and size
        t_rect rect;
        jbox_get_patching_rect(box, &rect);

        json obj_info = {
            {"maxclass", maxclass_str},
            {"position", json::array({rect.x, rect.y})},
            {"size", json::array({rect.width, rect.height})}
        };

        // Only add varname if it exists
        if (!varname_str.empty()) {
            obj_info["varname"] = varname_str;
        }

        objects.push_back(obj_info);
    }

    *(data->result) = {
        {"result", {
            {"patch_id", data->patch->patch_id},
            {"objects", objects},
            {"count", objects.size()}
        }}
    };

    delete data;
}

// Defer callback for getting empty position
static void get_position_deferred(t_maxmcp* patch, t_symbol* s, long argc, t_atom* argv) {
    t_get_position_data* data = (t_get_position_data*)argv;

    if (!data || !data->patch || !data->patch->patcher || !data->result) {
        if (data) delete data;
        return;
    }

    t_object* patcher = data->patch->patcher;
    double max_x = 50.0;  // Default starting position
    double start_y = 50.0;

    // Find bounding box of existing objects
    for (t_object* box = jpatcher_get_firstobject(patcher); box; box = jbox_get_nextobject(box)) {
        t_rect rect;
        jbox_get_patching_rect(box, &rect);

        // Update max_x to be past the rightmost object
        double box_right = rect.x + rect.width;
        if (box_right > max_x) {
            max_x = box_right;
        }
    }

    // Add margin and return position
    double margin = 50.0;
    double new_x = max_x + margin;
    double new_y = start_y;

    *(data->result) = {
        {"result", {
            {"position", json::array({new_x, new_y})},
            {"rationale", "Positioned to the right of existing objects with 50px margin"}
        }}
    };

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

json MCPServer::handle_request(const json& req) {
    std::string method = req.value("method", "");

    if (method == "initialize") {
        // Handle MCP initialize handshake
        return {
            {"jsonrpc", "2.0"},
            {"id", req.contains("id") ? req["id"] : nullptr},
            {"result", {
                {"protocolVersion", "2024-11-05"},
                {"capabilities", {
                    {"tools", json::object()}
                }},
                {"serverInfo", {
                    {"name", "maxmcp"},
                    {"version", "1.0.0"}
                }}
            }}
        };

    } else if (method == "tools/list") {
        // Return list of available tools
        return {
            {"jsonrpc", "2.0"},
            {"id", req.contains("id") ? req["id"] : nullptr},
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
                                }},
                                {"arguments", {
                                    {"type", "array"},
                                    {"description", "Object arguments (e.g., [440] for 'cycle~ 440')"}
                                }}
                            }},
                            {"required", json::array({"patch_id", "obj_type", "position"})}
                        }}
                    },
                    {
                        {"name", "get_patch_info"},
                        {"description", "Get detailed information about a specific patch"},
                        {"inputSchema", {
                            {"type", "object"},
                            {"properties", {
                                {"patch_id", {
                                    {"type", "string"},
                                    {"description", "Patch ID to query"}
                                }}
                            }},
                            {"required", json::array({"patch_id"})}
                        }}
                    },
                    {
                        {"name", "get_frontmost_patch"},
                        {"description", "Get the currently focused/frontmost patch"},
                        {"inputSchema", {
                            {"type", "object"},
                            {"properties", {}}
                        }}
                    },
                    {
                        {"name", "remove_max_object"},
                        {"description", "Remove a Max object from a patch by varname"},
                        {"inputSchema", {
                            {"type", "object"},
                            {"properties", {
                                {"patch_id", {
                                    {"type", "string"},
                                    {"description", "Patch ID containing the object"}
                                }},
                                {"varname", {
                                    {"type", "string"},
                                    {"description", "Variable name of the object to remove"}
                                }}
                            }},
                            {"required", json::array({"patch_id", "varname"})}
                        }}
                    },
                    {
                        {"name", "set_object_attribute"},
                        {"description", "Set an attribute of a Max object"},
                        {"inputSchema", {
                            {"type", "object"},
                            {"properties", {
                                {"patch_id", {
                                    {"type", "string"},
                                    {"description", "Patch ID containing the object"}
                                }},
                                {"varname", {
                                    {"type", "string"},
                                    {"description", "Variable name of the object"}
                                }},
                                {"attribute", {
                                    {"type", "string"},
                                    {"description", "Attribute name to set"}
                                }},
                                {"value", {
                                    {"description", "Attribute value (number, string, or array)"}
                                }}
                            }},
                            {"required", json::array({"patch_id", "varname", "attribute", "value"})}
                        }}
                    },
                    {
                        {"name", "connect_max_objects"},
                        {"description", "Create a patchcord connection between two Max objects"},
                        {"inputSchema", {
                            {"type", "object"},
                            {"properties", {
                                {"patch_id", {
                                    {"type", "string"},
                                    {"description", "Patch ID containing the objects"}
                                }},
                                {"src_varname", {
                                    {"type", "string"},
                                    {"description", "Source object variable name"}
                                }},
                                {"outlet", {
                                    {"type", "number"},
                                    {"description", "Source outlet index (0-based)"}
                                }},
                                {"dst_varname", {
                                    {"type", "string"},
                                    {"description", "Destination object variable name"}
                                }},
                                {"inlet", {
                                    {"type", "number"},
                                    {"description", "Destination inlet index (0-based)"}
                                }}
                            }},
                            {"required", json::array({"patch_id", "src_varname", "outlet", "dst_varname", "inlet"})}
                        }}
                    },
                    {
                        {"name", "disconnect_max_objects"},
                        {"description", "Remove a patchcord connection between two Max objects"},
                        {"inputSchema", {
                            {"type", "object"},
                            {"properties", {
                                {"patch_id", {
                                    {"type", "string"},
                                    {"description", "Patch ID containing the objects"}
                                }},
                                {"src_varname", {
                                    {"type", "string"},
                                    {"description", "Source object variable name"}
                                }},
                                {"outlet", {
                                    {"type", "number"},
                                    {"description", "Source outlet index (0-based)"}
                                }},
                                {"dst_varname", {
                                    {"type", "string"},
                                    {"description", "Destination object variable name"}
                                }},
                                {"inlet", {
                                    {"type", "number"},
                                    {"description", "Destination inlet index (0-based)"}
                                }}
                            }},
                            {"required", json::array({"patch_id", "src_varname", "outlet", "dst_varname", "inlet"})}
                        }}
                    },
                    {
                        {"name", "get_objects_in_patch"},
                        {"description", "List all objects in a patch with metadata"},
                        {"inputSchema", {
                            {"type", "object"},
                            {"properties", {
                                {"patch_id", {
                                    {"type", "string"},
                                    {"description", "Patch ID to query"}
                                }}
                            }},
                            {"required", json::array({"patch_id"})}
                        }}
                    },
                    {
                        {"name", "get_avoid_rect_position"},
                        {"description", "Find an empty position for placing new objects"},
                        {"inputSchema", {
                            {"type", "object"},
                            {"properties", {
                                {"patch_id", {
                                    {"type", "string"},
                                    {"description", "Patch ID to query"}
                                }},
                                {"width", {
                                    {"type", "number"},
                                    {"description", "Object width (default: 50)"}
                                }},
                                {"height", {
                                    {"type", "number"},
                                    {"description", "Object height (default: 20)"}
                                }}
                            }},
                            {"required", json::array({"patch_id"})}
                        }}
                    }
                })}
            }}
        };

    } else if (method == "tools/call") {
        // Execute tool
        std::string tool_name = req["params"]["name"];
        json arguments = req.value("params", json::object()).value("arguments", json::object());

        json result = execute_tool(tool_name, arguments);

        return {
            {"jsonrpc", "2.0"},
            {"id", req.contains("id") ? req["id"] : nullptr},
            {"result", result}
        };

    } else {
        // Unknown method
        return {
            {"jsonrpc", "2.0"},
            {"id", req.contains("id") ? req["id"] : nullptr},
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

    } else if (tool == "get_patch_info") {
        // Get detailed information about a patch
        std::string patch_id = params.value("patch_id", "");

        if (patch_id.empty()) {
            return {
                {"error", {
                    {"code", -32602},
                    {"message", "Missing required parameter: patch_id"}
                }}
            };
        }

        return PatchRegistry::get_patch_info(patch_id);

    } else if (tool == "get_frontmost_patch") {
        // Get the currently focused patch
        return PatchRegistry::get_frontmost_patch();

    } else if (tool == "add_max_object") {
        // Parse parameters
        std::string patch_id = params.value("patch_id", "");
        std::string obj_type = params.value("obj_type", "");
        std::string varname = params.value("varname", "");
        json arguments = params.value("arguments", json::array());

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
            varname,
            arguments
        };

        // Defer to main thread (CRITICAL for thread safety)
        defer(patch, (method)add_object_deferred, gensym("add_object"), 1, (t_atom*)data);

        return {
            {"result", {
                {"status", "success"},
                {"patch_id", patch_id},
                {"obj_type", obj_type},
                {"position", json::array({x, y})},
                {"arguments", arguments}
            }}
        };

    } else if (tool == "remove_max_object") {
        // Parse parameters
        std::string patch_id = params.value("patch_id", "");
        std::string varname = params.value("varname", "");

        if (patch_id.empty() || varname.empty()) {
            return {
                {"error", {
                    {"code", -32602},
                    {"message", "Missing required parameters: patch_id and varname"}
                }}
            };
        }

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
        t_remove_object_data* data = new t_remove_object_data{
            patch,
            varname
        };

        // Defer to main thread
        defer(patch, (method)remove_object_deferred, gensym("remove_object"), 1, (t_atom*)data);

        return {
            {"result", {
                {"status", "success"},
                {"patch_id", patch_id},
                {"varname", varname}
            }}
        };

    } else if (tool == "set_object_attribute") {
        // Parse parameters
        std::string patch_id = params.value("patch_id", "");
        std::string varname = params.value("varname", "");
        std::string attribute = params.value("attribute", "");

        if (patch_id.empty() || varname.empty() || attribute.empty()) {
            return {
                {"error", {
                    {"code", -32602},
                    {"message", "Missing required parameters: patch_id, varname, and attribute"}
                }}
            };
        }

        if (!params.contains("value")) {
            return {
                {"error", {
                    {"code", -32602},
                    {"message", "Missing required parameter: value"}
                }}
            };
        }

        json value = params["value"];

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
        t_set_attribute_data* data = new t_set_attribute_data{
            patch,
            varname,
            attribute,
            value
        };

        // Defer to main thread
        defer(patch, (method)set_attribute_deferred, gensym("set_attribute"), 1, (t_atom*)data);

        return {
            {"result", {
                {"status", "success"},
                {"patch_id", patch_id},
                {"varname", varname},
                {"attribute", attribute}
            }}
        };

    } else if (tool == "connect_max_objects") {
        // Parse parameters
        std::string patch_id = params.value("patch_id", "");
        std::string src_varname = params.value("src_varname", "");
        std::string dst_varname = params.value("dst_varname", "");
        long outlet = params.value("outlet", -1);
        long inlet = params.value("inlet", -1);

        if (patch_id.empty() || src_varname.empty() || dst_varname.empty() || outlet < 0 || inlet < 0) {
            return {
                {"error", {
                    {"code", -32602},
                    {"message", "Missing or invalid required parameters"}
                }}
            };
        }

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
        t_connect_objects_data* data = new t_connect_objects_data{
            patch,
            src_varname,
            outlet,
            dst_varname,
            inlet
        };

        // Defer to main thread
        defer(patch, (method)connect_objects_deferred, gensym("connect_objects"), 1, (t_atom*)data);

        return {
            {"result", {
                {"status", "success"},
                {"patch_id", patch_id},
                {"src_varname", src_varname},
                {"outlet", outlet},
                {"dst_varname", dst_varname},
                {"inlet", inlet}
            }}
        };

    } else if (tool == "disconnect_max_objects") {
        // Parse parameters
        std::string patch_id = params.value("patch_id", "");
        std::string src_varname = params.value("src_varname", "");
        std::string dst_varname = params.value("dst_varname", "");
        long outlet = params.value("outlet", -1);
        long inlet = params.value("inlet", -1);

        if (patch_id.empty() || src_varname.empty() || dst_varname.empty() || outlet < 0 || inlet < 0) {
            return {
                {"error", {
                    {"code", -32602},
                    {"message", "Missing or invalid required parameters"}
                }}
            };
        }

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
        t_disconnect_objects_data* data = new t_disconnect_objects_data{
            patch,
            src_varname,
            outlet,
            dst_varname,
            inlet
        };

        // Defer to main thread
        defer(patch, (method)disconnect_objects_deferred, gensym("disconnect_objects"), 1, (t_atom*)data);

        return {
            {"result", {
                {"status", "success"},
                {"patch_id", patch_id},
                {"src_varname", src_varname},
                {"outlet", outlet},
                {"dst_varname", dst_varname},
                {"inlet", inlet}
            }}
        };

    } else if (tool == "get_objects_in_patch") {
        // Parse parameters
        std::string patch_id = params.value("patch_id", "");

        if (patch_id.empty()) {
            return {
                {"error", {
                    {"code", -32602},
                    {"message", "Missing required parameter: patch_id"}
                }}
            };
        }

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

        // Create result holder
        json result;
        t_get_objects_data* data = new t_get_objects_data{
            patch,
            &result
        };

        // Defer to main thread and wait
        defer(patch, (method)get_objects_deferred, gensym("get_objects"), 1, (t_atom*)data);

        // Note: This is a simplified implementation that returns immediately
        // In a real implementation, we would need to synchronize with the defer callback
        // For now, return a success status
        return {
            {"result", {
                {"status", "deferred"},
                {"message", "Object list retrieval initiated"}
            }}
        };

    } else if (tool == "get_avoid_rect_position") {
        // Parse parameters
        std::string patch_id = params.value("patch_id", "");
        double width = params.value("width", 50.0);
        double height = params.value("height", 20.0);

        if (patch_id.empty()) {
            return {
                {"error", {
                    {"code", -32602},
                    {"message", "Missing required parameter: patch_id"}
                }}
            };
        }

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

        // Create result holder
        json result;
        t_get_position_data* data = new t_get_position_data{
            patch,
            width,
            height,
            &result
        };

        // Defer to main thread and wait
        defer(patch, (method)get_position_deferred, gensym("get_position"), 1, (t_atom*)data);

        // Note: This is a simplified implementation
        // For now, return calculated position directly (simplified, not thread-safe)
        // In production, this would require proper synchronization
        return {
            {"result", {
                {"status", "deferred"},
                {"message", "Position calculation initiated"}
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

std::string MCPServer::handle_request_string(const std::string& request_str) {
    try {
        ConsoleLogger::log(("Received message (" + std::to_string(request_str.length()) + " bytes): " + request_str).c_str());
        json req = json::parse(request_str);
        json response = handle_request(req);
        return response.dump();
    } catch (const std::exception& e) {
        // Return JSON-RPC error
        ConsoleLogger::log(("Parse error on message: " + request_str).c_str());
        ConsoleLogger::log(("Exception: " + std::string(e.what())).c_str());
        json error_response = {
            {"jsonrpc", "2.0"},
            {"error", {
                {"code", -32700},
                {"message", std::string("Parse error: ") + e.what()}
            }},
            {"id", nullptr}
        };
        return error_response.dump();
    }
}
