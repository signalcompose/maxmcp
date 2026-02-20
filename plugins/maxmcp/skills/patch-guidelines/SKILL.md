---
name: patch-guidelines
description: |
  Guidelines for creating Max/MSP patches with MaxMCP. Use this skill when:
  - Creating new Max patches via MaxMCP
  - Adding Max objects with add_max_object
  - Connecting objects with connect_max_objects
  - Planning patch layout and organization
  - Building audio/MIDI processing patches
  - Asking about MaxMCP patch creation best practices
invocation: user
---

# MaxMCP Patch Creation Guidelines

This skill provides comprehensive guidelines for creating well-organized, maintainable Max/MSP patches using MaxMCP's MCP tools.

## Pre-Creation Checklist

Before creating a patch, verify:

1. **MaxMCP Connection**: Ensure maxmcp agent is running and connected
2. **Target Patch**: Use `get_frontmost_patch` or `list_active_patches` to identify the target
3. **Existing Objects**: Use `get_objects_in_patch` to understand current state
4. **Layout Planning**: Plan object positions before creation

## Core Principles

### 1. Signal Flow Direction

Always follow the **top-to-bottom, left-to-right** signal flow convention:

```
[Input Sources]     ← Top of patch
      ↓
[Processing]        ← Middle
      ↓
[Output/Display]    ← Bottom of patch
```

### 2. Object Placement Strategy

Use `get_avoid_rect_position` to find safe positions that don't overlap existing objects:

```javascript
// Before adding an object, find a safe position
const position = await mcp.get_avoid_rect_position({
  patch_id: "...",
  near_x: 100,
  near_y: 200,
  width: 80,
  height: 20
});
```

### 3. Grid-Based Layout

Align objects to a consistent grid:
- **Horizontal spacing**: 100-120 pixels between columns
- **Vertical spacing**: 40-60 pixels between rows
- **Section gaps**: 80-100 pixels between logical sections

### 4. Logical Grouping

Group related objects together:
- **Input section**: Top area for external inputs (adc~, midiin, etc.)
- **Processing section**: Middle area for signal processing
- **Control section**: Parameters, UI elements
- **Output section**: Bottom area for outputs (dac~, midiout, etc.)

## Object Creation Best Practices

### Varname Conventions

Always provide meaningful varnames for important objects:

```javascript
await mcp.add_max_object({
  patch_id: "...",
  object_type: "cycle~",
  args: "440",
  varname: "osc_main",  // Meaningful, descriptive name
  position: [100, 200]
});
```

**Naming patterns**:
- `osc_*` for oscillators
- `filt_*` for filters
- `env_*` for envelopes
- `gain_*` for gain controls
- `ctrl_*` for UI controls
- `in_*` / `out_*` for I/O

### Connection Guidelines

Connect objects using their varnames:

```javascript
await mcp.connect_max_objects({
  patch_id: "...",
  source_varname: "osc_main",
  source_outlet: 0,
  dest_varname: "gain_master",
  dest_inlet: 0
});
```

**Connection rules**:
- Connect signal outlets (0) to signal inlets
- Verify inlet/outlet indices before connecting
- Use `get_patch_info` to check object connections

## Common Patterns

### Audio Signal Chain

```
cycle~ → *~ → dac~
```

1. Create oscillator at top
2. Create gain control (*~) below
3. Create dac~ at bottom
4. Connect in order

### MIDI Processing

```
midiin → midiparse → [processing] → noteout
```

### Subpatcher Organization

For complex patches, use subpatchers (p object):
- Group related functionality
- Use send/receive for communication
- Document inputs/outputs

## Reference Documentation

For detailed guidelines, see:
- [Layout Rules](reference/layout-rules.md) - Detailed positioning and spacing rules
- [Naming Conventions](reference/naming-conventions.md) - Varname and object naming standards
- [JavaScript Guide](reference/javascript-guide.md) - v8/v8ui scripting recommendations

## MCP Tools Quick Reference

| Tool | Purpose |
|------|---------|
| `list_active_patches` | List registered patches |
| `get_frontmost_patch` | Get currently focused patch |
| `get_objects_in_patch` | List objects in a patch |
| `get_patch_info` | Get patch metadata |
| `add_max_object` | Create a new object |
| `set_object_attribute` | Modify object properties |
| `connect_max_objects` | Create patchcord |
| `disconnect_max_objects` | Remove patchcord |
| `get_patchlines` | List all patchcords with coordinates and midpoints |
| `set_patchline_midpoints` | Add/remove midpoints to fold patchcords |
| `remove_max_object` | Delete an object |
| `get_avoid_rect_position` | Find safe position |
| `get_console_log` | Retrieve Max console messages |
