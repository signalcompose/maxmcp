# poly~ & bpatcher Techniques

Modular patch architecture using `poly~` for multi-instance processing and `bpatcher` for reusable components.

## poly~ Fundamentals

### Overview

`poly~` loads an external subpatcher and runs multiple instances simultaneously. It is the primary mechanism for:
- Polyphonic voice management (synthesizers, samplers)
- Parallel signal processing (multiband, multichannel)
- CPU load distribution via multithreading

### Arguments

```
poly~ <filename> <instance_count> [@parallel 1] [@threadcount N] [@args ...]
```

| Argument | Description |
|---|---|
| `filename` | Subpatcher file to load (required) |
| `instance_count` | Number of instances, 1-1023 |
| `@parallel 1` | Enable multithreaded audio processing |
| `@threadcount N` | Number of processing threads |
| `@args v1 v2 ...` | Up to 10 arguments passed to subpatcher (accessed as `#1`, `#2`, ...) |

### Inlets & Outlets

Inside the subpatcher:
- **`in N`** / **`out N`**: Message inlets/outlets (N = index, starting from 1)
- **`in~ N`** / **`out~ N`**: Audio signal inlets/outlets

The parent `poly~` object's outlet count equals the maximum of message and audio outlet counts.

### Voice Targeting with `target`

```
target N    → Send subsequent messages to instance N only
target 0    → Send to ALL instances (broadcast)
target -1   → Disable message routing to all instances
```

**Usage pattern**:
```
target 1, note 60 127    → instance 1 receives "note 60 127"
target 2, note 64 100    → instance 2 receives "note 64 100"
target 0, volume 80      → all instances receive "volume 80"
```

### Internal Objects

#### thispoly~

Inside a `poly~` subpatcher, `thispoly~` provides:
- **Outlet 0**: Instance number (integer, on bang)
- **Inlet 0**: `mute 1`/`mute 0` to enable/disable audio processing for this voice

**Common pattern** — mute on envelope release:
```
adsr~ → thispoly~ (outlet 1 of adsr~ indicates "voice active" state)
```

#### #0 (Unique Instance ID)

`#0` is replaced with a unique integer per patcher instance. Use it to create instance-local `send`/`receive` names:

```
send #0_pitch      → each instance gets a unique send name
receive #0_pitch   → only receives from the same instance
```

**Caveat**: `#0` is per-patcher, not per-poly~-voice. If you need per-voice naming, use `thispoly~` + `sprintf` instead (see below).

## Instance-Specific Messaging

For sending different data to each `poly~` voice without repeatedly changing `target`:

### Pattern: thispoly~ + sprintf + forward/receive

Inside the subpatcher:

```
loadbang
  ↓
thispoly~ (bang → outputs instance number)
  ↓
sprintf "voice_%d_pitch"    → generates "voice_1_pitch", "voice_2_pitch", etc.
  ↓
prepend set
  ↓
receive (dynamically named)
```

For audio signals, use `send~`/`receive~` with the `set` message:

```
thispoly~
  ↓
sprintf "voice_%d_signal"
  ↓
prepend set
  ↓
receive~ (dynamically named via set message)
```

**From the parent patch**, use `forward` (which can change destination per message):

```
forward voice_1_pitch    → sends to voice 1
forward voice_3_pitch    → sends to voice 3
```

**Advantage**: Enables true parallel per-voice communication without `target` bottleneck, leveraging multiple CPU cores.

## bpatcher Techniques

### Embedding Patches

Two loading modes:
- **External file**: Specify patcher file in Inspector → more reusable, shared across projects
- **Embedded**: Enable `@embed 1` or Inspector → "Embed Patcher in Parent" → self-contained, travels with parent

To edit an embedded bpatcher: Object menu → "New View of Embedded Patcher"

### Using Arguments

Pass arguments via the Inspector's "Argument(s)" field. Inside the subpatcher, access them as `#1`, `#2`, `#3`, etc.

```
bpatcher @args 440 0.5    → #1 = 440, #2 = 0.5 inside subpatcher
```

### Dynamic send/receive Names

Use arguments to create unique communication channels per instance:

```
// Inside bpatcher, with argument = "synth1"
send #1_volume       → becomes "synth1_volume"
receive #1_volume    → matches the same channel
```

This avoids the `#0` problem where all loaded instances of the same bpatcher would share the same `#0` value. With arguments, each bpatcher copy can have a distinct namespace.

### Argument Inheritance

Nested bpatchers can forward arguments:

```
Parent bpatcher (arg: "main")
  └── Child bpatcher (arg: "#1_sub")    → becomes "main_sub"
       └── send #1_data                 → becomes "main_sub_data"
```

This enables deep hierarchical naming without conflicts.

### Presentation Mode

Enable "Open in Presentation" in the subpatcher Inspector to display its presentation view inside the parent's presentation mode. Design the UI once in the subpatcher and reuse across multiple parent patches.

## Combining bpatcher + poly~ for UI

### Pattern: bpatcher Arguments as poly~ Voice Index

Create a single bpatcher containing per-voice UI and controls. Duplicate it for each voice, passing the voice number as an argument:

```
bpatcher @args 1    → controls poly~ voice 1
bpatcher @args 2    → controls poly~ voice 2
bpatcher @args 3    → controls poly~ voice 3
```

Inside each bpatcher:
```
slider → send #1_volume    → "1_volume", "2_volume", "3_volume"
```

Inside each poly~ voice:
```
receive #1_volume           → #1 matches the voice number via @args
```

**Namespace convention**: Use a project-specific prefix to avoid collisions:
```
#0_PROJECT_#1_paramname     → e.g., "1234_SYNTH_1_volume"
```

**Benefit**: Modifying the bpatcher template once updates all voice UIs simultaneously. One production project uses this pattern with ~50 send/receive connections per voice.

## mc.poly~ Caveats

`mc.poly~` (multichannel variant) has a known issue where MIDI messages are not processed unless the subpatcher contains at least one `out~` object.

**Workaround**: Always include `out~ 1` in `mc.poly~` subpatchers, even if audio output is not needed. The audio processing can be disabled afterward — the presence of the object is sufficient for proper initialization.

## Sources

- https://leico.github.io/TechnicalNote/Max/poly-summary
- https://leico.github.io/TechnicalNote/Max/poly-individual-signal
- https://leico.github.io/TechnicalNote/Max/bpatcher-tips
- https://leico.github.io/TechnicalNote/Max/bpatcher-poly-interface
- https://leico.github.io/TechnicalNote/Max/poly-mc-problem
