# M4L Tips & Reference Values

Practical tips and reference data for Max for Live device development.

## Logarithmic Controller Mapping

### The Problem

Linear controller mapping (0.0-1.0) doesn't match human perception for volume and frequency controls. For track volume, 0 dB corresponds to approximately 0.85 in Live's internal value — but a linear mapping places 0 dB at 85% of the knob travel, leaving very little range for boost.

### The Solution: Power Curve

Apply a power function to the controller input:

```
output = input ^ exponent
```

To place 0 dB at the controller's midpoint (0.5), calculate the exponent:

```
exponent = log(target_value) / log(0.5)
```

| Control | 0 dB Value | Exponent |
|---|---|---|
| Track Volume | 0.85 | ~0.234 |
| `live.gain~` | 0.921 | ~0.119 |

### Implementation in Max

```
live.slider (0.0 - 1.0)
  ↓
pow 0.234            → power curve for track volume
  ↓
prepend set value
  ↓
live.object          → track volume parameter
```

### Limitation

With MIDI controllers limited to 128 steps, the power curve can result in insufficient resolution at lower volume levels. Consider using 14-bit MIDI or higher-resolution controllers for critical volume faders.

## Live Level Reference (dBFS)

### Formula

Live uses amplitude-based dB calculation:

```
dB = 20 × log₁₀(value)
```

### 0 dB Values by Context

| Parameter | Internal Value at 0 dB | Range |
|---|---|---|
| Track Volume | ~0.85 | Extends to +6 dB (value > 0.85) |
| Send Level | 1.0 | 0.0 - 1.0 |
| Audio Output | 1.0 | Standard amplitude |
| `live.gain~` raw | ~0.921 | Non-standard mapping |

### Why Track Volume Uses 0.85

Track volume extends above 0 dB (up to +6 dB), so 0 dB falls at 0.85 rather than 1.0. The send level uses the full 0-1 range with 0 dB at 1.0 because sends don't provide boost above unity.

### Practical Advice

- Don't rely on precise dBFS-to-dBu conversion in Live
- Output at 0 dB from Live and adjust at the audio interface
- When mapping controllers, use the power curve approach above to align 0 dB with a sensible knob position

## Push2 Automapping Index

### The Problem

By default, M4L device parameters appear in an arbitrary order on Push2's display, making the device difficult to use.

### The Solution

Set the **Automapping Index** value in the Parameter Inspector for each parameter:

1. Open the Inspector for each parameter object
2. Find **Automapping Index** in the Parameter section
3. Assign sequential numbers (1, 2, 3, ...) in your desired display order

### Rules

| Rule | Behavior |
|---|---|
| Index = 0 (default) | Automatic ordering (unpredictable) |
| Index > 0 on ANY parameter | Manual ordering activates for ALL parameters |
| Skipped numbers | Create blank spaces on Push2 display |
| After saving | Cycle tracks (select another, then return) to see changes |

### Example Layout

For a 4-band EQ device:

```
Index 1-2:  Band 1 Freq, Gain
Index 3-4:  Band 2 Freq, Gain
Index 5-6:  Band 3 Freq, Gain
Index 7-8:  Band 4 Freq, Gain
Index 10:   Crossover 1-2     (index 9 = blank space for visual grouping)
Index 13:   Crossover 2-3
Index 15:   Crossover 3-4
```

**Tip**: Use gaps strategically to group related parameters visually on the Push2 display.

## Two-Stage Initialization (live.thisdevice + delay)

### The Problem

Some M4L devices need to resolve LOM paths and set up observers at startup. However, Live's internal objects may not be fully ready when `live.thisdevice` fires. Attempting LOM path resolution too early results in failed connections or missing IDs.

### The Pattern

```
live.thisdevice
  ↓ (bang on device load)
t b b
│   │
│   └→ s ---lb1              ← immediate init broadcast
└───→ delay 100
        ↓
        s ---lb2             ← delayed init broadcast
```

### How It Works

1. `live.thisdevice` fires a bang when the device loads
2. `t b b` splits into two initialization phases:
   - **Phase 1 (immediate)**: `s ---lb1` broadcasts to all `r ---lb1` receivers. Used for operations that don't depend on Live's state (UI setup, view path resolution)
   - **Phase 2 (delayed)**: `delay 100` waits 100ms, then `s ---lb2` broadcasts. Used for LOM path resolution, observer setup, and anything requiring Live to be fully ready

### Why --- Namespace

The `---` prefix is an M4L device-unique namespace. Each device instance gets its own `---lb1` and `---lb2` channels, preventing cross-device interference. See [Namespace & Parameters Reference](namespace-parameters.md) for details.

### Receiver Side Examples

```
r ---lb1                     r ---lb2
  ↓                            ↓
pack path live_set view      pack path live_set
  ↓                            ↓
live.path                    live.path
  ↓                            ↓
(resolve view for UI)        (resolve live_set for observer setup)
```

### When to Use

- Devices that resolve LOM paths on startup
- Devices with `live.observer` that need stable IDs
- Devices that read Live set state (track count, scene count) on load
- Any device that accesses LOM properties during initialization

## Sources

- https://leico.github.io/TechnicalNote/Live/knob-mapping
- https://leico.github.io/TechnicalNote/Live/dbfs
- https://leico.github.io/TechnicalNote/Live/automapping-index
