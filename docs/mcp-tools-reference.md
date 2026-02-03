# MaxMCP MCP Tools Reference

**Version**: 1.1.0
**Last Updated**: 2026-02-04

This document provides a complete reference for all MCP tools available in MaxMCP.

---

## Overview

MaxMCP provides 22 MCP tools for controlling Max/MSP patches through natural language commands. Tools are organized into categories based on their functionality.

---

## Tool Categories

| Category | Count | Description |
|----------|-------|-------------|
| Patch Management | 3 | List, query, and manage patches |
| Object Operations | 8 | Create, modify, and query objects |
| Connection Operations | 2 | Create and remove patchcords |
| Patch State | 3 | Lock state and dirty flag management |
| Undo/Redo | 2 | Undo transaction management |
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
        "maxclass": "cycle~",
        "varname": "osc1",
        "position": [100, 100],
        "size": [50, 22]
      }
    ],
    "count": 1
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

## Undo/Redo

### `begin_undo_group`

Begin an undo transaction group. Multiple operations within the group become a single undo step.

**Parameters**:
```json
{
  "patch_id": {"type": "string", "required": true},
  "name": {"type": "string", "required": true, "description": "Name for the undo action, e.g., 'Add synthesizer'"}
}
```

**Response**:
```json
{
  "result": {
    "success": true,
    "undo_name": "Add synthesizer"
  }
}
```

**Error** (if undo group already active):
```json
{
  "error": {
    "code": -32600,
    "message": "Undo group already active: Previous action"
  }
}
```

### `end_undo_group`

End an undo transaction group.

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
    "success": true
  }
}
```

**Error** (if no active undo group):
```json
{
  "error": {
    "code": -32600,
    "message": "No active undo group"
  }
}
```

**Usage Example**:
```
1. begin_undo_group(patch_id, "Add synth voice")
2. add_max_object(patch_id, "cycle~", ...)
3. add_max_object(patch_id, "*~", ...)
4. connect_max_objects(patch_id, ...)
5. end_undo_group(patch_id)
→ All operations can be undone with a single Cmd+Z
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
| -32600 | Invalid Request (e.g., undo group state error) |
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
