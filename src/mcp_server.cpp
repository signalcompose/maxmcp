/**
    @file mcp_server.cpp
    MaxMCP - MCP Server Implementation

    @ingroup maxmcp
*/

#include "mcp_server.h"

#include "ext.h"
#include "ext_obex.h"

#include "jpatcher_api.h"
#include "maxmcp.h"
#include "utils/console_logger.h"
#include "utils/patch_registry.h"

#include <chrono>
#include <condition_variable>
#include <fstream>
#include <iostream>
#include <mutex>
#include <sstream>

// Static member initialization
MCPServer* MCPServer::instance_ = nullptr;

// Helper structure for synchronizing deferred results
struct DeferredResult {
    json result;
    std::mutex mutex;
    std::condition_variable cv;
    bool completed = false;

    // Wait for completion with timeout
    bool wait_for(std::chrono::milliseconds timeout) {
        std::unique_lock<std::mutex> lock(mutex);
        return cv.wait_for(lock, timeout, [this] { return completed; });
    }

    // Notify waiting thread of completion
    void notify() {
        {
            std::lock_guard<std::mutex> lock(mutex);
            completed = true;
        }
        cv.notify_one();
    }
};

// Structures for defer callback data
struct t_add_object_data {
    t_maxmcp* patch;
    std::string obj_type;
    double x;
    double y;
    std::string varname;
    json arguments;   // Object arguments (e.g., [440] for cycle~)
    json attributes;  // Object attributes (e.g., {"bgcolor": [1.0, 0.5, 0.0, 1.0]})
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
    DeferredResult* deferred_result;  // Pointer to DeferredResult for synchronization
};

struct t_get_position_data {
    t_maxmcp* patch;
    double width;
    double height;
    DeferredResult* deferred_result;  // Pointer to DeferredResult for synchronization
};

// Defer callback for adding Max object
// Defer callback for adding Max object
static void add_object_deferred(t_maxmcp* patch, t_symbol* s, long argc, t_atom* argv) {
    // Extract data pointer from atom
    if (argc < 1 || !argv) {
        ConsoleLogger::log("ERROR: add_object_deferred called with no data");
        return;
    }

    t_add_object_data* data = (t_add_object_data*)atom_getobj(argv);

    if (!data || !data->patch || !data->patch->patcher) {
        ConsoleLogger::log("ERROR: Invalid data in add_object_deferred");
        if (data)
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

    // Escape special characters in obj_string
    // Note: Order matters! Escape backslash first to avoid double-escaping
    std::string escaped = obj_string;
    size_t pos = 0;

    // 1. Escape \ to prevent escape sequence issues
    while ((pos = escaped.find('\\', pos)) != std::string::npos) {
        escaped.replace(pos, 1, "\\\\");
        pos += 2;
    }

    // 2. Escape " to prevent syntax errors
    pos = 0;
    while ((pos = escaped.find('"', pos)) != std::string::npos) {
        escaped.replace(pos, 1, "\\\"");
        pos += 2;
    }

    // 3. Escape % to prevent format string injection
    pos = 0;
    while ((pos = escaped.find('%', pos)) != std::string::npos) {
        escaped.replace(pos, 1, "%%");
        pos += 2;
    }

    // Create object with newobject_sprintf (without size - use Max default)
    t_object* obj = (t_object*)newobject_sprintf(
        data->patch->patcher, "@maxclass newobj @text \"%s\" @patching_position %.2f %.2f",
        escaped.c_str(), data->x, data->y);

    if (obj) {
        // Set varname if provided
        if (!data->varname.empty()) {
            object_attr_setsym(obj, gensym("varname"), gensym(data->varname.c_str()));
        }

        // Set attributes if provided (BEFORE bringtofront to avoid z-order issues)
        if (!data->attributes.is_null() && data->attributes.is_object()) {
            for (auto& [attr_name, attr_value] : data->attributes.items()) {
                if (attr_value.is_array() && attr_value.size() == 4) {
                    // Color array [r, g, b, a]
                    t_atom color_atoms[4];
                    for (size_t i = 0; i < 4; i++) {
                        atom_setfloat(&color_atoms[i], attr_value[i].get<double>());
                    }
                    object_attr_setvalueof(obj, gensym(attr_name.c_str()), 4, color_atoms);
                } else if (attr_value.is_number()) {
                    double val = attr_value.get<double>();
                    t_atom a;
                    atom_setfloat(&a, val);
                    object_attr_setvalueof(obj, gensym(attr_name.c_str()), 1, &a);
                } else if (attr_value.is_string()) {
                    std::string val = attr_value.get<std::string>();
                    t_atom a;
                    atom_setsym(&a, gensym(val.c_str()));
                    object_attr_setvalueof(obj, gensym(attr_name.c_str()), 1, &a);
                }
            }
        }

        // Bring object to front AFTER setting attributes (so it appears above panels/backgrounds)
        object_method(obj, gensym("bringtofront"));

        // Enable presentation mode visibility by default
        object_attr_setlong(obj, gensym("presentation"), 1);

        ConsoleLogger::log(("Object created: " + obj_string).c_str());
    } else {
        ConsoleLogger::log(("Failed to create object: " + obj_string).c_str());
    }

    delete data;
}

// Defer callback for removing Max object
static void remove_object_deferred(t_maxmcp* patch, t_symbol* s, long argc, t_atom* argv) {
    if (argc < 1 || !argv) {
        ConsoleLogger::log("ERROR: remove_object_deferred called with no data");
        return;
    }

    t_remove_object_data* data = (t_remove_object_data*)atom_getobj(argv);

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
    if (argc < 1 || !argv) {
        ConsoleLogger::log("ERROR: set_attribute_deferred called with no data");
        return;
    }

    t_set_attribute_data* data = (t_set_attribute_data*)atom_getobj(argv);

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
    if (argc < 1 || !argv) {
        ConsoleLogger::log("ERROR: connect_objects_deferred called with no data");
        return;
    }

    // Retrieve data from argv
    t_connect_objects_data* data = nullptr;
    if (argv[0].a_type == A_LONG) {
        data = reinterpret_cast<t_connect_objects_data*>(argv[0].a_w.w_long);
    }
    if (!data) {
        ConsoleLogger::log("ERROR: Invalid connect data pointer");
        return;
    }

    // Get patcher from patch object
    if (!data->patch || !data->patch->patcher) {
        ConsoleLogger::log("ERROR: Invalid patch or patcher in connect data");
        delete data;
        return;
    }

    t_object* patcher = data->patch->patcher;

    // Find source and destination boxes by varname
    t_object* src_box = nullptr;
    t_object* dst_box = nullptr;

    t_object* box = nullptr;
    for (box = jpatcher_get_firstobject(patcher); box; box = jbox_get_nextobject(box)) {
        t_symbol* varname = jbox_get_varname(box);
        if (varname == gensym(data->src_varname.c_str())) {
            src_box = box;
        } else if (varname == gensym(data->dst_varname.c_str())) {
            dst_box = box;
        }
    }

    if (!src_box || !dst_box) {
        ConsoleLogger::log("ERROR: Cannot find source or destination box");
        delete data;
        return;
    }

    t_atom connect_args[4];
    atom_setobj(&connect_args[0], src_box);
    atom_setlong(&connect_args[1], data->outlet);
    atom_setobj(&connect_args[2], dst_box);
    atom_setlong(&connect_args[3], data->inlet);

    // Create connection
    t_atom result;
    object_method_typed(patcher, gensym("connect"), 4, connect_args, &result);

    std::string msg = "Connected: " + data->src_varname + "[" + std::to_string(data->outlet) +
                      "] -> " + data->dst_varname + "[" + std::to_string(data->inlet) + "]";
    ConsoleLogger::log(msg.c_str());

    delete data;
}

// Defer callback for disconnecting Max objects
static void disconnect_objects_deferred(t_maxmcp* patch, t_symbol* s, long argc, t_atom* argv) {
    if (argc < 1 || !argv) {
        ConsoleLogger::log("ERROR: disconnect_objects_deferred called with no data");
        return;
    }

    t_disconnect_objects_data* data = nullptr;
    if (argv[0].a_type == A_LONG) {
        data = reinterpret_cast<t_disconnect_objects_data*>(argv[0].a_w.w_long);
    }
    if (!data) {
        ConsoleLogger::log("ERROR: Invalid disconnect data pointer");
        return;
    }

    if (!data->patch || !data->patch->patcher) {
        ConsoleLogger::log("ERROR: Invalid patch or patcher in disconnect data");
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

            if (src_box && dst_box)
                break;
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

        if (line_box1 == src_box && line_outlet == data->outlet && line_box2 == dst_box &&
            line_inlet == data->inlet) {

            object_free(line);
            found = true;

            std::string msg = "Disconnected: " + data->src_varname + "[" +
                              std::to_string(data->outlet) + "] -> " + data->dst_varname + "[" +
                              std::to_string(data->inlet) + "]";
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
    if (argc < 1 || !argv) {
        ConsoleLogger::log("ERROR: get_objects_deferred called with no data");
        return;
    }

    t_get_objects_data* data = (t_get_objects_data*)atom_getobj(argv);

    if (!data || !data->patch || !data->patch->patcher || !data->deferred_result) {
        if (data)
            delete data;
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

        json obj_info = {{"maxclass", maxclass_str},
                         {"position", json::array({rect.x, rect.y})},
                         {"size", json::array({rect.width, rect.height})}};

        // Only add varname if it exists
        if (!varname_str.empty()) {
            obj_info["varname"] = varname_str;
        }

        objects.push_back(obj_info);
    }

    // Store result and notify waiting thread
    data->deferred_result->result = {
        {"patch_id", data->patch->patch_id}, {"objects", objects}, {"count", objects.size()}};

    // Log the result to console for debugging
    std::string log_msg = "Objects in patch " + std::string(data->patch->patch_id) + ": " +
                          std::to_string(objects.size()) + " objects";
    ConsoleLogger::log(log_msg.c_str());

    // Notify waiting thread that result is ready
    data->deferred_result->notify();

    delete data;
}

// Defer callback for getting empty position
static void get_position_deferred(t_maxmcp* patch, t_symbol* s, long argc, t_atom* argv) {
    if (argc < 1 || !argv) {
        ConsoleLogger::log("ERROR: get_position_deferred called with no data");
        return;
    }

    t_get_position_data* data = (t_get_position_data*)atom_getobj(argv);

    if (!data || !data->patch || !data->patch->patcher || !data->deferred_result) {
        if (data)
            delete data;
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

    // Store result and notify waiting thread
    data->deferred_result->result = {
        {"position", json::array({new_x, new_y})},
        {"rationale", "Positioned to the right of existing objects with 50px margin"}};

    // Notify waiting thread that result is ready
    data->deferred_result->notify();

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
        // Echo back the client's protocol version (MCP spec requirement)
        std::string client_protocol_version = req["params"].value("protocolVersion", "2024-11-05");

        ConsoleLogger::log(("MCP: Client protocol version: " + client_protocol_version).c_str());

        return {{"jsonrpc", "2.0"},
                {"id", req.contains("id") ? req["id"] : nullptr},
                {"result",
                 {{"protocolVersion", client_protocol_version},  // Echo client's version
                  {"capabilities", {{"tools", {{"listChanged", true}}}}},
                  {"serverInfo", {{"name", "maxmcp"}, {"version", "0.2.0"}}}}}};

    } else if (method == "tools/list") {
        // Return list of available tools
        ConsoleLogger::log("MCP: Handling tools/list request");

        auto tools = json::array(
            {{{"name", "get_console_log"},
              {"description", "Retrieve recent Max Console messages"},
              {"inputSchema",
               {{"type", "object"},
                {"properties",
                 {{"lines",
                   {{"type", "number"},
                    {"description", "Number of recent lines (default: 50, max: 1000)"}}},
                  {"clear",
                   {{"type", "boolean"},
                    {"description", "Clear log after reading (default: false)"}}}}}}}},
             {{"name", "list_active_patches"},
              {"description",
               "List all registered MaxMCP client patches. Optionally filter by group name."},
              {"inputSchema",
               {{"type", "object"},
                {"properties",
                 {{"group",
                   {{"type", "string"},
                    {"description",
                     "Optional group name to filter patches (e.g., 'synths', 'effects')"}}}}}}}},
             {{"name", "add_max_object"},
              {"description", "Add a Max object to a patch"},
              {"inputSchema",
               {{"type", "object"},
                {"properties",
                 {{"patch_id", {{"type", "string"}, {"description", "Patch ID to add object to"}}},
                  {"obj_type",
                   {{"type", "string"},
                    {"description", "Max object type (e.g., 'number', 'button', 'dac~')"}}},
                  {"position",
                   {{"type", "array"},
                    {"items", {{"type", "number"}}},
                    {"description", "Position [x, y] in patch"}}},
                  {"varname",
                   {{"type", "string"},
                    {"description", "Variable name for the object (optional)"}}},
                  {"arguments",
                   {{"type", "array"},
                    {"description", "Object arguments (e.g., [440] for 'cycle~ 440')"}}},
                  {"attributes",
                   {{"type", "object"},
                    {"description",
                     "Object attributes (e.g., {\"bgcolor\": [1.0, 0.5, 0.0, 1.0]})"}}}}},
                {"required", json::array({"patch_id", "obj_type", "position"})}}}},
             {{"name", "get_patch_info"},
              {"description", "Get detailed information about a specific patch"},
              {"inputSchema",
               {{"type", "object"},
                {"properties",
                 {{"patch_id", {{"type", "string"}, {"description", "Patch ID to query"}}}}},
                {"required", json::array({"patch_id"})}}}},
             {{"name", "get_frontmost_patch"},
              {"description", "Get the currently focused/frontmost patch"},
              {"inputSchema", {{"type", "object"}, {"properties", json::object()}}}},
             {{"name", "remove_max_object"},
              {"description", "Remove a Max object from a patch by varname"},
              {"inputSchema",
               {{"type", "object"},
                {"properties",
                 {{"patch_id",
                   {{"type", "string"}, {"description", "Patch ID containing the object"}}},
                  {"varname",
                   {{"type", "string"},
                    {"description", "Variable name of the object to remove"}}}}},
                {"required", json::array({"patch_id", "varname"})}}}},
             {{"name", "set_object_attribute"},
              {"description", "Set an attribute of a Max object"},
              {"inputSchema",
               {{"type", "object"},
                {"properties",
                 {{"patch_id",
                   {{"type", "string"}, {"description", "Patch ID containing the object"}}},
                  {"varname", {{"type", "string"}, {"description", "Variable name of the object"}}},
                  {"attribute", {{"type", "string"}, {"description", "Attribute name to set"}}},
                  {"value", {{"description", "Attribute value (number, string, or array)"}}}}},
                {"required", json::array({"patch_id", "varname", "attribute", "value"})}}}},
             {{"name", "connect_max_objects"},
              {"description", "Create a patchcord connection between two Max objects"},
              {"inputSchema",
               {{"type", "object"},
                {"properties",
                 {{"patch_id",
                   {{"type", "string"}, {"description", "Patch ID containing the objects"}}},
                  {"src_varname",
                   {{"type", "string"}, {"description", "Source object variable name"}}},
                  {"outlet",
                   {{"type", "number"}, {"description", "Source outlet index (0-based)"}}},
                  {"dst_varname",
                   {{"type", "string"}, {"description", "Destination object variable name"}}},
                  {"inlet",
                   {{"type", "number"}, {"description", "Destination inlet index (0-based)"}}}}},
                {"required",
                 json::array({"patch_id", "src_varname", "outlet", "dst_varname", "inlet"})}}}},
             {{"name", "disconnect_max_objects"},
              {"description", "Remove a patchcord connection between two Max objects"},
              {"inputSchema",
               {{"type", "object"},
                {"properties",
                 {{"patch_id",
                   {{"type", "string"}, {"description", "Patch ID containing the objects"}}},
                  {"src_varname",
                   {{"type", "string"}, {"description", "Source object variable name"}}},
                  {"outlet",
                   {{"type", "number"}, {"description", "Source outlet index (0-based)"}}},
                  {"dst_varname",
                   {{"type", "string"}, {"description", "Destination object variable name"}}},
                  {"inlet",
                   {{"type", "number"}, {"description", "Destination inlet index (0-based)"}}}}},
                {"required",
                 json::array({"patch_id", "src_varname", "outlet", "dst_varname", "inlet"})}}}},
             {{"name", "get_objects_in_patch"},
              {"description", "List all objects in a patch with metadata"},
              {"inputSchema",
               {{"type", "object"},
                {"properties",
                 {{"patch_id", {{"type", "string"}, {"description", "Patch ID to query"}}}}},
                {"required", json::array({"patch_id"})}}}},
             {{"name", "get_avoid_rect_position"},
              {"description", "Find an empty position for placing new objects"},
              {"inputSchema",
               {{"type", "object"},
                {"properties",
                 {{"patch_id", {{"type", "string"}, {"description", "Patch ID to query"}}},
                  {"width", {{"type", "number"}, {"description", "Object width (default: 50)"}}},
                  {"height",
                   {{"type", "number"}, {"description", "Object height (default: 20)"}}}}},
                {"required", json::array({"patch_id"})}}}}});

        ConsoleLogger::log(("MCP: Returning " + std::to_string(tools.size()) + " tools").c_str());

        auto response = json{{"jsonrpc", "2.0"},
                             {"id", req.contains("id") ? req["id"] : nullptr},
                             {"result", {{"tools", tools}}}};

        std::string response_str = response.dump();
        ConsoleLogger::log(
            ("MCP: Response length: " + std::to_string(response_str.length()) + " bytes").c_str());
        ConsoleLogger::log(("MCP: Response preview: " + response_str.substr(0, 200)).c_str());

        // Write full JSON to file for debugging
        std::ofstream debug_file("/tmp/maxmcp_tools_list.json");
        if (debug_file.is_open()) {
            debug_file << response.dump(2);  // Pretty print with 2-space indent
            debug_file.close();
            ConsoleLogger::log("MCP: Full tools/list JSON written to /tmp/maxmcp_tools_list.json");
        }

        return response;

    } else if (method == "tools/call") {
        // Execute tool
        std::string tool_name = req["params"]["name"];
        json arguments = req.value("params", json::object()).value("arguments", json::object());

        ConsoleLogger::log(("MCP: tools/call - tool: " + tool_name).c_str());
        ConsoleLogger::log(("MCP: tools/call - arguments: " + arguments.dump()).c_str());

        json result = execute_tool(tool_name, arguments);

        ConsoleLogger::log(("MCP: tools/call - raw result: " + result.dump()).c_str());

        // Check if tool returned an error
        bool is_error = result.contains("error");

        // Wrap result in MCP-compliant content format
        // According to MCP spec, tools/call response MUST have:
        // - "content" array with type/text fields
        // - "isError" boolean flag
        json mcp_result = {{"content", json::array({{
                                           {"type", "text"},
                                           {"text", result.dump()}  // Serialize JSON result as text
                                       }})},
                           {"isError", is_error}};

        json response = {
            {"jsonrpc", "2.0"},
            {"id", req.contains("id") ? req["id"] : nullptr},
            {"result", mcp_result}  // MCP-compliant format
        };

        ConsoleLogger::log(("MCP: tools/call - MCP response: " + response.dump()).c_str());

        // Write response to file for debugging
        std::ofstream out("/tmp/maxmcp_tools_call_response.json");
        out << response.dump(2);
        out.close();

        return response;

    } else if (method.find("notifications/") == 0) {
        // Notification messages (no response required)
        // According to JSON-RPC 2.0, notifications MUST NOT receive a response
        // Return null JSON to signal that no response should be sent
        ConsoleLogger::log(("MCP: Received notification: " + method).c_str());
        return nullptr;
    } else {
        // Unknown method
        return {{"jsonrpc", "2.0"},
                {"id", req.contains("id") ? req["id"] : nullptr},
                {"error", {{"code", -32601}, {"message", "Method not found"}}}};
    }
}

json MCPServer::execute_tool(const std::string& tool, const json& params) {
    ConsoleLogger::log(("MCP: execute_tool called: " + tool).c_str());

    if (tool == "get_console_log") {
        size_t lines = params.value("lines", 50);
        bool clear = params.value("clear", false);

        return ConsoleLogger::get_logs(lines, clear);

    } else if (tool == "list_active_patches") {
        // Get list of active patches from global registry
        // Optionally filter by group
        std::string group_filter = params.value("group", "");
        json result = PatchRegistry::list_patches(group_filter);
        ConsoleLogger::log(("MCP: list_active_patches result: " + result.dump()).c_str());
        return result;

    } else if (tool == "get_patch_info") {
        // Get detailed information about a patch
        std::string patch_id = params.value("patch_id", "");

        if (patch_id.empty()) {
            return {
                {"error", {{"code", -32602}, {"message", "Missing required parameter: patch_id"}}}};
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

        // Parse attributes (handle both string and object)
        json attributes = json::object();
        if (params.contains("attributes")) {
            if (params["attributes"].is_string()) {
                // Parse JSON string
                try {
                    attributes = json::parse(params["attributes"].get<std::string>());
                } catch (const json::exception& e) {
                    ConsoleLogger::log(
                        ("Failed to parse attributes JSON: " + std::string(e.what())).c_str());
                }
            } else if (params["attributes"].is_object()) {
                // Use object directly
                attributes = params["attributes"];
            }
        }

        if (patch_id.empty() || obj_type.empty()) {
            return {{"error",
                     {{"code", -32602},
                      {"message", "Missing required parameters: patch_id and obj_type"}}}};
        }

        // Get position array
        if (!params.contains("position") || !params["position"].is_array() ||
            params["position"].size() < 2) {
            return {{"error",
                     {{"code", -32602},
                      {"message", "Invalid position parameter: must be array [x, y]"}}}};
        }

        double x = params["position"][0].get<double>();
        double y = params["position"][1].get<double>();

        // Find patch
        t_maxmcp* patch = PatchRegistry::find_patch(patch_id);
        if (!patch) {
            return {{"error", {{"code", -32602}, {"message", "Patch not found: " + patch_id}}}};
        }

        // Create defer data
        t_add_object_data* data =
            new t_add_object_data{patch, obj_type, x, y, varname, arguments, attributes};

        // Create atom to hold pointer
        t_atom a;
        atom_setobj(&a, data);

        // Defer to main thread (CRITICAL for thread safety)
        defer(patch, (method)add_object_deferred, gensym("add_object"), 1, &a);

        return {{"result",
                 {{"status", "success"},
                  {"patch_id", patch_id},
                  {"obj_type", obj_type},
                  {"position", json::array({x, y})},
                  {"arguments", arguments}}}};

    } else if (tool == "remove_max_object") {
        // Parse parameters
        std::string patch_id = params.value("patch_id", "");
        std::string varname = params.value("varname", "");

        if (patch_id.empty() || varname.empty()) {
            return {{"error",
                     {{"code", -32602},
                      {"message", "Missing required parameters: patch_id and varname"}}}};
        }

        // Find patch
        t_maxmcp* patch = PatchRegistry::find_patch(patch_id);
        if (!patch) {
            return {{"error", {{"code", -32602}, {"message", "Patch not found: " + patch_id}}}};
        }

        // Create defer data
        t_remove_object_data* data = new t_remove_object_data{patch, varname};

        // Create atom to hold pointer
        t_atom a;
        atom_setobj(&a, data);

        // Defer to main thread
        defer(patch, (method)remove_object_deferred, gensym("remove_object"), 1, &a);

        return {{"result", {{"status", "success"}, {"patch_id", patch_id}, {"varname", varname}}}};

    } else if (tool == "set_object_attribute") {
        // Parse parameters
        std::string patch_id = params.value("patch_id", "");
        std::string varname = params.value("varname", "");
        std::string attribute = params.value("attribute", "");

        if (patch_id.empty() || varname.empty() || attribute.empty()) {
            return {
                {"error",
                 {{"code", -32602},
                  {"message", "Missing required parameters: patch_id, varname, and attribute"}}}};
        }

        if (!params.contains("value")) {
            return {
                {"error", {{"code", -32602}, {"message", "Missing required parameter: value"}}}};
        }

        json value = params["value"];

        // Find patch
        t_maxmcp* patch = PatchRegistry::find_patch(patch_id);
        if (!patch) {
            return {{"error", {{"code", -32602}, {"message", "Patch not found: " + patch_id}}}};
        }

        // Create defer data
        t_set_attribute_data* data = new t_set_attribute_data{patch, varname, attribute, value};

        // Create atom to hold pointer
        t_atom a;
        atom_setobj(&a, data);

        // Defer to main thread
        defer(patch, (method)set_attribute_deferred, gensym("set_attribute"), 1, &a);

        return {{"result",
                 {{"status", "success"},
                  {"patch_id", patch_id},
                  {"varname", varname},
                  {"attribute", attribute}}}};

    } else if (tool == "connect_max_objects") {
        // Parse parameters
        std::string patch_id = params.value("patch_id", "");
        std::string src_varname = params.value("src_varname", "");
        std::string dst_varname = params.value("dst_varname", "");
        long outlet = params.value("outlet", -1);
        long inlet = params.value("inlet", -1);

        if (patch_id.empty() || src_varname.empty() || dst_varname.empty() || outlet < 0 ||
            inlet < 0) {
            return {{"error",
                     {{"code", -32602}, {"message", "Missing or invalid required parameters"}}}};
        }

        // Find patch
        t_maxmcp* patch = PatchRegistry::find_patch(patch_id);
        if (!patch) {
            return {{"error", {{"code", -32602}, {"message", "Patch not found: " + patch_id}}}};
        }

        // Create defer data
        t_connect_objects_data* data =
            new t_connect_objects_data{patch, src_varname, outlet, dst_varname, inlet};

        // Create atom to hold pointer
        t_atom a;
        atom_setlong(&a, reinterpret_cast<t_atom_long>(data));

        // Defer to main thread
        defer(patch, (method)connect_objects_deferred, gensym("connect_objects"), 1, &a);

        return {{"result",
                 {{"status", "success"},
                  {"patch_id", patch_id},
                  {"src_varname", src_varname},
                  {"outlet", outlet},
                  {"dst_varname", dst_varname},
                  {"inlet", inlet}}}};

    } else if (tool == "disconnect_max_objects") {
        // Parse parameters
        std::string patch_id = params.value("patch_id", "");
        std::string src_varname = params.value("src_varname", "");
        std::string dst_varname = params.value("dst_varname", "");
        long outlet = params.value("outlet", -1);
        long inlet = params.value("inlet", -1);

        if (patch_id.empty() || src_varname.empty() || dst_varname.empty() || outlet < 0 ||
            inlet < 0) {
            return {{"error",
                     {{"code", -32602}, {"message", "Missing or invalid required parameters"}}}};
        }

        // Find patch
        t_maxmcp* patch = PatchRegistry::find_patch(patch_id);
        if (!patch) {
            return {{"error", {{"code", -32602}, {"message", "Patch not found: " + patch_id}}}};
        }

        // Create defer data
        t_disconnect_objects_data* data =
            new t_disconnect_objects_data{patch, src_varname, outlet, dst_varname, inlet};

        // Create atom to hold pointer
        t_atom a;
        atom_setlong(&a, reinterpret_cast<t_atom_long>(data));

        // Defer to main thread
        defer(patch, (method)disconnect_objects_deferred, gensym("disconnect_objects"), 1, &a);

        return {{"result",
                 {{"status", "success"},
                  {"patch_id", patch_id},
                  {"src_varname", src_varname},
                  {"outlet", outlet},
                  {"dst_varname", dst_varname},
                  {"inlet", inlet}}}};

    } else if (tool == "get_objects_in_patch") {
        // Parse parameters
        std::string patch_id = params.value("patch_id", "");

        if (patch_id.empty()) {
            return {
                {"error", {{"code", -32602}, {"message", "Missing required parameter: patch_id"}}}};
        }

        // Find patch
        t_maxmcp* patch = PatchRegistry::find_patch(patch_id);
        if (!patch) {
            return {{"error", {{"code", -32602}, {"message", "Patch not found: " + patch_id}}}};
        }

        // Allocate DeferredResult on heap (lives until callback completes)
        DeferredResult* deferred_result = new DeferredResult();

        t_get_objects_data* data = new t_get_objects_data{patch, deferred_result};

        // Create atom to hold pointer
        t_atom a;
        atom_setobj(&a, data);

        // Defer to main thread
        defer(patch, (method)get_objects_deferred, gensym("get_objects"), 1, &a);

        // Wait for defer callback to complete (timeout: 5 seconds)
        if (!deferred_result->wait_for(std::chrono::milliseconds(5000))) {
            delete deferred_result;
            return {{"error",
                     {{"code", -32603}, {"message", "Timeout waiting for patch object list"}}}};
        }

        // Success - copy result before cleanup
        json result = deferred_result->result;
        delete deferred_result;

        return {{"result", result}};

    } else if (tool == "get_avoid_rect_position") {
        // Parse parameters
        std::string patch_id = params.value("patch_id", "");
        double width = params.value("width", 50.0);
        double height = params.value("height", 20.0);

        if (patch_id.empty()) {
            return {
                {"error", {{"code", -32602}, {"message", "Missing required parameter: patch_id"}}}};
        }

        // Find patch
        t_maxmcp* patch = PatchRegistry::find_patch(patch_id);
        if (!patch) {
            return {{"error", {{"code", -32602}, {"message", "Patch not found: " + patch_id}}}};
        }

        // Allocate DeferredResult on heap (lives until callback completes)
        DeferredResult* deferred_result = new DeferredResult();

        t_get_position_data* data = new t_get_position_data{patch, width, height, deferred_result};

        // Create atom to hold pointer
        t_atom a;
        atom_setobj(&a, data);

        // Defer to main thread
        defer(patch, (method)get_position_deferred, gensym("get_position"), 1, &a);

        // Wait for defer callback to complete (timeout: 5 seconds)
        if (!deferred_result->wait_for(std::chrono::milliseconds(5000))) {
            delete deferred_result;
            return {{"error",
                     {{"code", -32603}, {"message", "Timeout waiting for position calculation"}}}};
        }

        // Success - copy result before cleanup
        json result = deferred_result->result;
        delete deferred_result;

        return {{"result", result}};

    } else {
        return {{"error", {{"code", -32602}, {"message", "Unknown tool: " + tool}}}};
    }
}

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
