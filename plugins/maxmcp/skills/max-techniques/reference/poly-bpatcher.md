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

**Important**: Each index number must be unique — do not place two `in 1` or two `out~ 1` objects in the same subpatcher. Use different index numbers (e.g., `in 1`, `in 2`) to create additional inlets/outlets.

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

**Multiple instances**: `thispoly~` can be placed multiple times in the same subpatcher — all report the same voice number and share the same mute state. This is useful when mute control and voice number retrieval happen in different locations (e.g., left column for `adsr~` → `thispoly~`, right column for `loadbang` → `thispoly~` → `sprintf`). Avoid unnecessary duplication; use only where wiring to a single instance would be impractical.

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

## poly~ Communication Hierarchy

Complex poly~ architectures use three distinct communication layers, each with different scope and timing:

### Layer 1: Global Broadcast (r __Master_*)

Global `send`/`receive` pairs that reach all voices across all poly~ instances. Used for system-wide initialization events and shared state changes.

```
// Parent or top-level patch
s __Init_BufferNames        ← all sampler voices everywhere respond
s __Init_ReadSamples        ← all voices load their samples
s __Init_BindBuffers        ← all wave~/waveform~ bind to buffers
```

All poly~ voices receive these simultaneously. Combine with Cascading Multi-Stage Initialization (see tips.md) for ordered startup.

### Layer 2: Instance-Scoped (r #1_*)

`receive` objects using the argument-based prefix. Shared by all voices within the same poly~ instance, but isolated from other instances.

```
// Inside poly~ voice subpatcher
r #1_OriginKey              ← all 16 voices in this instance share the same base key
r #1_Start                  ← all voices use the same loop start
r #1_End                    ← all voices use the same loop end
```

The parent sets these values via `send`:
```
s myPrefix_Sampler_OriginKey     ← #1 was set to "myPrefix_Sampler" via @args
```

### Layer 3: Per-Voice (in N via target)

Message input routed to individual voices by poly~'s voice allocation or explicit `target` messages. Each voice receives different data.

```
// Parent
prepend note → mc.poly~     ← poly~ routes to the allocated voice

// Inside voice
in 1 → unpack 0 0           ← only this voice processes the note
```

### Choosing the Right Layer

| Need | Layer | Mechanism |
|------|-------|-----------|
| System-wide init events | Global | `r __Master_*` |
| Per-instrument parameters (key, loop region) | Instance | `r #1_*` |
| Per-note data (pitch, velocity) | Per-voice | `in N` |
| Per-voice dynamic naming | Per-voice | `thispoly~` + `sprintf` |

## Argument Forwarding with Transformation

When a parent subpatcher loads a child via `poly~` or `bpatcher`, arguments can be selectively forwarded, combined, or renumbered.

### Concatenation

Append a suffix to the parent's argument to create a child-specific namespace:

```
// Parent has #1 = "myProject"
mc.poly~ child_patch 16 @args #1_Sampler #2 #4
                               ↓          ↓  ↓
                          child #1     child #2  child #3
```

The child receives `#1` = `"myProject_Sampler"` — a unique namespace derived from the parent's prefix.

### Selective Forwarding

Not all parent arguments need to reach the child. Pass only what the child needs:

```
// Parent: 5 arguments (#1-#5)
//   #1: unique prefix
//   #2: base key
//   #3: sample file prefix
//   #4: sample interval (ms)
//   #5: sample count

mc.poly~ child 16 @args #1_Sampler #2 #4
// Child receives only 3 args:
//   child #1 = parent #1 + "_Sampler"  (concatenated)
//   child #2 = parent #2              (forwarded as-is)
//   child #3 = parent #4              (renumbered: parent's 4th → child's 3rd)
```

### Documentation Convention

Both parent and child should document their arguments with a comment block (see patch-guidelines: Subpatcher Argument Documentation). When arguments are transformed, the mapping is implicit in the `@args` — the comment in each patch documents the meaning at that level.

## mc.poly~ Caveats

`mc.poly~` (multichannel variant) has a known issue where MIDI messages are not processed unless the subpatcher contains at least one `out~` object.

**Workaround**: Always include `out~ 1` in `mc.poly~` subpatchers, even if audio output is not needed. The audio processing can be disabled afterward — the presence of the object is sufficient for proper initialization.

## Sources

- https://leico.github.io/TechnicalNote/Max/poly-summary
- https://leico.github.io/TechnicalNote/Max/poly-individual-signal
- https://leico.github.io/TechnicalNote/Max/bpatcher-tips
- https://leico.github.io/TechnicalNote/Max/bpatcher-poly-interface
- https://leico.github.io/TechnicalNote/Max/poly-mc-problem
