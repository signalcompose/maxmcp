# MaxMCP MCP Tools Reference

**Version**: 1.1.0
**Last Updated**: 2026-02-04

This document provides a complete reference for all MCP tools available in MaxMCP.

---

## Overview

MaxMCP provides 24 MCP tools for controlling Max/MSP patches through natural language commands. Tools are organized into categories based on their functionality.

---

## Tool Categories

| Category | Count | Description |
|----------|-------|-------------|
| Patch Management | 3 | List, query, and manage patches |
| Object Operations | 10 | Create, modify, replace, assign varnames, and query objects |
| Connection Operations | 4 | Create, remove, and manage patchcords |
| Patch State | 3 | Lock state and dirty flag management |
| Hierarchy | 2 | Parent/child patcher navigation |
| Utilities | 2 | Console logging and positioning |

---

## Patch Management

### `list_active_patches`

List all registered MaxMCP client patches.

**Parameters**:
```json
{
  "group": {
    "type": "string",
    "description": "Optional group name to filter patches",
    "required": false
  }
}
```

**Response**:
```json
{
  "patches": [
    {
      "patch_id": "synth_a7f2",
      "display_name": "synth",
      "group": "instruments"
    }
  ],
  "count": 1
}
```

### `get_patch_info`

Get detailed information about a specific patch.

**Parameters**:
```json
{
  "patch_id": {
    "type": "string",
    "description": "Patch ID to query",
    "required": true
  }
}
```

**Response**:
```json
{
  "patch_id": "synth_a7f2",
  "display_name": "synth",
  "file_path": "/path/to/synth.maxpat",
  "group": "instruments"
}
```

### `get_frontmost_patch`

Get the currently focused/frontmost patch.

**Parameters**: None

**Response**:
```json
{
  "patch_id": "synth_a7f2",
  "display_name": "synth"
}
```

---

## Object Operations

### `add_max_object`

Add a Max object to a patch.

**Parameters**:
```json
{
  "patch_id": {"type": "string", "required": true},
  "obj_type": {"type": "string", "required": true, "description": "e.g., 'cycle~', 'number', 'button'"},
  "position": {"type": "array", "required": true, "description": "[x, y] coordinates"},
  "varname": {"type": "string", "required": false, "description": "Variable name for the object"},
  "arguments": {"type": "array", "required": false, "description": "Object arguments, e.g., [440]"},
  "attributes": {"type": "object", "required": false, "description": "Object attributes"}
}
```

**Response**:
```json
{
  "result": {
    "status": "success",
    "patch_id": "synth_a7f2",
    "obj_type": "cycle~",
    "position": [100, 100]
  }
}
```

### `remove_max_object`

Remove a Max object from a patch by varname.

**Parameters**:
```json
{
  "patch_id": {"type": "string", "required": true},
  "varname": {"type": "string", "required": true}
}
```

### `get_objects_in_patch`

List all objects in a patch with metadata.

**Parameters**:
```json
{
  "patch_id": {"type": "string", "required": true}
}
```

**Response**:
```json
{
  "result": {
    "patch_id": "synth_a7f2",
    "objects": [
      {
        "index": 0,
        "maxclass": "newobj",
        "text": "cycle~ 440",
        "position": [100, 100],
        "size": [80, 22],
        "varname": "osc1"
      },
      {
        "index": 1,
        "maxclass": "number",
        "text": "",
        "position": [100, 150],
        "size": [50, 22]
      }
    ],
    "count": 2
  }
}
```

### `set_object_attribute`

Set an attribute of a Max object.

**Parameters**:
```json
{
  "patch_id": {"type": "string", "required": true},
  "varname": {"type": "string", "required": true},
  "attribute": {"type": "string", "required": true},
  "value": {"required": true, "description": "Number, string, or array"}
}
```

### `get_object_io_info`

Get inlet and outlet count for an object.

**Parameters**:
```json
{
  "patch_id": {"type": "string", "required": true},
  "varname": {"type": "string", "required": true}
}
```

**Response**:
```json
{
  "result": {
    "varname": "osc1",
    "inlet_count": 2,
    "outlet_count": 1
  }
}
```

### `get_object_hidden`

Check if an object is hidden.

**Parameters**:
```json
{
  "patch_id": {"type": "string", "required": true},
  "varname": {"type": "string", "required": true}
}
```

**Response**:
```json
{
  "result": {
    "varname": "osc1",
    "hidden": false
  }
}
```

### `set_object_hidden`

Set the visibility of an object in a patch.

**Parameters**:
```json
{
  "patch_id": {"type": "string", "required": true},
  "varname": {"type": "string", "required": true},
  "hidden": {"type": "boolean", "required": true, "description": "true=hide, false=show"}
}
```

**Response**:
```json
{
  "result": {
    "success": true,
    "varname": "osc1",
    "hidden": true
  }
}
```

### `redraw_object`

Force redraw of a specific object.

**Parameters**:
```json
{
  "patch_id": {"type": "string", "required": true},
  "varname": {"type": "string", "required": true}
}
```

**Response**:
```json
{
  "result": {
    "success": true,
    "varname": "osc1"
  }
}
```

### `replace_object_text`

Replace the box text of an existing Max object by deleting and recreating it. All patchcord connections are automatically saved and restored.

For regular objects, `new_text` is the full box text including the class name (e.g., `"cycle~ 880"`). For message/comment/textedit objects, `new_text` is the displayed text content.

**Parameters**:
```json
{
  "patch_id": {"type": "string", "required": true},
  "varname": {"type": "string", "required": true, "description": "Variable name of the object"},
  "new_text": {"type": "string", "required": true, "description": "New box text or content"}
}
```

**Response**:
```json
{
  "result": {
    "status": "success",
    "varname": "my_comment",
    "old_text": "cycle~ 440",
    "new_text": "cycle~ 880",
    "reconnected": 3
  }
}
```

**Notes**:
- The object is deleted and recreated with the new text
- Position, varname, presentation state, and hidden state are preserved
- All patchcord connections (both incoming and outgoing) are automatically restored
- For textfield types (message, comment, textedit, live.comment), `new_text` sets the displayed content

### `assign_varnames`

Assign varnames to objects identified by index. Use `get_objects_in_patch` first to get object indices, then assign meaningful varnames based on object type and context. Existing varnames can be overwritten.

**Parameters**:
```json
{
  "patch_id": {"type": "string", "required": true},
  "assignments": {
    "type": "array",
    "required": true,
    "description": "Array of index-varname pairs",
    "items": {
      "index": {"type": "integer", "description": "Object index from get_objects_in_patch"},
      "varname": {"type": "string", "description": "Varname to assign (e.g., 'osc_440', 'gain_ctrl')"}
    }
  }
}
```

**Response**:
```json
{
  "result": {
    "status": "success",
    "assigned": 2,
    "assignments": [
      {"index": 0, "varname": "osc_440", "maxclass": "newobj"},
      {"index": 1, "varname": "gain_ctrl", "maxclass": "number"}
    ]
  }
}
```

**Notes**:
- Objects are identified by their `index` from `get_objects_in_patch` output
- Duplicate varnames in a single call are rejected
- Out-of-range indices return an error
- Existing varnames can be overwritten (useful for renaming)

**Typical workflow**:
1. Call `get_objects_in_patch` to see all objects with their indices and text
2. Identify objects without varnames
3. Call `assign_varnames` with meaningful names based on object type/context
4. Subsequent tools can now reference these objects by varname

---

## Connection Operations

### `connect_max_objects`

Create a patchcord connection between two Max objects.

**Parameters**:
```json
{
  "patch_id": {"type": "string", "required": true},
  "src_varname": {"type": "string", "required": true},
  "outlet": {"type": "number", "required": true, "description": "0-based outlet index"},
  "dst_varname": {"type": "string", "required": true},
  "inlet": {"type": "number", "required": true, "description": "0-based inlet index"}
}
```

### `disconnect_max_objects`

Remove a patchcord connection between two Max objects.

**Parameters**:
```json
{
  "patch_id": {"type": "string", "required": true},
  "src_varname": {"type": "string", "required": true},
  "outlet": {"type": "number", "required": true},
  "dst_varname": {"type": "string", "required": true},
  "inlet": {"type": "number", "required": true}
}
```

### `get_patchlines`

List all patchlines (connections) in a patch with metadata including source/destination, coordinates, color, and visibility.

**Parameters**:
```json
{
  "patch_id": {"type": "string", "required": true}
}
```

**Response**:
```json
{
  "result": {
    "patch_id": "synth_a7f2",
    "patchlines": [
      {
        "src_varname": "osc1",
        "outlet": 0,
        "dst_varname": "dac",
        "inlet": 0,
        "start_point": {"x": 125, "y": 122},
        "end_point": {"x": 125, "y": 172},
        "num_midpoints": 0,
        "hidden": false,
        "color": {"r": 0.0, "g": 0.0, "b": 0.0, "a": 1.0}
      }
    ],
    "count": 1
  }
}
```

### `set_patchline_midpoints`

Set midpoint coordinates for a patchcord. Pass an array of `{x, y}` objects to fold the cord, or an empty array to straighten it.

**Parameters**:
```json
{
  "patch_id": {"type": "string", "required": true},
  "src_varname": {"type": "string", "required": true},
  "outlet": {"type": "number", "required": true},
  "dst_varname": {"type": "string", "required": true},
  "inlet": {"type": "number", "required": true},
  "midpoints": {"type": "array", "required": true, "description": "Array of {x, y} objects, or [] to remove"}
}
```

---

## Patch State

### `get_patch_lock_state`

Get the lock/edit state of a patch.

**Parameters**:
```json
{
  "patch_id": {"type": "string", "required": true}
}
```

**Response**:
```json
{
  "result": {
    "locked": false,
    "patch_id": "synth_a7f2"
  }
}
```

**Notes**:
- `locked: true` = Presentation mode (user interaction mode)
- `locked: false` = Edit mode (patching mode)

### `set_patch_lock_state`

Set the lock/edit state of a patch.

**Parameters**:
```json
{
  "patch_id": {"type": "string", "required": true},
  "locked": {"type": "boolean", "required": true, "description": "true=lock, false=unlock"}
}
```

**Response**:
```json
{
  "result": {
    "success": true,
    "locked": true
  }
}
```

### `get_patch_dirty`

Check if a patch has unsaved changes.

**Parameters**:
```json
{
  "patch_id": {"type": "string", "required": true}
}
```

**Response**:
```json
{
  "result": {
    "dirty": true,
    "patch_id": "synth_a7f2"
  }
}
```

---

## Hierarchy

### `get_parent_patcher`

Get the parent patcher of a subpatcher.

**Parameters**:
```json
{
  "patch_id": {"type": "string", "required": true, "description": "Child patch ID"}
}
```

**Response** (has parent):
```json
{
  "result": {
    "parent_name": "main.maxpat",
    "has_parent": true
  }
}
```

**Error** (no parent):
```json
{
  "error": {
    "code": -32602,
    "message": "No parent patcher (top-level patch)"
  }
}
```

### `get_subpatchers`

List all subpatchers in a patch.

**Parameters**:
```json
{
  "patch_id": {"type": "string", "required": true}
}
```

**Response**:
```json
{
  "result": {
    "subpatchers": [
      {
        "varname": "synth",
        "type": "bpatcher",
        "name": "synth.maxpat"
      },
      {
        "varname": "fx",
        "type": "patcher",
        "name": "effects"
      }
    ],
    "count": 2
  }
}
```

---

## Utilities

### `get_console_log`

Retrieve recent Max Console messages.

**Parameters**:
```json
{
  "lines": {"type": "number", "required": false, "description": "Number of lines (default: 50, max: 1000)"},
  "clear": {"type": "boolean", "required": false, "description": "Clear log after reading (default: false)"}
}
```

**Response**:
```json
{
  "logs": [
    "MaxMCP: Created cycle~ at [100, 100]",
    "MaxMCP: Connected osc1[0] -> dac~[0]"
  ],
  "count": 2
}
```

### `get_avoid_rect_position`

Find an empty position for placing new objects.

**Parameters**:
```json
{
  "patch_id": {"type": "string", "required": true},
  "width": {"type": "number", "required": false, "description": "Object width (default: 50)"},
  "height": {"type": "number", "required": false, "description": "Object height (default: 20)"}
}
```

**Response**:
```json
{
  "result": {
    "position": [250, 50],
    "rationale": "Positioned to the right of existing objects with 50px margin"
  }
}
```

---

## Error Codes

| Code | Meaning |
|------|---------|
| -32600 | Invalid Request |
| -32602 | Invalid Params (missing/invalid parameters, object not found) |
| -32603 | Internal Error (timeout, execution error) |
| -32700 | Parse Error (invalid JSON) |

---

## Threading Notes

All MCP tools execute on the Max main thread using `defer()`. This ensures thread safety when interacting with Max API. Tools that need to return results use a synchronization mechanism with a 5-second timeout.

---

## See Also

- [MaxMCP Architecture](./architecture.md)
- [MaxMCP Specifications](./specifications.md)
- [Manual Testing Guide](./manual-test-new-tools.md)
