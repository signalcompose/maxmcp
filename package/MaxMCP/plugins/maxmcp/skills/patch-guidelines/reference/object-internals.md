# Max Object Internals

Internal specifications of Max objects that differ from their external appearance. Understanding these helps correctly create and identify objects via MCP tools.

## number / flonum

In Max's internal representation, **flonum is not a separate class**. Both integer and floating-point number boxes share the same `maxclass: "number"`.

### Identification via `format` Attribute

| Display | maxclass | format | Description |
|---------|----------|--------|-------------|
| `[0]` (integer) | `number` | `0` | Integer number box |
| `[0.]` (float) | `number` | `6` | Floating-point number box (flonum) |

### Creating flonum via MCP

To create a flonum (floating-point number box):

1. Create a `number` object with `add_max_object`
2. Set `format` attribute to `6` with `set_object_attribute`

```
add_max_object: obj_type = "number"
set_object_attribute: attribute = "format", value = 6
```

### Identifying flonum in Existing Patches

`get_objects_in_patch` reports both integer and float number boxes as `maxclass: "number"`. To distinguish them:

```
get_object_attribute: attribute = "format"
  → 0 = integer number box
  → 6 = floating-point number box (flonum)
```

### Implications for Patch Analysis

When analyzing a patch, do not assume `maxclass: "number"` means integer-only. Always check the `format` attribute to determine the actual number type.

## get_object_value Compatibility

`get_object_value` uses Max SDK's `object_getvalueof()` internally. Not all objects implement this interface.

### Supported Objects

| Object | Return Type | Notes |
|--------|-------------|-------|
| `number` (integer) | number | Current integer value |
| `number` (flonum, format=6) | number | Current float value |
| `toggle` | number | 0 (off) or 1 (on) |
| `led` | number | 0 (off) or 1 (on) |
| `textbutton` | number | 0 (off) or 1 (on) |
| `tab` | number | Selected tab index (integer) |
| `slider` | number | Current slider position (integer) |
| `dial` | number | Current dial value |
| `gain~` | number | Current gain value (integer) |
| `live.gain~` | number | Current gain in dB (float) |
| `multislider` | array | Array of slider values (float) |
| `live.text` | number | 0.0 (off) or 1.0 (on), float |
| `live.tab` | number | Selected tab index (float) |
| `live.step` | array | Sequencer data (steps, notes, velocity, etc.) |
| `live.grid` | array | Grid state (size, cell values, etc.) |
| `textedit` | string | Text content |

### Unsupported Objects

| Object | Behavior |
|--------|----------|
| `message` | Returns object type info, not text content |
| `comment` | Returns object type info, not text content |

For message/comment text content, use `get_objects_in_patch` (which includes box text) or `replace_object_text` for modification. The `binbuf_totext()` API could support direct text retrieval in a future tool.
