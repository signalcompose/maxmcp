# MaxMCP Manual Testing Guide

**Version**: 1.1.0
**Last Updated**: 2026-02-04
**Total Tools**: 20 MCP Tools

This document provides test cases for all MaxMCP MCP tools. Use these prompts with Claude Code to verify tool functionality.

---

## Prerequisites

### Environment Setup

1. **Max/MSP running** with a patch containing `[maxmcp @mode patch]`
2. **Claude Code** configured with MaxMCP MCP server
3. **Test patch** saved and named (e.g., `test.maxpat`)

### Quick Verification

Before testing, verify the connection:

```
"List all active MaxMCP patches"
```

Expected: A list containing your test patch with its patch_id.

---

## Test Categories

| Category | Tools | Section |
|----------|-------|---------|
| Patch Management | 3 | [Section 1](#1-patch-management) |
| Object Operations | 8 | [Section 2](#2-object-operations) |
| Connection Operations | 2 | [Section 3](#3-connection-operations) |
| Patch State | 3 | [Section 4](#4-patch-state) |
| Hierarchy | 2 | [Section 5](#5-hierarchy) |
| Utilities | 2 | [Section 6](#6-utilities) |

---

## 1. Patch Management

### 1.1 list_active_patches

**Purpose**: List all registered MaxMCP client patches.

**Test Prompt**:
```
List all active MaxMCP patches
```

**Expected Result**:
- Returns array of patches with `patch_id`, `display_name`, `group`
- Count matches number of open patches with `[maxmcp @mode patch]`

**Error Test**:
```
List active patches in group "nonexistent_group"
```
Expected: Empty array or filtered results.

---

### 1.2 get_patch_info

**Purpose**: Get detailed information about a specific patch.

**Test Prompt** (replace `<patch_id>` with actual ID):
```
Get detailed info for patch <patch_id>
```

**Expected Result**:
- Returns `patch_id`, `display_name`, `file_path`, `group`
- File path matches actual .maxpat location

**Error Test**:
```
Get info for patch "invalid_patch_id_12345"
```
Expected: Error with code -32602 (Invalid Params).

---

### 1.3 get_frontmost_patch

**Purpose**: Get the currently focused/frontmost patch.

**Test Prompt**:
```
Which MaxMCP patch is currently in front?
```

**Expected Result**:
- Returns `patch_id` and `display_name` of focused patch
- Matches the patch window that is currently active in Max

**Note**: Click on different Max patch windows and repeat test to verify.

---

## 2. Object Operations

### 2.1 add_max_object

**Purpose**: Add a Max object to a patch.

**Test Prompt**:
```
Add a cycle~ oscillator at position [100, 100] in patch <patch_id> with frequency 440
```

**Expected Result**:
- Object created with varname (e.g., `obj_xxxx`)
- Object visible at specified position
- Arguments applied (440Hz)

**Test Variations**:
```
Add a [number] object at [200, 100] named "freq_display"
```

```
Add a [gain~] object at [150, 200] with @size attribute [100, 150]
```

---

### 2.2 remove_max_object

**Purpose**: Remove a Max object from a patch.

**Test Prompt**:
```
Remove object named "freq_display" from patch <patch_id>
```

**Expected Result**:
- Object removed from patch
- Success status returned

**Error Test**:
```
Remove object "nonexistent_object" from patch <patch_id>
```
Expected: Error with code -32602.

---

### 2.3 get_objects_in_patch

**Purpose**: List all objects in a patch.

**Test Prompt**:
```
List all objects in patch <patch_id>
```

**Expected Result**:
- Array of objects with `maxclass`, `varname`, `position`, `size`
- Count matches visible objects in patch

---

### 2.4 set_object_attribute

**Purpose**: Set an attribute of a Max object.

**Test Prompt**:
```
Set the "bgcolor" attribute of object "mybutton" to [1.0, 0.0, 0.0, 1.0] in patch <patch_id>
```

**Expected Result**:
- Attribute changed (button turns red)
- Success status returned

**Test Variations**:
```
Set the frequency of cycle~ object "osc1" to 880
```

---

### 2.5 get_object_io_info

**Purpose**: Get inlet and outlet count for an object.

**Test Prompt**:
```
How many inlets and outlets does object "osc1" have in patch <patch_id>?
```

**Expected Result**:
- `inlet_count`: number of inlets
- `outlet_count`: number of outlets
- Matches visual inspection in Max

---

### 2.6 get_object_hidden

**Purpose**: Check if an object is hidden.

**Test Prompt**:
```
Is object "myobject" hidden in patch <patch_id>?
```

**Expected Result**:
- `hidden`: true or false
- Matches object visibility in patch

---

### 2.7 set_object_hidden

**Purpose**: Set the visibility of an object.

**Test Prompt**:
```
Hide object "myobject" in patch <patch_id>
```

**Expected Result**:
- Object disappears from patch view
- `hidden`: true in response

**Reverse Test**:
```
Show object "myobject" in patch <patch_id>
```
Expected: Object reappears.

---

### 2.8 redraw_object

**Purpose**: Force redraw of a specific object.

**Test Prompt**:
```
Redraw object "myobject" in patch <patch_id>
```

**Expected Result**:
- Success status
- Object visually refreshed (may not be visually apparent)

---

## 3. Connection Operations

### 3.1 connect_max_objects

**Purpose**: Create a patchcord connection between two objects.

**Setup**: Create two objects first (e.g., `cycle~` and `dac~`).

**Test Prompt**:
```
Connect outlet 0 of "osc1" to inlet 0 of "output" in patch <patch_id>
```

**Expected Result**:
- Patchcord created
- Success status returned

**Verify**: Visual inspection shows connected objects.

---

### 3.2 disconnect_max_objects

**Purpose**: Remove a patchcord connection.

**Test Prompt**:
```
Disconnect outlet 0 of "osc1" from inlet 0 of "output" in patch <patch_id>
```

**Expected Result**:
- Patchcord removed
- Success status returned

---

## 4. Patch State

### 4.1 get_patch_lock_state

**Purpose**: Get the lock/edit state of a patch.

**Test Prompt**:
```
Is patch <patch_id> locked or in edit mode?
```

**Expected Result**:
- `locked`: true (presentation mode) or false (edit mode)
- Matches Max's lock state (Cmd+E toggles)

---

### 4.2 set_patch_lock_state

**Purpose**: Set the lock/edit state of a patch.

**Test Prompt**:
```
Lock patch <patch_id>
```

**Expected Result**:
- Patch switches to presentation mode
- `locked`: true

**Reverse Test**:
```
Unlock patch <patch_id>
```
Expected: Patch switches to edit mode.

---

### 4.3 get_patch_dirty

**Purpose**: Check if a patch has unsaved changes.

**Test Prompt**:
```
Does patch <patch_id> have unsaved changes?
```

**Expected Result**:
- `dirty`: true or false
- Make a change in Max (add/remove object) and test again

---

## 5. Hierarchy

### 5.1 get_parent_patcher

**Purpose**: Get the parent patcher of a subpatcher.

**Setup**: Use a patch that contains a `[p subpatch]` or `[bpatcher]`.

**Test Prompt**:
```
What is the parent patcher of patch <subpatch_id>?
```

**Expected Result**:
- `parent_name`: name of parent patcher
- `has_parent`: true

**Error Test** (on top-level patch):
```
What is the parent patcher of patch <toplevel_id>?
```
Expected: Error indicating no parent (top-level patch).

---

### 5.2 get_subpatchers

**Purpose**: List all subpatchers in a patch.

**Test Prompt**:
```
List all subpatchers in patch <patch_id>
```

**Expected Result**:
- Array of subpatchers with `varname`, `type`, `name`
- Types: "patcher", "bpatcher", "poly~"

---

## 6. Utilities

### 6.1 get_console_log

**Purpose**: Retrieve recent Max Console messages.

**Setup**: Generate console output with `[print]` or error messages.

**Test Prompt**:
```
Show me the last 20 lines from the Max Console
```

**Expected Result**:
- Array of log messages
- Count matches requested lines (or fewer if less available)

**Test with clear**:
```
Show Max Console logs and clear the buffer
```
Expected: Logs returned, subsequent call shows empty or new logs only.

---

### 6.2 get_avoid_rect_position

**Purpose**: Find an empty position for placing new objects.

**Test Prompt**:
```
Find an empty position in patch <patch_id> for a new object with width 100 and height 50
```

**Expected Result**:
- `position`: [x, y] coordinates
- `rationale`: explanation of placement logic

**Verify**: Add object at suggested position, should not overlap existing objects.

---

## Error Handling Tests

### Invalid Patch ID

```
Get info for patch "completely_invalid_id"
```
Expected: Error code -32602, message about patch not found.

### Missing Required Parameter

```
Add an object without specifying position
```
Expected: Error code -32602, message about missing parameter.

### Invalid Object Varname

```
Remove object "" from patch <patch_id>
```
Expected: Error code -32602, message about missing varname.

---

## Integration Test Workflow

Test a complete workflow combining multiple tools:

**Prompt**:
```
In patch <patch_id>:
1. Add a [cycle~ 440] at position [100, 100] named "osc"
2. Add a [*~ 0.5] at position [100, 200] named "gain"
3. Add a [dac~] at position [100, 300] named "output"
4. Connect osc outlet 0 to gain inlet 0
5. Connect gain outlet 0 to output inlet 0
6. Lock the patch
```

**Expected Result**:
- Simple synth patch created
- All connections visible
- Patch in presentation mode

---

## Test Results Template

| Tool | Status | Notes |
|------|--------|-------|
| list_active_patches | ⬜ | |
| get_patch_info | ⬜ | |
| get_frontmost_patch | ⬜ | |
| add_max_object | ⬜ | |
| remove_max_object | ⬜ | |
| get_objects_in_patch | ⬜ | |
| set_object_attribute | ⬜ | |
| get_object_io_info | ⬜ | |
| get_object_hidden | ⬜ | |
| set_object_hidden | ⬜ | |
| redraw_object | ⬜ | |
| connect_max_objects | ⬜ | |
| disconnect_max_objects | ⬜ | |
| get_patch_lock_state | ⬜ | |
| set_patch_lock_state | ⬜ | |
| get_patch_dirty | ⬜ | |
| get_parent_patcher | ⬜ | |
| get_subpatchers | ⬜ | |
| get_console_log | ⬜ | |
| get_avoid_rect_position | ⬜ | |

**Legend**: ✅ Pass | ❌ Fail | ⬜ Not Tested

---

## Troubleshooting

### "Patch not found" Error
- Verify `[maxmcp @mode patch]` is instantiated in the patch
- Check patch is saved (untitled patches may have issues)
- Restart Max and reconnect

### "Timeout" Error
- Max may be unresponsive
- Check Max Console for errors
- Verify WebSocket connection is active

### Objects Not Appearing
- Patch may be locked - unlock first
- Position may be outside visible area
- Check z-order (object may be behind others)

---

## See Also

- [MCP Tools Reference](./mcp-tools-reference.md) - Complete API documentation
- [Architecture](./architecture.md) - System design
- [Development Guide](./development-guide.md) - Contributing guidelines
