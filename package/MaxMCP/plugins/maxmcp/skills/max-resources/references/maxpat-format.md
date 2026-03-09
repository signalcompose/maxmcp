# Max Patch Format (.maxpat JSON)

Max patches are JSON files with a standardized structure for objects, connections, and metadata.

## Basic Structure

```json
{
    "patcher": {
        "fileversion": 1,
        "appversion": {
            "major": 9,
            "minor": 0,
            "revision": 5
        },
        "classnamespace": "box",
        "rect": [100, 100, 800, 600],
        "gridsize": [15.0, 15.0],
        "boxes": [...],
        "lines": [...],
        "dependency_cache": [...],
        "autosave": 0
    }
}
```

## Boxes Array

Each object in the patch is a "box":

```json
{
    "boxes": [
        {
            "box": {
                "id": "obj-1",
                "maxclass": "newobj",
                "numinlets": 2,
                "numoutlets": 1,
                "outlettype": ["signal"],
                "patching_rect": [100.0, 100.0, 63.0, 22.0],
                "text": "cycle~ 440"
            }
        },
        {
            "box": {
                "id": "obj-2",
                "maxclass": "ezdac~",
                "numinlets": 2,
                "numoutlets": 0,
                "patching_rect": [100.0, 200.0, 45.0, 45.0]
            }
        },
        {
            "box": {
                "id": "obj-3",
                "maxclass": "flonum",
                "numinlets": 1,
                "numoutlets": 2,
                "outlettype": ["", "bang"],
                "parameter_enable": 0,
                "patching_rect": [100.0, 50.0, 50.0, 22.0],
                "varname": "freq_control"
            }
        }
    ]
}
```

### Box Properties

| Property | Description |
|----------|-------------|
| `id` | Unique identifier (obj-1, obj-2, ...) |
| `maxclass` | Object type (newobj, flonum, button, etc.) |
| `text` | Object text (for newobj: "cycle~ 440") |
| `patching_rect` | Position [x, y, width, height] |
| `varname` | Scripting name (for MCP access) |
| `numinlets` | Number of inlets |
| `numoutlets` | Number of outlets |
| `outlettype` | Output types ("", "signal", "bang", etc.) |

### Common maxclass Values

| maxclass | Description |
|----------|-------------|
| `newobj` | Text-based object |
| `message` | Message box |
| `comment` | Comment |
| `flonum` | Float number box |
| `number` | Int number box |
| `button` | Button |
| `toggle` | Toggle |
| `slider` | Slider |
| `dial` | Dial |
| `ezdac~` | Easy DAC |
| `ezadc~` | Easy ADC |
| `inlet` | Patcher inlet |
| `outlet` | Patcher outlet |

## Lines Array

Connections between objects:

```json
{
    "lines": [
        {
            "patchline": {
                "source": ["obj-3", 0],
                "destination": ["obj-1", 0],
                "hidden": 0,
                "midpoints": []
            }
        },
        {
            "patchline": {
                "source": ["obj-1", 0],
                "destination": ["obj-2", 0],
                "hidden": 0
            }
        },
        {
            "patchline": {
                "source": ["obj-1", 0],
                "destination": ["obj-2", 1],
                "hidden": 0
            }
        }
    ]
}
```

### Patchline Properties

| Property | Description |
|----------|-------------|
| `source` | [object_id, outlet_index] |
| `destination` | [object_id, inlet_index] |
| `hidden` | 0 = visible, 1 = hidden |
| `midpoints` | Bend points [x1, y1, x2, y2, ...] |
| `color` | Line color [r, g, b, a] |

## Object-Specific Properties

### newobj with Attributes

```json
{
    "box": {
        "id": "obj-1",
        "maxclass": "newobj",
        "text": "cycle~ @interp 2",
        "patching_rect": [100.0, 100.0, 100.0, 22.0]
    }
}
```

### UI Objects

```json
{
    "box": {
        "id": "obj-4",
        "maxclass": "slider",
        "patching_rect": [50.0, 50.0, 20.0, 140.0],
        "size": 128.0,
        "min": 0.0,
        "mult": 1.0,
        "floatoutput": 1,
        "varname": "my_slider"
    }
}
```

### Subpatchers

```json
{
    "box": {
        "id": "obj-5",
        "maxclass": "newobj",
        "text": "p my_subpatch",
        "patching_rect": [100.0, 150.0, 80.0, 22.0],
        "patcher": {
            "fileversion": 1,
            "boxes": [...],
            "lines": [...]
        }
    }
}
```

## Extracting for MCP Recreation

### Step 1: Parse Objects

```javascript
// Extract objects from patch JSON
const objects = patch.patcher.boxes.map(b => ({
    id: b.box.id,
    type: b.box.maxclass === 'newobj'
        ? b.box.text.split(' ')[0]  // First word is object type
        : b.box.maxclass,
    args: b.box.maxclass === 'newobj'
        ? b.box.text.split(' ').slice(1).join(' ')
        : '',
    position: b.box.patching_rect.slice(0, 2),
    varname: b.box.varname
}));
```

### Step 2: Parse Connections

```javascript
// Extract connections
const connections = patch.patcher.lines.map(l => ({
    source_id: l.patchline.source[0],
    source_outlet: l.patchline.source[1],
    dest_id: l.patchline.destination[0],
    dest_inlet: l.patchline.destination[1]
}));
```

### Step 3: Map IDs to Varnames

For MCP tools, you need varnames:

```javascript
// Create id -> varname mapping
const idToVarname = {};
objects.forEach((obj, i) => {
    idToVarname[obj.id] = obj.varname || `obj_${i}`;
});

// Convert connections
const mcpConnections = connections.map(c => ({
    source_varname: idToVarname[c.source_id],
    source_outlet: c.source_outlet,
    dest_varname: idToVarname[c.dest_id],
    dest_inlet: c.dest_inlet
}));
```

## Validation

### Check Required Fields

```bash
# Verify patch structure
jq '.patcher.boxes | length' patch.maxpat
jq '.patcher.lines | length' patch.maxpat

# List all object types
jq '.patcher.boxes[].box.maxclass' patch.maxpat | sort | uniq -c
```

### Common Issues

1. **Missing varnames**: Not all objects have varnames; assign them
2. **Object index vs ID**: JSON uses string IDs like "obj-1"
3. **Relative positions**: Positions are absolute, may need adjustment
4. **Subpatchers**: Nested patchers need recursive processing
