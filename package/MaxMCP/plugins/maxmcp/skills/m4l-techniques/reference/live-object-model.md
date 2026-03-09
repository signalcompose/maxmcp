# Live Object Model (LOM)

The Live Object Model provides programmatic access to Ableton Live's internal structure from Max for Live devices. The workflow follows a consistent pipeline: **path → id → control/observe**.

## Overview

```
live.path         → resolves a path string to an object ID
     ↓ id
live.object       → read/write properties, call functions
live.observer     → monitor property changes in real-time
```

## Step 1: Path Navigation with live.path

### Path Syntax

Live objects are accessed using hierarchical paths based on the LOM structure:

```
live_set                              → the Live set itself
live_set tracks 0                     → first track
live_set tracks 2 clip_slots 0 clip   → first clip in third track
live_set tracks 0 mixer_device volume → first track's volume
live_set return_tracks 0              → first return track
live_set master_track                 → master track
```

**Indexing**: Zero-based (0 = first track, 1 = second, etc.)

### Using live.path

Send a `path` message to `live.path` to resolve it:

```
message "path live_set tracks 0 mixer_device volume"
  ↓
live.path
  ↓ (left outlet: id N)
```

The left outlet outputs the resolved ID (e.g., `id 3`). If the path is invalid, it outputs `id 0`.

### Dynamic Path Construction

Build paths dynamically using `pak` or `sprintf`:

```
number (track index)
  ↓
pak path live_set tracks 0
  ↓
live.path
```

This allows selecting different tracks/clips/parameters based on user input.

## Step 2: ID Lifecycle

### Key Characteristics

| Property | Behavior |
|---|---|
| Format | `id N` (integer, e.g., `id 3`) |
| Assignment | On first access via `live.path` |
| Persistence | Within a session only — **IDs change on reload** |
| Invalid | `id 0` means the object doesn't exist |
| Stability | Remains valid even if the object moves during a session |

### Important Caveat

IDs are assigned in the order `live.path` accesses them. They are **not persistent** across sessions. Never store IDs for later use — always re-resolve paths when a device loads.

**Pattern**: Use `live.thisdevice` bang → re-resolve all paths on device load.

## Step 3: Control with live.object

### Setup

Send the ID to `live.object`'s **right inlet** first, then send commands to the **left inlet**:

```
live.path
  ↓ (id)
live.object (right inlet)    ← set target
  ↑ (left inlet)
messages: get, set, call     ← send commands
```

### Commands

#### get — Read Properties

```
get value     → outputs current value
get min       → outputs minimum value
get max       → outputs maximum value
```

Output appears on `live.object`'s left outlet.

#### set value — Write Properties

```
set value 0.85    → set the parameter to 0.85
```

**For UI synchronization**, connect a `live.slider` or `live.dial`:

```
live.slider (0.0 - 1.0)
  ↓
prepend set value
  ↓
live.object
```

#### call — Invoke Functions

```
call fire          → fire a clip
call stop          → stop a clip
call select_device → select a device
```

### getinfo — Discover Available Properties

```
getinfo
  ↓
live.object → outputs all properties, children, and callable functions
```

Use this to explore what's available for a given LOM object.

## Step 4: Monitor with live.observer

### Purpose

`live.observer` watches a property and outputs its value whenever it changes in Live. This is essential for **bidirectional synchronization** — reflecting Live-side changes back into your Max device.

### Setup

```
live.path
  ↓ (id)
live.observer @property value (right inlet)
  ↓ (left outlet: updated value whenever it changes)
```

### Why Not Use metro?

Polling with `metro` introduces latency and wastes CPU. `live.observer` is event-driven — it fires only when the value actually changes.

### Limitations

- Not all properties are observable — check with `getinfo`
- When observing **child objects** (e.g., `tracks`), the output updates when children are added/removed (e.g., adding a new track)
- Documentation may not always list which properties are observable — test empirically

### Bidirectional Sync Pattern

```
live.path → id
             ↓
live.object ←──── live.slider ←──── live.observer
  (set value)      (display)         (monitor changes)
```

1. User moves slider → `live.object` sets the Live parameter
2. Live parameter changes externally → `live.observer` detects → updates slider

**Caveat**: Avoid feedback loops. Use `change` object or a gate to prevent the slider update from re-triggering `live.object`.

## Common Path Reference

| Path | Target |
|---|---|
| `live_set` | The Live set |
| `live_set tracks N` | Track N (0-indexed) |
| `live_set return_tracks N` | Return track N |
| `live_set master_track` | Master track |
| `live_set tracks N mixer_device` | Track N mixer |
| `live_set tracks N mixer_device volume` | Track N volume |
| `live_set tracks N mixer_device panning` | Track N pan |
| `live_set tracks N mixer_device sends N` | Track N send N |
| `live_set tracks N clip_slots N clip` | Clip in track N, slot N |
| `live_set tracks N devices N` | Device N on track N |
| `live_set tracks N devices N parameters N` | Parameter N of device N |
| `live_set scenes N` | Scene N |
| `live_set view selected_parameter` | Currently selected parameter |

## Sources

- https://leico.github.io/TechnicalNote/Live/m4l-path
- https://leico.github.io/TechnicalNote/Live/m4l-id
- https://leico.github.io/TechnicalNote/Live/m4l-object
- https://leico.github.io/TechnicalNote/Live/m4l-observer
