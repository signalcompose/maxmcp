# Patch Recreation Examples

## Example 1: Simple Oscillator Patch

**Goal**: Recreate a basic oscillator patch from Max examples.

### Step 1: Find Example

```bash
./scripts/list-examples.sh synths
```

Found: `synths/simple-oscillator.maxpat`

### Step 2: Read and Analyze

```bash
cat "/Applications/Max.app/Contents/Resources/Examples/synths/simple-oscillator.maxpat"
```

Extracted structure:
```
Objects:
  - flonum (frequency control)
  - cycle~ 440
  - *~ 0.3 (gain)
  - ezdac~

Connections:
  - flonum → cycle~ (inlet 0)
  - cycle~ → *~ (inlet 0)
  - *~ → ezdac~ (inlet 0)
  - *~ → ezdac~ (inlet 1)
```

### Step 3: Apply Guidelines

Following `patch-guidelines`:
- Layout: Signal flow top-to-bottom
- Spacing: 50px vertical
- Varnames: `ctrl_freq`, `osc_main`, `gain_output`, `dac_out`

### Step 4: MCP Tool Calls

```javascript
// 1. Create frequency control
add_max_object({
    patch_id: "demo_patch",
    object_type: "flonum",
    varname: "ctrl_freq",
    position: [100, 50]
})

// 2. Create oscillator
add_max_object({
    patch_id: "demo_patch",
    object_type: "cycle~",
    args: "440",
    varname: "osc_main",
    position: [100, 100]
})

// 3. Create gain
add_max_object({
    patch_id: "demo_patch",
    object_type: "*~",
    args: "0.3",
    varname: "gain_output",
    position: [100, 150]
})

// 4. Create DAC
add_max_object({
    patch_id: "demo_patch",
    object_type: "ezdac~",
    varname: "dac_out",
    position: [100, 200]
})

// 5. Connect freq → osc
connect_max_objects({
    patch_id: "demo_patch",
    source_varname: "ctrl_freq",
    source_outlet: 0,
    dest_varname: "osc_main",
    dest_inlet: 0
})

// 6. Connect osc → gain
connect_max_objects({
    patch_id: "demo_patch",
    source_varname: "osc_main",
    source_outlet: 0,
    dest_varname: "gain_output",
    dest_inlet: 0
})

// 7. Connect gain → dac (left)
connect_max_objects({
    patch_id: "demo_patch",
    source_varname: "gain_output",
    source_outlet: 0,
    dest_varname: "dac_out",
    dest_inlet: 0
})

// 8. Connect gain → dac (right)
connect_max_objects({
    patch_id: "demo_patch",
    source_varname: "gain_output",
    source_outlet: 0,
    dest_varname: "dac_out",
    dest_inlet: 1
})
```

## Example 2: FM Synthesis Patch

**Goal**: Create an FM synthesizer based on example.

### Analysis

FM synthesis requires:
- Carrier oscillator
- Modulator oscillator
- Modulation depth control
- Frequency ratio control

### Structure

```
[flonum: carrier_freq]  [flonum: mod_ratio]  [flonum: mod_depth]
         │                     │                     │
         │              [*: ratio_mult]────────────┐│
         │                     │                   ││
         └──────┬──────────────┘                   ││
                │                                  ││
         [cycle~: mod_osc]─────────────[*~: mod_scale]
                │
         [+~: freq_mod]────────[cycle~: carrier]
                                      │
                               [*~: output_gain]
                                      │
                               [ezdac~: dac]
```

### MCP Implementation

```javascript
// Parameter controls
add_max_object({ object_type: "flonum", varname: "ctrl_carrier", position: [100, 50] })
add_max_object({ object_type: "flonum", varname: "ctrl_ratio", position: [200, 50] })
add_max_object({ object_type: "flonum", varname: "ctrl_depth", position: [300, 50] })

// Ratio multiplier
add_max_object({ object_type: "*", varname: "ratio_mult", position: [150, 100] })

// Modulator
add_max_object({ object_type: "cycle~", varname: "osc_mod", position: [150, 150] })
add_max_object({ object_type: "*~", varname: "mod_scale", position: [250, 150] })

// Carrier
add_max_object({ object_type: "+~", varname: "freq_mod", position: [150, 200] })
add_max_object({ object_type: "cycle~", varname: "osc_carrier", position: [150, 250] })

// Output
add_max_object({ object_type: "*~", args: "0.3", varname: "gain_out", position: [150, 300] })
add_max_object({ object_type: "ezdac~", varname: "dac_out", position: [150, 350] })

// Connections (abbreviated)
// ctrl_carrier → ratio_mult (inlet 0)
// ctrl_ratio → ratio_mult (inlet 1)
// ratio_mult → osc_mod (inlet 0)
// ctrl_depth → mod_scale (inlet 1)
// osc_mod → mod_scale (inlet 0)
// ctrl_carrier → freq_mod (inlet 0)
// mod_scale → freq_mod (inlet 1)
// freq_mod → osc_carrier (inlet 0)
// osc_carrier → gain_out (inlet 0)
// gain_out → dac_out (both channels)
```

## Example 3: Using Snippets

**Goal**: Use a snippet as starting point.

### Find Snippet

```bash
./scripts/get-snippet.sh msp filter
```

Found: `lowpass-filter.maxsnip`

### Read Snippet Content

Snippets are also JSON format:

```json
{
  "boxes": [
    {"box": {"maxclass": "newobj", "text": "svf~", ...}},
    {"box": {"maxclass": "flonum", ...}},
    ...
  ],
  "lines": [...]
}
```

### Incorporate into Patch

```javascript
// Add snippet objects to existing patch
// Follow the same recreation workflow
```

## Example 4: Complex Patch with Subpatchers

### Identify Subpatchers

When reading example JSON, look for nested `patcher` objects:

```json
{
  "box": {
    "maxclass": "newobj",
    "text": "p voice",
    "patcher": {
      "boxes": [...],
      "lines": [...]
    }
  }
}
```

### Recreation Strategy

1. Create main patch objects
2. Create subpatcher object: `add_max_object({ object_type: "p", args: "voice", ... })`
3. Access subpatcher contents
4. Recursively create objects inside

```javascript
// Create subpatcher
add_max_object({
    object_type: "p",
    args: "voice",
    varname: "sub_voice",
    position: [100, 150]
})

// Note: Contents must be created within the subpatcher context
// This may require additional MCP tool support
```

## Tips for Recreation

### 1. Position Calculation

Use grid alignment:

```javascript
const GRID = 15;
const alignToGrid = (pos) => Math.round(pos / GRID) * GRID;
```

### 2. Varname Generation

From object ID:

```javascript
const generateVarname = (id, objType) => {
    const prefix = objType.replace('~', '').toLowerCase();
    const num = id.replace('obj-', '');
    return `${prefix}_${num}`;
};
```

### 3. Handle Special Characters

Object names with `~` need proper escaping in some contexts:

```javascript
const sanitizeType = (type) => type.replace('~', '_tilde');
```

### 4. Verify Connections

After recreation, use `get_objects_in_patch` to verify:

```javascript
get_objects_in_patch({ patch_id: "demo_patch" })
// Check all objects exist and have correct properties
```

## Common Patterns

| Pattern | Objects | Use Case |
|---------|---------|----------|
| Signal gain | `*~ value` | Volume control |
| Stereo split | `*~ → dac~ (0,1)` | Output to both channels |
| LFO modulation | `cycle~ 0.5` | Slow modulation |
| Envelope | `adsr~` or `function` | Amplitude shaping |
| Mixing | `+~` | Combine signals |
