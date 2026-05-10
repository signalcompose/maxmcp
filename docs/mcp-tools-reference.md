# MaxMCP MCP Tools Reference

**Last Updated**: 2026-05-09

This document lists all MCP tools available in MaxMCP with their response shapes and behavioral notes.

> **Parameter schemas are not duplicated here.** The authoritative parameter schema (types, required flags, descriptions) is defined in C++ at [`src/tools/`](../src/tools/) and served live to MCP clients via `tools/list`. Any MCP-compliant client can introspect parameters at runtime; humans should consult the source files directly to avoid documentation drift.

---

## Overview

MaxMCP provides 26 MCP tools for controlling Max/MSP patches through natural language commands.

| Category | Count | Source File |
|----------|-------|-------------|
| Patch Management | 3 | [`src/tools/patch_tools.cpp`](../src/tools/patch_tools.cpp) |
| Object Operations | 12 | [`src/tools/object_tools.cpp`](../src/tools/object_tools.cpp) |
| Connection Operations | 4 | [`src/tools/connection_tools.cpp`](../src/tools/connection_tools.cpp) |
| Patch State | 3 | [`src/tools/state_tools.cpp`](../src/tools/state_tools.cpp) |
| Hierarchy | 2 | [`src/tools/hierarchy_tools.cpp`](../src/tools/hierarchy_tools.cpp) |
| Utilities | 2 | [`src/tools/utility_tools.cpp`](../src/tools/utility_tools.cpp) |

---

## Patch Management

### `list_active_patches`

List all registered MaxMCP client patches. Optionally filter by group.

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

### `get_objects_in_patch`

List all objects in a patch with metadata. The optional `mode` parameter narrows the response shape to reduce token usage when only specific fields are needed.

**Response (default — full metadata)**:
Includes `index`, `varname` (when set), `maxclass`, `text`, `position`, `size`.
```json
{
  "result": {
    "patch_id": "synth_a7f2",
    "objects": [
      {
        "index": 0,
        "varname": "osc1",
        "maxclass": "cycle~",
        "text": "cycle~ 440",
        "position": [100, 100],
        "size": [80, 22]
      }
    ],
    "count": 1
  }
}
```

**Response (`mode: "layout"`)**:
Includes `varname` (when set), `position`, `size` only. Use for layout/positioning work.
```json
{
  "result": {
    "patch_id": "synth_a7f2",
    "objects": [
      {"varname": "osc1", "position": [100, 100], "size": [80, 22]}
    ],
    "count": 1
  }
}
```

**Response (`mode: "identity"`)**:
Includes `index`, `varname` (when set), `maxclass`, `text` only. Use for naming and inspection (e.g., before `assign_varnames`).
```json
{
  "result": {
    "patch_id": "synth_a7f2",
    "objects": [
      {"index": 0, "varname": "osc1", "maxclass": "cycle~", "text": "cycle~ 440"}
    ],
    "count": 1
  }
}
```

### `set_object_attribute`

Set an attribute of a Max object. Multi-value attributes (`patching_rect`, `bgcolor`, etc.) accept JSON arrays of numbers.

### `get_object_attribute`

Get the value of an attribute of a Max object. Uses `object_attr_getvalueof()` internally.

**Response**:
```json
{
  "result": {
    "varname": "osc1",
    "attribute": "patching_rect",
    "value": [100, 200, 80, 22]
  }
}
```

**Notes**:
- Returns scalar value for single-value attributes, array for multi-value attributes
- Returns an error if the attribute does not exist or has no value

### `get_object_io_info`

Get inlet and outlet count for an object.

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

### `get_object_value`

Get the current value of a Max object. Uses `object_getvalueof()` internally. Works with objects that implement the getvalueof interface (e.g., number boxes, flonum, sliders, dials).

**Response**:
```json
{
  "result": {
    "varname": "freq_num",
    "value": 440
  }
}
```

**Notes**:
- Returns a number or array depending on the object type
- Returns an error if the object does not support `getvalueof` or has no value

---

## Connection Operations

### `connect_max_objects`

Create a patchcord connection between two Max objects.

### `disconnect_max_objects`

Remove a patchcord connection between two Max objects.

### `get_patchlines`

List all patchlines (connections) in a patch. The optional `mode` parameter narrows the response shape to reduce token usage when only specific fields are needed.

**Response (default — full metadata)**:
Includes topology (`src_varname`, `outlet`, `dst_varname`, `inlet`), `start_point`, `end_point`, `num_midpoints`, optional `midpoints`, `hidden`, `color`. Folded patchcords also include `midpoints` as an array of `{x, y}` points.
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

**Response (`mode: "geometry"`)**:
Drops `hidden` and `color`. Use for layout verification (overlap detection, upward connection detection) where visual attributes are noise.
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
        "num_midpoints": 0
      }
    ],
    "count": 1
  }
}
```

**Response (`mode: "connections"`)**:
Topology only. Use for connectivity checks and inspecting an existing patch before editing (insert/replace/refactor flows).
```json
{
  "result": {
    "patch_id": "synth_a7f2",
    "patchlines": [
      {"src_varname": "osc1", "outlet": 0, "dst_varname": "dac", "inlet": 0}
    ],
    "count": 1
  }
}
```

### `set_patchline_midpoints`

Set midpoint coordinates for a patchcord. Pass an array of `{x, y}` objects to fold the cord, or an empty array to straighten it.

---

## Patch State

### `get_patch_lock_state`

Get the lock/edit state of a patch.

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

Retrieve recent Max Console messages. Optionally clear the log after reading.

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
