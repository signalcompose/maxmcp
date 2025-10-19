/**
    @file maxmcp.cpp
    MaxMCP - MCP Server for Max/MSP

    Main implementation file for MaxMCP external object.

    @ingroup maxmcp
*/

#include "maxmcp.h"

// Global class pointer
static t_class* maxmcp_class = nullptr;

/**
 * @brief Main entry point for the MaxMCP external
 *
 * Called when the external is loaded by Max.
 * Registers the object class and its methods.
 *
 * @param r Module reference (unused)
 */
void ext_main(void* r) {
    t_class* c;

    c = class_new("maxmcp",
                  (method)maxmcp_new,
                  (method)maxmcp_free,
                  (long)sizeof(t_maxmcp),
                  nullptr,  // No class flags
                  A_GIMME,  // Accept variable arguments
                  0);

    // Add assist method for inlet/outlet descriptions
    class_addmethod(c, (method)maxmcp_assist, "assist", A_CANT, 0);

    // Register the class
    class_register(CLASS_BOX, c);
    maxmcp_class = c;

    post("MaxMCP: MCP Server for Max/MSP loaded");
}

/**
 * @brief Constructor for MaxMCP object
 *
 * Creates a new instance of the MaxMCP object.
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
        // Post initialization message to Max console
        object_post((t_object*)x, "MaxMCP initialized!");

        // Log number of arguments if any
        if (argc > 0) {
            object_post((t_object*)x, "Created with %ld argument(s)", argc);
        }
    }

    return x;
}

/**
 * @brief Destructor for MaxMCP object
 *
 * Cleans up resources when object is deleted.
 *
 * @param x Pointer to MaxMCP object
 */
void maxmcp_free(t_maxmcp* x) {
    // Cleanup will be added in future tasks
    // For now, just log destruction
    if (x) {
        object_post((t_object*)x, "MaxMCP destroyed");
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
    if (m == ASSIST_INLET) {
        snprintf(s, 256, "Control messages");
    } else {
        snprintf(s, 256, "Status output");
    }
}
