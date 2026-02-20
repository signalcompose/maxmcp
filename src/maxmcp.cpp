/**
    @file maxmcp.cpp
    MaxMCP - Unified MCP Server for Max/MSP

    Main implementation file for MaxMCP external object.
    Supports two modes via @mode attribute:
    - "agent": MCP server mode (WebSocket server, MCP protocol handler)
    - "patch": Patch registration mode (registers patch with global registry)

    @ingroup maxmcp
*/

#include "maxmcp.h"

#include "mcp_server.h"
#include "tools/tool_common.h"
#include "utils/console_logger.h"
#include "utils/patch_registry.h"
#include "utils/uuid_generator.h"
#include "websocket_server.h"

#include <cstdlib>
#include <iostream>
#include <sstream>

#include <nlohmann/json.hpp>

using json = nlohmann::json;

// Global class pointer
static t_class* maxmcp_class = nullptr;

// Global singleton instance (for agent mode)
static t_maxmcp* g_agent_instance = nullptr;

/**
 * @brief Main entry point for the MaxMCP external
 *
 * Called when the external is loaded by Max.
 * Registers the unified object class supporting both agent and patch modes.
 *
 * @param r Module reference (unused)
 */
void ext_main(void* r) {
    t_class* c;

    c = class_new("maxmcp", (method)maxmcp_new, (method)maxmcp_free, (long)sizeof(t_maxmcp),
                  nullptr,  // No class flags
                  A_GIMME,  // Accept variable arguments
                  0);

    // Add assist method for inlet/outlet descriptions
    class_addmethod(c, (method)maxmcp_assist, "assist", A_CANT, 0);

    // Add notify method for patcher lifecycle events (patch mode)
    class_addmethod(c, (method)maxmcp_notify, "notify", A_CANT, 0);

    // Add agent mode methods
    class_addmethod(c, (method)maxmcp_start, "start", 0);
    class_addmethod(c, (method)maxmcp_stop, "stop", 0);

    // ========================================
    // Basic Configuration
    // ========================================

    // Mode attribute (required)
    CLASS_ATTR_SYM(c, "mode", 0, t_maxmcp, mode);
    CLASS_ATTR_LABEL(c, "mode", 0, "Operation Mode");
    CLASS_ATTR_ENUM(c, "mode", 0, "agent patch");
    CLASS_ATTR_DEFAULT(c, "mode", 0, "patch");
    CLASS_ATTR_BASIC(c, "mode", 0);
    CLASS_ATTR_SAVE(c, "mode", 0);
    CLASS_ATTR_CATEGORY(c, "mode", 0, "Basic");
    CLASS_ATTR_STYLE(c, "mode", 0, "enum");
    CLASS_METHOD_ATTR_PARSE(c, "mode", "description", gensym("symbol"), 0,
                            "\"agent\" for MCP server mode, \"patch\" for client mode");

    // ========================================
    // Agent Mode Attributes
    // ========================================

    CLASS_ATTR_LONG(c, "port", 0, t_maxmcp, port);
    CLASS_ATTR_LABEL(c, "port", 0, "WebSocket Port");
    CLASS_ATTR_DEFAULT(c, "port", 0, "7400");
    CLASS_ATTR_ACCESSORS(c, "port", nullptr, (method)maxmcp_port_set);
    CLASS_ATTR_CATEGORY(c, "port", 0, "Agent Mode");
    CLASS_ATTR_STYLE(c, "port", 0, "text");
    CLASS_METHOD_ATTR_PARSE(c, "port", "description", gensym("symbol"), 0,
                            "WebSocket port for agent mode (default: 7400)");

    CLASS_ATTR_CHAR(c, "debug", 0, t_maxmcp, debug);
    CLASS_ATTR_STYLE_LABEL(c, "debug", 0, "onoff", "Debug Mode");
    CLASS_ATTR_DEFAULT(c, "debug", 0, "0");
    CLASS_ATTR_CATEGORY(c, "debug", 0, "Agent Mode");
    CLASS_METHOD_ATTR_PARSE(c, "debug", "description", gensym("symbol"), 0,
                            "Enable debug logging (0=off, 1=on)");

    // ========================================
    // Patch Mode Attributes
    // ========================================

    CLASS_ATTR_SYM(c, "alias", 0, t_maxmcp, alias);
    CLASS_ATTR_LABEL(c, "alias", 0, "Custom Patch ID");
    CLASS_ATTR_BASIC(c, "alias", 0);
    CLASS_ATTR_SAVE(c, "alias", 0);
    CLASS_ATTR_CATEGORY(c, "alias", 0, "Patch Mode");
    CLASS_ATTR_STYLE(c, "alias", 0, "text");
    CLASS_METHOD_ATTR_PARSE(c, "alias", "description", gensym("symbol"), 0,
                            "Custom patch ID (optional, defaults to auto-generated)");

    CLASS_ATTR_SYM(c, "group", 0, t_maxmcp, group);
    CLASS_ATTR_LABEL(c, "group", 0, "Patch Group");
    CLASS_ATTR_BASIC(c, "group", 0);
    CLASS_ATTR_SAVE(c, "group", 0);
    CLASS_ATTR_CATEGORY(c, "group", 0, "Patch Mode");
    CLASS_ATTR_STYLE(c, "group", 0, "text");
    CLASS_METHOD_ATTR_PARSE(c, "group", "description", gensym("symbol"), 0,
                            "Patch group for organizing multiple patches");

    // Register the class
    class_register(CLASS_BOX, c);
    maxmcp_class = c;

    post("MaxMCP: Unified external loaded (agent + patch modes)");
}

/**
 * @brief Constructor for MaxMCP object
 *
 * Creates a new instance of the MaxMCP object.
 * Initializes based on @mode attribute (default: "patch").
 *
 * @param s Symbol (object name)
 * @param argc Number of arguments
 * @param argv Array of atoms (arguments)
 * @return Pointer to new object instance, or nullptr on failure
 */
void* maxmcp_new(t_symbol* s, long argc, t_atom* argv) {
    t_maxmcp* x = nullptr;

    // Allocate object memory
    x = (t_maxmcp*)object_alloc(maxmcp_class);

    if (x) {
        // Initialize mode to default ("patch")
        x->mode = gensym("patch");

        // Initialize all fields to safe defaults
        x->outlet_log = nullptr;
        x->initialized = false;
        new (&x->protocol_version) std::string("");
        x->running = false;
        x->ws_server = nullptr;
        x->port = 7400;
        x->debug = false;

        x->patcher = nullptr;
        new (&x->patch_id) std::string("");
        new (&x->display_name) std::string("");
        new (&x->patcher_name) std::string("");
        x->alias = gensym("");
        x->group = gensym("");

        // Process attributes from arguments (@mode, @alias, @group, etc.)
        attr_args_process(x, argc, argv);

        // Initialize based on mode
        if (x->mode == gensym("agent")) {
            // === AGENT MODE INITIALIZATION ===

            // Singleton check
            if (g_agent_instance != nullptr) {
                object_error(nullptr,
                             "maxmcp (agent mode) already exists! Only one agent instance allowed");
                // Clean up and return nullptr
                x->patch_id.~basic_string();
                x->display_name.~basic_string();
                x->patcher_name.~basic_string();
                x->protocol_version.~basic_string();
                return nullptr;
            }

            x->running = true;

            // Create outlet for bang signal (server ready notification)
            x->outlet_log = outlet_new((t_object*)x, NULL);

            // Check environment variable for port
            if (const char* env_port = std::getenv("MAXMCP_PORT")) {
                x->port = std::atoi(env_port);
            }

            // Create WebSocket server
            x->ws_server = new WebSocketServer((int)x->port);

            // Set synchronous message callback - called from LWS_CALLBACK_RECEIVE
            x->ws_server->set_sync_message_callback(
                [x](const std::string& client_id, const std::string& message) -> std::string {
                    try {
                        ConsoleLogger::log(("Sync callback: Processing message (" +
                                            std::to_string(message.length()) + " bytes)")
                                               .c_str());

                        // Route to MCP server (thread-safe)
                        std::string response =
                            MCPServer::get_instance()->handle_request_string(message);

                        ConsoleLogger::log(("Sync callback: Generated response (" +
                                            std::to_string(response.length()) + " bytes)")
                                               .c_str());

                        return response;

                    } catch (const std::exception& e) {
                        object_error((t_object*)x, "Request processing error: %s", e.what());

                        // Return error response
                        json error_response = {
                            {"jsonrpc", "2.0"},
                            {"error",
                             {{"code", ToolCommon::ErrorCode::INTERNAL_ERROR},
                              {"message", std::string("Internal error: ") + e.what()}}},
                            {"id", nullptr}};

                        return error_response.dump();
                    }
                });

            // Start WebSocket server
            if (!x->ws_server->start()) {
                object_error((t_object*)x, "Failed to start WebSocket server");
                delete x->ws_server;
                x->ws_server = nullptr;
                x->patch_id.~basic_string();
                x->display_name.~basic_string();
                x->patcher_name.~basic_string();
                x->protocol_version.~basic_string();
                return nullptr;
            }

            // Start MCP server
            MCPServer::get_instance()->start();

            // Set global singleton
            g_agent_instance = x;

            object_post((t_object*)x, "maxmcp (agent mode) started on port %ld", x->port);
            ConsoleLogger::log(
                ("maxmcp: WebSocket server listening on port " + std::to_string(x->port)).c_str());

            // Defer bang output to main thread (outlets must be called from main thread)
            defer_low((t_object*)x, (method)maxmcp_send_ready_bang, NULL, 0, NULL);

        } else {
            // === PATCH MODE INITIALIZATION ===

            // Get patcher reference and name FIRST (needed for patch ID generation)
            object_obex_lookup(x, gensym("#P"), &x->patcher);

            // Initialize patch metadata
            std::string patcher_name_str = "Untitled";
            if (x->patcher) {
                t_symbol* patcher_name = object_attr_getsym(x->patcher, gensym("name"));
                if (patcher_name && patcher_name->s_name) {
                    patcher_name_str = patcher_name->s_name;
                }
            }

            // Generate patch ID: {patchname}_{uuid8}
            x->patch_id = generate_patch_id(patcher_name_str, 8);
            x->display_name = remove_extension(patcher_name_str);
            x->patcher_name = patcher_name_str;

            // If alias attribute is set, override patch_id
            if (x->alias && x->alias->s_name && strlen(x->alias->s_name) > 0) {
                x->patch_id = x->alias->s_name;
                object_post((t_object*)x, "Using custom patch ID: %s", x->patch_id.c_str());
            }

            // Register with global patch registry
            PatchRegistry::register_patch(x);

            // Subscribe to patcher notifications (for lifecycle management)
            if (x->patcher) {
                object_attach_byptr_register(x, x->patcher, CLASS_NOBOX);
            }

            // Post initialization message
            object_post((t_object*)x, "maxmcp (patch mode) initialized (ID: %s)",
                        x->patch_id.c_str());
            if (x->group && x->group->s_name && strlen(x->group->s_name) > 0) {
                object_post((t_object*)x, "Group: %s", x->group->s_name);
            }
        }
    }

    return x;
}

/**
 * @brief Destructor for MaxMCP object
 *
 * Cleans up resources when object is deleted.
 * Cleanup logic depends on mode.
 *
 * @param x Pointer to MaxMCP object
 */
void maxmcp_free(t_maxmcp* x) {
    if (x) {
        if (x->mode == gensym("agent")) {
            // === AGENT MODE CLEANUP ===

            // Stop running
            x->running = false;

            // Stop WebSocket server
            if (x->ws_server) {
                x->ws_server->stop();
                delete x->ws_server;
                x->ws_server = nullptr;
            }

            // Stop MCP server
            MCPServer::destroy_instance();

            ConsoleLogger::log("maxmcp (agent mode) destroyed");

            // Clear singleton
            g_agent_instance = nullptr;

            object_post((t_object*)x, "maxmcp (agent mode) freed");

        } else {
            // === PATCH MODE CLEANUP ===

            // Detach from patcher notifications
            if (x->patcher) {
                object_detach_byptr(x, x->patcher);
            }

            // Unregister from global patch registry
            PatchRegistry::unregister_patch(x);

            object_post((t_object*)x, "maxmcp (patch mode) destroyed (ID: %s)",
                        x->patch_id.c_str());
        }

        // Explicitly destroy std::string members (placement delete)
        x->patch_id.~basic_string();
        x->display_name.~basic_string();
        x->patcher_name.~basic_string();
        x->protocol_version.~basic_string();
    }
}

/**
 * @brief Assist method for inlet/outlet descriptions
 *
 * Provides text descriptions for inlets and outlets when hovering in Max.
 *
 * @param x Pointer to MaxMCP object
 * @param b Unused
 * @param m ASSIST_INLET or ASSIST_OUTLET
 * @param a Inlet/outlet index
 * @param s String buffer to write description
 */
void maxmcp_assist(t_maxmcp* x, void* b, long m, long a, char* s) {
    if (x->mode == gensym("agent")) {
        if (m == ASSIST_INLET) {
            snprintf(s, 256, "Control messages (start, stop)");
        } else {
            snprintf(s, 256, "Ready bang");
        }
    } else {
        if (m == ASSIST_INLET) {
            snprintf(s, 256, "Control messages");
        } else {
            snprintf(s, 256, "Status output");
        }
    }
}

/**
 * @brief Notification handler for patcher lifecycle events (patch mode only)
 *
 * Called when patcher sends notifications (e.g., close, free).
 * Handles automatic cleanup and unregistration.
 *
 * @param x Pointer to MaxMCP object
 * @param s Sender symbol
 * @param msg Notification message (e.g., "free")
 * @param sender Sender object
 * @param data Additional data (unused)
 */
void maxmcp_notify(t_maxmcp* x, t_symbol* s, t_symbol* msg, void* sender, void* data) {
    if (!x || !msg || x->mode != gensym("patch")) {
        return;
    }

    // Check if notification is from our patcher
    if (sender == x->patcher) {
        // Handle patcher close/free event
        if (msg == gensym("free")) {
            // Unregister from global patch registry
            PatchRegistry::unregister_patch(x);

            object_post((t_object*)x, "maxmcp: patcher closing, unregistered (ID: %s)",
                        x->patch_id.c_str());
        }
    }
}

// ===== AGENT MODE METHODS =====

/**
 * @brief Start agent (called via "start" message)
 *
 * Currently agent starts automatically on creation.
 * This method is reserved for future manual start support.
 *
 * @param x Pointer to MaxMCP object
 */
void maxmcp_start(t_maxmcp* x) {
    if (x->mode != gensym("agent")) {
        object_error((t_object*)x, "start message only valid in agent mode");
        return;
    }

    // Check if WebSocket server exists
    if (!x->ws_server) {
        object_error((t_object*)x, "WebSocket server not initialized");
        return;
    }

    // If already running, do nothing
    if (x->ws_server->is_running()) {
        object_post((t_object*)x, "Agent already running on port %ld", x->port);
        return;
    }

    // Restart server
    x->running = true;
    if (x->ws_server->start()) {
        object_post((t_object*)x, "Agent restarted on port %ld", x->port);
    } else {
        object_error((t_object*)x, "Failed to restart Agent on port %ld", x->port);
        x->running = false;
    }
}

/**
 * @brief Stop agent (called via "stop" message)
 *
 * Stops the WebSocket server and MCP server.
 *
 * @param x Pointer to MaxMCP object
 */
void maxmcp_stop(t_maxmcp* x) {
    if (x->mode != gensym("agent")) {
        object_error((t_object*)x, "stop message only valid in agent mode");
        return;
    }

    x->running = false;

    if (x->ws_server) {
        x->ws_server->stop();
        object_post((t_object*)x, "Agent stopped");
    }
}

/**
 * @brief Defer callback to send ready bang (agent mode)
 *
 * Called from main thread via defer_low() to ensure outlet_bang() is called
 * from proper thread context.
 *
 * @param x Agent instance
 * @param s Symbol (unused)
 * @param argc Argument count (unused)
 * @param argv Arguments (unused)
 */
void maxmcp_send_ready_bang(t_maxmcp* x, t_symbol* s, long argc, t_atom* argv) {
    if (x && x->mode == gensym("agent") && x->outlet_log) {
        outlet_bang(x->outlet_log);
        object_post((t_object*)x, "DEBUG: Ready bang sent from main thread");
    }
}

// ===== ATTRIBUTE ACCESSORS =====

/**
 * @brief Attribute setter for port (agent mode)
 *
 * @param x MaxMCP object
 * @param attr Attribute object
 * @param ac Argument count
 * @param av Argument values
 * @return Error code
 */
t_max_err maxmcp_port_set(t_maxmcp* x, t_object* attr, long ac, t_atom* av) {
    if (ac && av) {
        x->port = atom_getlong(av);
        ConsoleLogger::log(("Port set to " + std::to_string(x->port)).c_str());
    }
    return MAX_ERR_NONE;
}
