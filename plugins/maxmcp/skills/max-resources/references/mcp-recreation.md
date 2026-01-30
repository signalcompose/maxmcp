# MCP Patch Recreation Workflow

This guide describes how to recreate a Max patch from examples using MaxMCP tools.

## Overview

```
Example Patch (.maxpat)
        │
        ▼
   Parse JSON
   (boxes, lines)
        │
        ▼
   Plan Layout
   (apply patch-guidelines)
        │
        ▼
   Create Objects
   (add_max_object)
        │
        ▼
   Connect Objects
   (connect_max_objects)
        │
        ▼
   Set Attributes
   (set_object_attribute)
```

## Step 1: Load Example Patch

```bash
# Find relevant example
./scripts/list-examples.sh synths

# Read patch JSON
cat /Applications/Max.app/Contents/Resources/Examples/synths/fm-simple.maxpat
```

## Step 2: Analyze Structure

### Extract Object List

From the JSON, identify:
- Object types (`maxclass` or first word of `text`)
- Creation arguments (rest of `text`)
- Positions (`patching_rect`)
- Varnames if present

### Extract Connections

From `lines` array:
- Source object and outlet
- Destination object and inlet

### Example Analysis

```json
// Original patch excerpt
{
  "boxes": [
    {"box": {"id": "obj-1", "maxclass": "newobj", "text": "cycle~ 440", "patching_rect": [100, 100, 63, 22]}},
    {"box": {"id": "obj-2", "maxclass": "newobj", "text": "*~ 0.5", "patching_rect": [100, 150, 42, 22]}},
    {"box": {"id": "obj-3", "maxclass": "ezdac~", "patching_rect": [100, 200, 45, 45]}}
  ],
  "lines": [
    {"patchline": {"source": ["obj-1", 0], "destination": ["obj-2", 0]}},
    {"patchline": {"source": ["obj-2", 0], "destination": ["obj-3", 0]}},
    {"patchline": {"source": ["obj-2", 0], "destination": ["obj-3", 1]}}
  ]
}
```

Extracted information:
- Objects: cycle~ 440, *~ 0.5, ezdac~
- Signal chain: cycle~ → *~ → ezdac~ (both channels)

## Step 3: Plan Layout

Apply `patch-guidelines` rules:

1. **Signal flow**: Top-to-bottom
2. **Spacing**: 50px vertical, align horizontally
3. **Varnames**: Descriptive names following conventions

Planned layout:
```
[cycle~ 440]     y=100  varname: osc_main
      │
   [*~ 0.5]      y=150  varname: gain_main
      │
   [ezdac~]      y=200  varname: dac_out
```

## Step 4: Create Objects (MCP)

```javascript
// Create oscillator
await mcp.add_max_object({
    patch_id: "my_patch",
    object_type: "cycle~",
    args: "440",
    varname: "osc_main",
    position: [100, 100]
});

// Create gain
await mcp.add_max_object({
    patch_id: "my_patch",
    object_type: "*~",
    args: "0.5",
    varname: "gain_main",
    position: [100, 150]
});

// Create DAC
await mcp.add_max_object({
    patch_id: "my_patch",
    object_type: "ezdac~",
    varname: "dac_out",
    position: [100, 200]
});
```

## Step 5: Connect Objects (MCP)

```javascript
// osc -> gain
await mcp.connect_max_objects({
    patch_id: "my_patch",
    source_varname: "osc_main",
    source_outlet: 0,
    dest_varname: "gain_main",
    dest_inlet: 0
});

// gain -> dac left
await mcp.connect_max_objects({
    patch_id: "my_patch",
    source_varname: "gain_main",
    source_outlet: 0,
    dest_varname: "dac_out",
    dest_inlet: 0
});

// gain -> dac right
await mcp.connect_max_objects({
    patch_id: "my_patch",
    source_varname: "gain_main",
    source_outlet: 0,
    dest_varname: "dac_out",
    dest_inlet: 1
});
```

## Step 6: Set Attributes (Optional)

```javascript
// If the example uses attributes
await mcp.set_object_attribute({
    patch_id: "my_patch",
    varname: "osc_main",
    attribute_name: "interp",
    attribute_value: "2"
});
```

## Complete Recreation Script

```javascript
async function recreatePatch(sourceFile, targetPatchId) {
    // 1. Read source patch
    const source = JSON.parse(fs.readFileSync(sourceFile));
    const boxes = source.patcher.boxes;
    const lines = source.patcher.lines;

    // 2. Create ID -> varname mapping
    const idMap = {};
    for (let i = 0; i < boxes.length; i++) {
        const box = boxes[i].box;
        const varname = box.varname || `obj_${i}`;
        idMap[box.id] = varname;

        // 3. Create object
        let objType, args;
        if (box.maxclass === 'newobj') {
            const parts = box.text.split(' ');
            objType = parts[0];
            args = parts.slice(1).join(' ');
        } else {
            objType = box.maxclass;
            args = '';
        }

        await mcp.add_max_object({
            patch_id: targetPatchId,
            object_type: objType,
            args: args,
            varname: varname,
            position: [box.patching_rect[0], box.patching_rect[1]]
        });
    }

    // 4. Create connections
    for (const line of lines) {
        const pl = line.patchline;
        await mcp.connect_max_objects({
            patch_id: targetPatchId,
            source_varname: idMap[pl.source[0]],
            source_outlet: pl.source[1],
            dest_varname: idMap[pl.destination[0]],
            dest_inlet: pl.destination[1]
        });
    }
}
```

## Best Practices

### Position Adjustment

Example positions may not follow guidelines. Recalculate:

```javascript
function adjustPositions(boxes) {
    const baseX = 100;
    let currentY = 100;
    const spacing = 50;

    return boxes.map(box => ({
        ...box,
        position: [baseX, currentY += spacing]
    }));
}
```

### Varname Assignment

Follow `patch-guidelines` naming conventions:

| Object Type | Varname Pattern |
|-------------|-----------------|
| `cycle~` | `osc_purpose` |
| `*~` | `gain_purpose` |
| `buffer~` | `buf_name` |
| `dac~` | `dac_out` |
| `metro` | `metro_purpose` |

### Handle Missing Objects

Some objects in examples may not exist in your Max version:

```javascript
try {
    await mcp.add_max_object(...);
} catch (error) {
    console.log(`Skipping: ${objType} - ${error.message}`);
    // Use fallback or skip
}
```

### Subpatcher Handling

For nested patchers, recurse:

```javascript
async function createSubpatcher(box, parentId) {
    // Create the subpatcher object first
    await mcp.add_max_object({
        patch_id: parentId,
        object_type: "p",
        args: extractName(box.text),
        varname: box.varname
    });

    // Then populate its contents
    if (box.patcher) {
        await recreatePatch(box.patcher, box.varname);
    }
}
```

## Troubleshooting

| Issue | Solution |
|-------|----------|
| Object not found | Check spelling, use `get-reference.sh` |
| Connection fails | Verify inlet/outlet indices |
| Position overlap | Recalculate with `get_avoid_rect_position` |
| Missing varname | Assign from ID: `obj_1`, `obj_2`, etc. |
