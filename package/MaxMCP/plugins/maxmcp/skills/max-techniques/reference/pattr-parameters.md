# pattr & Parameter Management

State persistence, parameter control, and initialization patterns using `pattr`, `pattrstorage`, and the Parameter Inspector.

## pattr + pattrstorage Basics

### pattr

`pattr` binds to a UI object or stores a value, exposing it to `pattrstorage` for preset management.

**Binding modes**:
- **By scripting name**: Set `@bindto <scripting_name>` to link to a named UI object
- **Direct connection**: Connect `pattr` outlet to a UI object's inlet — `pattr` mirrors the object's value
- **Standalone**: `pattr <name>` stores a value without binding to UI

### pattrstorage

`pattrstorage` collects all `pattr` objects in the patcher hierarchy and manages presets.

```
pattrstorage mypresets
  │
  ├── pattr pitch        → stored as "mypresets::pitch"
  ├── pattr volume       → stored as "mypresets::volume"
  └── pattr filter_freq  → stored as "mypresets::filter_freq"
```

**Key messages**:
- `store N` — store current state to preset slot N
- `recall N` — recall preset slot N
- `write` / `read` — save/load presets to/from JSON file

## Parameter Inspector

The Parameter Inspector (accessible via the Inspector panel) enables advanced parameter features for both Max and Max for Live.

### Parameter Mode

Enable **Parameter Mode Enable** in the Inspector to:
- Persist the parameter value when saving the patch/Live set
- Make the parameter available for automation in Live
- Enable initial value settings

**Without Parameter Mode**: Values reset to default (typically 0) when the patch reloads.

### Initial Value

When Parameter Mode is enabled:
1. Check **Initial Enable** in the Parameter Inspector
2. Set the **Initial** value (float)

**When initialization occurs**: Only when a device is first placed on a track, NOT when reopening a saved set. This prevents overwriting user-modified saved values.

**Priority**: Saved values always take precedence over initial values on reload.

## Initialization Timing

Understanding the execution order is critical for reliable parameter setup.

### Execution Order

```
1. Parameter Inspector initial values are applied
2. Saved parameter values are loaded (overwriting initials)
3. live.thisdevice bang fires (can overwrite everything)
```

### Implications

| Scenario | Behavior |
|---|---|
| First device placement | Initial value applies, then `live.thisdevice` fires |
| Reopening saved set | Saved value loads, then `live.thisdevice` fires |
| `live.thisdevice` sets a value | Overrides both initial and saved values |

**Recommendation**:
- Use **Parameter Inspector** for default values that respect user saves
- Use **`live.thisdevice`** only when you need a guaranteed startup value regardless of saved state
- Avoid using both on the same parameter — `live.thisdevice` always wins

## bpatcher + pattr Naming Collisions

### The Problem

When loading the same subpatcher in multiple `bpatcher` instances, `pattr` objects with identical scripting names collide. All instances write to the same parameter slot, overwriting each other.

**Specifically**: This occurs when:
- The bpatcher loads an **external file** (not embedded)
- `pattr`'s **"Link to Scripting Name"** option is enabled

Embedded bpatchers don't have this problem because Max automatically appends unique numbers to duplicate scripting names.

### The Solution: Argument-Based Naming

Pass a unique prefix as a bpatcher argument and include it in the `pattr` name:

```
// Parent patch
bpatcher @args inst1    → first instance
bpatcher @args inst2    → second instance

// Inside bpatcher
pattr #1_volume         → becomes "inst1_volume" / "inst2_volume"
pattr #1_pitch          → becomes "inst1_pitch" / "inst2_pitch"
```

This ensures each instance has unique `pattr` names while keeping "Link to Scripting Name" functional.

## pattr Range Limitations

### Default Range: 0-127

By default, `pattr` with Parameter Mode enabled restricts values to **0-127** (integer) range. This causes:
- Negative values (e.g., -100) silently clamp to 0
- Values above 127 (e.g., 1800) silently clamp to 127

### Fixing the Range

In the Parameter Inspector, expand the **Range/Enum** section:

1. Set **Minimum** and **Maximum** to your desired range
2. For audio parameters: `-32768` to `32767` covers most use cases
3. Real-world examples:
   - Gain: -70 to 6 (dB)
   - Frequency: 20 to 20000 (Hz)
   - Pan: -1.0 to 1.0

### Alternative: Float Type with Int Display

Select **Float** as the parameter type and set **Unit Style** to **Int**. This allows storing values outside the standard integer boundaries while displaying them as integers.

## Remote UI with pattr @invisible @bindto

### The Problem

In hierarchical patches (top-level → subpatchers → poly~ voices), parameters defined inside subpatchers are not directly visible or controllable from the top level. You need a way to create a centralized control panel in the parent patch that mirrors and controls parameters deep in the hierarchy.

### The Pattern

```
// Top-level patch — "Remote UI" section
pattr @invisible 1 @bindto sub_synth::comp_threshold   ← read: monitors value
  ↓
prepend set
  ↓
number                                                   ← displays current value
  ↓
pattr @invisible 1 @bindto sub_synth::comp_threshold   ← write: sends changes back
```

### How It Works

Each remote parameter uses **two pattr objects**:

1. **Read pattr** (top → display): Monitors the subpatcher's pattr value. When the value changes in the subpatcher, this pattr outputs the new value → `prepend set` → `number` (displays without triggering output)
2. **Write pattr** (display → bottom): Connected from the `number` output. When the user changes the number box, the new value flows to this pattr, which writes it back to the subpatcher's parameter

**Why `prepend set`?** Without it, the read pattr's output would flow to the number box and immediately trigger its output, creating a feedback loop. `prepend set` sets the value silently.

**Why `@invisible 1`?** These pattr objects are infrastructure — they should not appear in the pattrstorage client window or clutter the parameter list. They exist solely to bridge the hierarchy.

### Addressing Subpatcher Parameters

The `@bindto` path uses `::` to traverse the patcher hierarchy:

```
@bindto sub_synth::gain                    ← direct child subpatcher
@bindto sub_synth::comp_threshold          ← pattr named "comp_threshold" inside "sub_synth"
```

The subpatcher must have a `varname` (scripting name) for this addressing to work.

### When to Use

- Standalone applications with deep subpatcher hierarchies
- Creating a centralized control panel for parameters spread across subpatchers
- Any patch where the top level needs to monitor and control subpatcher state

## Centralized State with pattrstorage @greedy

### The Pattern

```
pattrstorage myPresets @greedy 1
```

### How @greedy Works

By default, `pattrstorage` only registers pattr objects that are directly connected or explicitly bound. With `@greedy 1`, it automatically discovers and registers **all pattr objects in the entire patcher hierarchy**, including:

- `pattr @invisible 1 @bindto` objects (Remote UI bridges)
- pattr objects inside subpatchers
- pattr objects added after pattrstorage was created

### Combined with Remote UI

When Remote UI pattr bridges and `@greedy 1` are used together, a single `pattrstorage` at the top level can save and recall the complete state of all parameters across the entire hierarchy:

```
pattrstorage myApp @greedy 1
  ├── captures: Remote UI pattr → sub_synth::gain
  ├── captures: Remote UI pattr → sub_synth::comp_threshold
  ├── captures: Remote UI pattr → sub_fx::delay_time
  └── ... (all parameters across all subpatchers)
```

**Result**: `store 1` saves everything, `recall 1` restores everything — the entire application state in one preset operation.

## Quick Reference

| Task | Solution |
|---|---|
| Persist parameter values | Enable Parameter Mode in Inspector |
| Set default on first load | Enable Initial Enable + set Initial value |
| Guaranteed startup value | Use `live.thisdevice` bang → set value |
| Multiple bpatcher instances | Use `#1_` argument prefix for `pattr` names |
| Values beyond 0-127 | Adjust Range in Parameter Inspector |
| Preset management | Use `pattrstorage` with named `pattr` objects |
| Remote parameter control | Use `pattr @invisible 1 @bindto sub::param` (read + write pair) |
| Capture all parameters | Use `pattrstorage @greedy 1` |

## Sources

- https://leico.github.io/TechnicalNote/Live/store-parameter
- https://leico.github.io/TechnicalNote/Live/initialize
- https://leico.github.io/TechnicalNote/Live/initial-timing
- https://leico.github.io/TechnicalNote/Live/m4l-bpatcher-pattr
- https://leico.github.io/TechnicalNote/Live/pattr-range
