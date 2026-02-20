# M4L Namespaces & Parameter Persistence

Max for Live introduces device-level scoping and parameter persistence behaviors that differ from standard Max. Understanding these differences is critical for building reliable M4L devices.

## Namespace Prefixes: `---` vs `#0`

### `---` (Three Dashes) — Device-Wide Scope

In Max for Live, `---` is replaced with a unique identifier **per device instance**. All objects within the same M4L device share the same `---` value.

```
// Device instance A:
send ---mydata    → becomes "042mydata"
receive ---mydata → becomes "042mydata" (same device, matches)

// Device instance B:
send ---mydata    → becomes "057mydata" (different device, different ID)
```

**Use case**: Communication between abstractions/bpatchers within the same M4L device.

### `#0` — Patcher-Instance Scope

`#0` is replaced with a unique integer **per patcher load**. Each abstraction or bpatcher instance gets its own `#0` value, even within the same device.

```
// bpatcher instance 1:
send #0_data    → becomes "1234_data"

// bpatcher instance 2 (same device):
send #0_data    → becomes "5678_data" (different patcher, different ID)
```

**Use case**: Isolating communication within individual bpatcher instances.

### Comparison

| Feature | `---` | `#0` |
|---|---|---|
| Scope | Entire M4L device | Single patcher instance |
| Shared across bpatchers | Yes | No |
| Unique per device copy | Yes | Yes (per patcher) |
| Cross-device isolation | Yes | Yes |
| Works in standard Max | No (M4L only) | Yes |

### Combined Usage

Both can be used simultaneously without conflict:

```
send ---#0_param    → device-unique + patcher-unique
```

This creates a fully qualified name that is unique both across devices and across bpatcher instances within a device.

## pattr Must NOT Use `---` Prefix

### The Problem

If a `pattr` object's name starts with `---`, **saved parameter values cannot be restored** on device reload.

```
pattr ---volume    ← BROKEN: values reset to default on reload
```

**Why**: The `---` prefix generates a different ID each time the device loads (e.g., `042volume` on first load, `057volume` on second load). The saved data is keyed to the old ID and cannot be found.

### The Solution

Simply omit the `---` prefix for `pattr` names:

```
pattr volume       ← CORRECT: values persist across sessions
```

Testing confirms that multiple instances of the same device on different tracks do not cause `pattr` naming conflicts — each device instance maintains its own parameter storage independently.

**Rule**: Never use `---` in `pattr` names. The `pattr` system handles device-level isolation internally.

## Unbound pattr Persistence (Float → blob)

### The Problem

Unbound `pattr` objects (not connected to any UI) inside a `bpatcher` with argument-based namespacing fail to persist their values when the parameter type is **Float**. Values reset to 0 on reload.

```
// Inside bpatcher with @args prefix
pattr #1_internal_state    ← stores intermediate data, no UI
// Parameter Type: Float   ← values LOST on reload
```

### The Solution

Change the parameter type from **Float** to **blob** in the Parameter Inspector:

```
pattr #1_internal_state
// Parameter Type: blob    ← values PERSIST correctly
```

The exact cause is undocumented, but the blob type reliably persists values in this configuration.

## selected_parameter Monitoring

### The Problem

Observing `live_set view selected_parameter` with `live.observer` produces unreliable output:

1. Selecting non-parameter objects outputs `id 0` (should be silent)
2. Selecting M4L device parameters outputs multiple IDs in rapid succession

### Filtering Pattern

```
live.observer @property selected_parameter
  ↓
route id              → strip "id" prefix, filter id 0
  ↓
change                → suppress duplicate values
  ↓
thresh 0              → collect rapid outputs into a single list
  ↓
zl.ecils 1            → extract LAST element (the correct ID)
  ↓
prepend id            → restore "id N" format
  ↓
(output: reliable parameter ID)
```

**Key insight**: When multiple IDs fire in rapid succession, the **last** one is always the correct value.

## Quick Reference

| Scenario | Solution |
|---|---|
| Share data across bpatchers in same device | Use `---` prefix |
| Isolate data per bpatcher instance | Use `#0` prefix |
| Both device + instance isolation | Use `---#0_` prefix |
| Store pattr values persistently | Do NOT use `---` in pattr name |
| Unbound pattr in bpatcher loses values | Change type to blob |
| selected_parameter gives wrong IDs | Filter with thresh + zl.ecils |

## Sources

- https://leico.github.io/TechnicalNote/Live/m4l-bpatcher-namespace
- https://leico.github.io/TechnicalNote/Live/pattr-name
- https://leico.github.io/TechnicalNote/Live/m4l-bpatcher-pattr-nobind
- https://leico.github.io/TechnicalNote/Live/selected-parameter
