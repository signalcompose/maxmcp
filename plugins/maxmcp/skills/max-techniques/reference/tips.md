# General Max/MSP Tips

Proven patterns for safe and reliable patch behavior.

## Hot Inlet / Cold Inlet

### The Fundamental Rule

Nearly all Max objects follow this convention:

- **Leftmost inlet = Hot**: Receiving a value triggers immediate output
- **All other inlets = Cold**: Receiving a value stores it internally without producing output

The stored cold-inlet value is used the next time the hot inlet triggers.

### Example: The + Object

```
  [hot]  [cold]
    ↓      ↓
    +  0
    ↓
  (sum)
```

- Send `5` to right inlet (cold): stores 5, no output
- Send `3` to left inlet (hot): outputs `3 + 5 = 8`
- Send `10` to left inlet (hot): outputs `10 + 5 = 15` (5 is still stored)

### Why trigger Outputs Right-to-Left

`trigger` (t) sends outputs from the rightmost outlet first, proceeding left. This is not arbitrary — it is designed to work with the hot/cold inlet convention:

```
t b -1
│    │
│    └→ + (right inlet = cold: stores -1, no output)
└───→ int (left inlet = hot: triggers output through +)
```

1. Right outlet fires first → value reaches downstream cold inlets (stored, no output)
2. Left outlet fires last → value reaches downstream hot inlets (triggers processing)
3. Result: all values are in place when processing begins

### Exceptions: All-Hot Objects

Some objects intentionally make every inlet hot:

| Object | Behavior |
|--------|----------|
| `pak` | Any inlet triggers list output (compare with `pack`: only left inlet is hot) |
| `buddy` | Outputs only when all inlets have received values |

### Practical Implications

Understanding hot/cold is essential for:

- **Correct trigger argument ordering**: Place the hot-inlet-bound output on the left
- **Choosing pack vs pak**: `pack` when you want controlled triggering, `pak` when any change should trigger
- **Debugging silent patches**: If nothing happens when you send a value, you're likely hitting a cold inlet

## Constant Parameters with trigger

### The Problem

Many Max objects that store values are vulnerable to accidental modification:

| Object | Risk |
|---|---|
| `int` / `float` | Right inlet overwrites stored value |
| `message` | Clicking the box sends the message AND right outlet allows overwriting |
| `button` / `bangbang` | Clicking in edit mode or stray connections can trigger unintended bangs |
| `loadmess` | Only fires on patch load, not on demand |
| `pack` / `pak` | Inlets accept new values, changing stored data |

### The Solution: trigger

`trigger` with typed arguments is the safest way to output constant values:

```
trigger i 440 f 0.5 b
         │     │    └── bang
         │     └── float 0.5 (always)
         └── int 440 (always)
```

**Why trigger is safe**:
- Arguments are fixed at creation — no inlet can change them
- No clickable UI that could accidentally fire
- No right inlet for value injection
- Outputs are deterministic regardless of input (any input triggers the sequence)
- Right-to-left output order is predictable

**Usage pattern**:
```
bang / any message
  ↓
trigger 440 0.5 b
  │      │    └── to envelope (bang)
  │      └── to gain (*~) (0.5)
  └── to oscillator (cycle~) (440)
```

**Type specifiers**: `i` (int), `f` (float), `b` (bang), `l` (list), `s` (symbol), or literal values.

## Sampling Rate Dependent Behavior

### The Problem

Filter cutoff frequencies that exceed the Nyquist frequency (half the sampling rate) cause DSP failure. For example, a 30kHz high-pass filter at 44.1kHz sampling rate (Nyquist = 22.05kHz) can freeze the entire DSP chain.

### The Solution: dspstate~

`dspstate~` reports the current audio configuration:

- **Outlet 0**: Signal (1.0 when DSP is on, 0.0 when off)
- **Outlet 1**: Current sampling rate (integer, e.g., 44100, 48000, 96000)
- **Outlet 2**: Signal vector size

### Adaptive Filter Pattern

```
dspstate~
  ↓ (outlet 1: sampling rate)
/ 2                              → Nyquist frequency
  ↓
clip 20 20000                    → safety clamp
  ↓
filtercoeff~ / biquad~           → use as max cutoff
```

**Practical implementation**:

```
dspstate~
  ↓ (outlet 1)
select                           → branch by rate
  ├── 44100 → 20000 (safe max)
  ├── 48000 → 24000
  └── default → 20000 (conservative fallback)
```

**Rule of thumb**:
- Sampling rate < 48kHz → limit high-frequency parameters to 20kHz
- Sampling rate >= 48kHz → can safely extend to 24kHz
- Always provide a conservative default for unknown rates

### When to Use

- Any patch with user-controllable filter cutoff frequencies
- Patches that may run at different sampling rates (e.g., shared patches, multi-interface setups)
- Patches using `filtercoeff~`, `biquad~`, `svf~`, or other filter objects with frequency parameters

## Message Manipulation Patterns

### Avoid message Boxes

`message` boxes have multiple risks:
- Clicking the box in locked mode sends the message
- Right inlet allows overwriting the stored value
- Easy to trigger accidentally during editing

Use `trigger` or dedicated objects instead.

### Setting Values Without Output: prepend set

To set an object's value without triggering output (equivalent to `set $1`), use `prepend set`:

```
incoming value
  ↓
prepend set
  ↓
number / flonum / message    → value is set, no output triggered
```

This is safer and more readable than using `message` with `set $1`.

### Appending to Messages: append

To add a value at the end of a message or list:

```
incoming value
  ↓
append hz
  ↓
→ "440 hz"
```

### Single-Value Messages: trigger

For sending a specific constant value (e.g., `1`, `clear`), use `trigger` instead of `message`:

```
bang
  ↓
t 1         → always outputs 1
```

```
bang
  ↓
t clear     → always outputs "clear"
```

**Why not message?** A `message` box with `1` can be clicked accidentally, and its value can be overwritten via the right inlet. `trigger` has neither problem.

### List Storage: zl.reg

To store and recall a list, use `zl.reg`:

```
list input
  ↓
zl.reg
  ↓ (bang to recall)
stored list output
```

**Why not message or coll?** `zl.reg` is purpose-built for list storage — no click risk, no file overhead.

### Building Specific Lists: pack

To construct a specific list from individual values:

```
pack 0 0. symbol
  │   │    └── symbol inlet
  │   └── float inlet
  └── int inlet (triggers output)
```

`pack` outputs the list when the leftmost inlet receives a value. Use `pak` if any inlet should trigger output.

## Increment/Decrement Counter Pattern

### The Problem

A common UI need: two buttons (up/down or left/right) that adjust a shared counter. Building this naively requires duplicating the counter logic for each button.

### The Pattern

```
[button -]          [button +]
    ↓                   ↓
  t b -1             t b 1
  │    │             │    │
  │    └──→  +  0 ←──┘    │
  └──→ int 0 ←────────────┘
         ↓
    live.numbox (or number)
```

### How It Works (Hot/Cold in Action)

Each button triggers a `trigger` that outputs a bang and an offset value:

1. **Right outlet fires first** (cold inlet path):
   - `t b -1`: sends `-1` to `+` right inlet (cold — stored, no output)
   - `t b 1`: sends `1` to `+` right inlet (cold — stored, no output)

2. **Left outlet fires second** (hot inlet path):
   - Bang goes to `int` left inlet (hot — outputs stored value)
   - `int`'s output flows to `+` left inlet (hot — adds stored offset, outputs sum)
   - Sum flows to display AND back to `int` right inlet (cold — stored as new value)

### Key Insight

Both `t b -1` and `t b 1` share the same `int` and `+` objects. This works because:
- The offset is set on the cold inlet before the bang triggers the hot inlet
- 4 objects total (2 triggers + 1 int + 1 add) handle bidirectional counting

### Bounds Checking

Combine with `clip` or `live.numbox` range to prevent overflow:

```
+ 0
  ↓
clip 0 7          ← constrain to valid range
  ↓
number
```

Or use `live.numbox` with `_parameter_range` to enforce bounds automatically.

## change Object for Feedback Loop Prevention

### The Problem

Circular connections (A → B → C → A) cause infinite message loops. This commonly occurs in bidirectional sync patterns — for example, a numbox that both controls and reflects a LOM value.

### The Pattern

```
incoming value
  ↓
change              ← only passes if value differs from last output
  ↓
(downstream processing)
  ↓
(feedback path back to incoming)
```

### How It Works

`change` stores the last value it output. When a new value arrives:
- If it differs from the stored value → output the value and update stored value
- If it matches the stored value → suppress (no output), breaking the loop

### Initial Value: change -1

`change` defaults to storing `0` internally. If your expected initial value is `0`, the first legitimate `0` would be suppressed. Use `change -1` (or any value outside your expected range) to initialize:

```
change -1           ← stored value starts at -1
                    ← first input of 0 passes through (0 ≠ -1)
```

### When to Use

- Bidirectional LOM sync (slider ↔ live.observer)
- Feedback connections between related parameters
- Any circular message path where values may echo
- Counter patterns where the output feeds back to the input (as in the Increment/Decrement pattern above)

## Cascading Multi-Stage Initialization

### The Problem

Complex patches (especially standalone applications and installations) require multiple initialization steps that must execute in a specific order with timing gaps between them. For example: load configuration → read audio files → bind buffers → initialize parameters → start audio → start sequencer. Each step may need the previous step to complete before proceeding.

### The Pattern

```
loadbang
  ↓
delay 10000                    ← wait for patch to fully load
  ↓
t b b
│   │
│   └→ s __Init_Step1         ← Step 1: e.g., set buffer names
└───→ delay 1000
        ↓
        t b b
        │   │
        │   └→ s __Init_Step2 ← Step 2: e.g., read sample files
        └───→ delay 1000
                ↓
                t b b
                │   │
                │   └→ s __Init_Step3  ← Step 3: e.g., bind buffers
                └───→ delay 1000
                        ↓
                        ...   ← continue chaining
```

### How It Works

Each stage follows the same `delay → t b b` building block:

1. `delay N` waits for the previous step to complete (N varies by step complexity)
2. `t b b` splits into two paths (right-to-left execution):
   - **Right outlet**: `send` broadcasts a bang to receivers for this step's work
   - **Left outlet**: triggers the next `delay` in the chain

Receivers anywhere in the patch respond to the broadcast:

```
r __Init_Step1               r __Init_Step2
  ↓                            ↓
(set buffer names)           (read sample files)
```

### Key Design Decisions

**Why delay between steps?**
Some operations (file I/O, network requests, DSP setup) are asynchronous. A fixed delay ensures each step has time to complete before the next begins. Typical values: 1000ms for file operations, 5000-10000ms for network/audio setup.

**Why send/receive instead of direct connections?**
The broadcast pattern decouples the initialization chain from the work it triggers. Receivers can be placed anywhere in the patch hierarchy, and steps can be reordered or added without rewiring.

**Why loadbang + delay instead of immediate execution?**
The initial delay (e.g., `delay 10000`) ensures the entire patch is fully loaded before initialization begins. This is critical for standalone applications where subpatchers and externals may still be loading.

### Manual Triggers

Add `textbutton` objects connected to each `send` for manual re-triggering during development:

```
textbutton "Step 1"  →  s __Init_Step1
textbutton "Step 2"  →  s __Init_Step2
```

This allows testing individual steps without restarting the entire sequence.

### When to Use

- Standalone Max applications with complex startup requirements
- Installation patches that must initialize hardware, audio, and network in order
- Any patch with multiple asynchronous setup operations that depend on each other
- Patches that fetch data from external servers before playback

### Comparison with M4L Two-Stage Init

| Aspect | Cascading Chain | M4L Two-Stage |
|--------|----------------|---------------|
| Trigger | `loadbang` | `live.thisdevice` |
| Stages | Unlimited (chain as needed) | Typically 2 (immediate + delayed) |
| Namespace | Global (`__Master_*` or similar) | Device-scoped (`---`) |
| Use case | Standalone apps, installations | M4L devices in Live |

## closebang Cleanup Pattern

### The Problem

When a patch closes, active audio and held MIDI notes persist momentarily, causing clicks, stuck notes, or resource leaks. Without explicit cleanup, closing a patch can leave the audio system in an inconsistent state.

### The Pattern

```
closebang                    closebang
  ↓                            ↓
t stop                       flush
  ↓                            ↓
dac~                         (MIDI output)
```

### closebang vs loadbang

| Object | Fires when | Use for |
|--------|-----------|---------|
| `loadbang` | Patch opens | Initialization |
| `closebang` | Patch closes | Cleanup |
| `freebang` | Object is deleted | Per-object cleanup |

### Common Cleanup Actions

| Action | Pattern | Purpose |
|--------|---------|---------|
| Stop audio | `closebang → t stop → dac~` | Prevent audio clicks on close |
| Release MIDI | `closebang → flush` | Send note-off for all held notes |
| Stop sequencer | `closebang → t stop → seq/seq~` | Halt playback |
| Stop metro | `closebang → t 0 → metro` | Stop timed processes |
| Stop network | `closebang → t stop → maxurl` | Cancel pending HTTP requests |

### Multiple Cleanup Actions

Use `trigger` to sequence multiple cleanup operations:

```
closebang
  ↓
t stop stop b
│     │    └→ flush (MIDI cleanup)
│     └────→ seq (stop sequencer)
└──────────→ dac~ (stop audio last)
```

**Order matters**: Stop audio last to allow MIDI note-offs to reach the output cleanly.

### When to Use

- Any patch with `dac~` (always add audio cleanup)
- Patches that send MIDI notes (prevent stuck notes)
- Patches with network connections (`maxurl`, `udpsend`)
- Patches with timed processes (`metro`, `qmetro`, `delay`)
- Standalone applications where clean shutdown is critical

## Output Safety Chain

### The Problem

Audio processing chains can produce unexpected level spikes, DC offset, or clipping — especially when using dynamic effects (compressors, distortion) or sample playback with varying source levels. Without output protection, these can damage speakers or cause distortion.

### The Pattern

```
signal source
  ↓
omx.comp~ (or other processing)
  ↓
*~ 1.              ← gain stage (linear multiplier)
  ↓
limi~ @dcblock 1 @lookahead 100   ← output protection
  ↓
outlet
```

The gain value comes from a dB-to-linear conversion:

```
pattr gain → number → gen db2level → *~
```

### How It Works

1. **`gen db2level`**: Converts a dB value to a linear multiplier (e.g., -6dB → 0.5, 0dB → 1.0)
2. **`*~`**: Applies the linear gain to the signal — safer than `*~` with raw dB values
3. **`limi~`**: Lookahead limiter prevents output from exceeding 0dBFS
   - `@dcblock 1`: Removes DC offset that can accumulate through processing
   - `@lookahead 100`: 100ms lookahead for transparent limiting (no audible pumping)

### Why This Order

| Stage | Purpose |
|-------|---------|
| Gain (`*~`) first | User-controlled level adjustment before limiting |
| Limiter (`limi~`) last | Catches any peaks the gain stage doesn't anticipate |

Placing the limiter after gain ensures the output never clips regardless of gain setting.

### Stereo Configuration

For stereo output, duplicate the chain for each channel:

```
mc.mixdown~ → mc.unpack~
  ├── L: *~ → limi~ @dcblock 1 @lookahead 100 → outlet
  └── R: *~ → limi~ @dcblock 1 @lookahead 100 → outlet
```

Both channels share the same gain source (`gen db2level` output).

### When to Use

- Any patch with audio output (`dac~`, `outlet~`, `mc.send~`)
- After dynamic processing (compressors, distortion, waveshaping)
- Sample playback with unknown source levels
- Live performance patches where clipping is unacceptable

## Normalized Parameter Interface

### The Problem

Audio effect objects (e.g., `omx.comp~`) accept parameters in different ranges (threshold: 0-100dB, ratio: 0-100, attack: 0-150ms, release: 0-150ms). When exposing these parameters via `pattr` for preset management or external control, each parameter needs different range handling, making the interface inconsistent.

### The Pattern

```
pattr comp_threshold          pattr comp_attack
  ↓                             ↓
number (0-100)                number (0-100)
  ↓                             ↓
scale 0 100 0 100             scale 0 100 0 150
  ↓                             ↓
prepend agcThreshold          prepend attack
  ↓                             ↓
omx.comp~  ←←←←←←←←←←←←←←←←←←┘
```

### How It Works

1. **`pattr`**: Stores normalized 0-100 value, exposed to `pattrstorage` for presets
2. **`number`** with `minimum`/`maximum` attributes: Clamps the value to the valid range (e.g., 0-100) and displays it to the user. This prevents out-of-range values from reaching downstream processing
3. **`scale 0 100 0 <actual_max>`**: Maps normalized range to the target object's actual range
4. **`prepend <param_name>`**: Formats as a message the target object understands

### Input Clamping with number

The `number` object's `minimum` and `maximum` attributes serve as the input guard:

```
number @minimum 0 @maximum 100    ← normalized parameters (comp, etc.)
number @minimum -70 @maximum 12   ← dB gain (-70dB to +12dB)
```

This clamping is essential — without it, values from `pattr` recall or external controllers could exceed the expected range, causing unexpected behavior in `scale` or downstream objects. The `number` object enforces bounds at the UI level before any processing occurs.

### Key Design Decisions

**Why normalize to 0-100?**
- All parameters share the same input range → consistent UI
- External controllers (MIDI CC, OSC) naturally map to 0-127 or 0-100
- Preset interpolation works uniformly across all parameters
- Parent patches can control all parameters with identical logic

**Why `scale` instead of direct range in `pattr`?**
- `pattr` range settings (Parameter Inspector) are hidden and easy to forget
- `scale` makes the mapping visible and editable in the patch
- Easy to adjust ranges without opening Inspector dialogs
- Multiple parameters can use different target ranges with the same source range

### Extending the Pattern

For parameters that need non-linear mapping (e.g., logarithmic frequency):

```
pattr frequency
  ↓
number (0-100)
  ↓
scale 0 100 0. 1.         ← normalize to 0-1
  ↓
expr pow($f1, 3.) * 19980 + 20   ← cubic curve: 20Hz-20kHz
  ↓
prepend cutoff
  ↓
target effect
```

### When to Use

- Wrapping effect objects with multiple parameters (`omx.comp~`, `omx.peaklim~`, etc.)
- Subpatchers that expose parameters to parent patches via `pattr`
- Any patch where external control (MIDI, OSC, pattrstorage) needs uniform ranges

## Numbered Sample File Loading

### The Problem

Sample-based instruments need to load one file from a set of numbered audio files (e.g., `Piano_01.wav` through `Piano_12.wav`). The selection may be random, sequential, or externally controlled. Building this with message boxes is fragile and hard to maintain.

### The Pattern

```
bang (trigger load)
  ↓
random N → + 1            ← 1-based random index
  ↓
zl.reg (stored prefix)    ← file name prefix (e.g., "Piano")
  ↓
sprintf %s_%02d.wav       ← formats "Piano_01.wav"
  ↓
t l l
├── prepend read → append 0 -1 → buffer~    ← load file (range: full)
└── prepend name → buffer~                   ← set buffer name (for waveform~ etc.)
```

### How It Works

1. **`random N` + `+ 1`**: Generates a 1-based index (1 to N). Use `+ 1` because `random N` outputs 0 to N-1
2. **`zl.reg`**: Stores the filename prefix, outputs it as a list when banged. The prefix is set during initialization via `send`/`receive`
3. **`sprintf %s_%02d.wav`**: Combines prefix + zero-padded index into a complete filename
4. **`t l l`**: Splits into two paths — file loading and buffer naming
5. **`prepend read` + `append 0 -1`**: Sends `read <filename> 0 -1` to `buffer~` (load entire file)
6. **`prepend name`**: Sets the buffer's reference name for other objects (`waveform~`, `groove~`)

### File Naming Convention

The pattern expects zero-padded numbered files:

```
<prefix>_01.wav
<prefix>_02.wav
...
<prefix>_12.wav
```

Adjust the `sprintf` format for different naming:
- `%s_%02d.wav` → `Piano_01.wav` (2-digit zero-padded)
- `%s_%03d.wav` → `Piano_001.wav` (3-digit)
- `%s_%d.wav` → `Piano_1.wav` (no padding)
- `%s-%d.aif` → `Piano-1.aif` (different separator and format)

### Initialization Integration

Combine with the Cascading Multi-Stage Initialization pattern:

```
r __Init_Step1 (SetBufferName)
  ↓
t <prefix>                    ← set the prefix
  ↓
zl.reg                        ← stored for later use

r __Init_Step2 (ReadSampleFile)
  ↓
random N → + 1 → ...         ← trigger random load
```

### When to Use

- Sample-based instruments with multiple sound variations
- Drum machines with randomized sample selection
- Granular synthesis with numbered grain files
- Any patch that manages a numbered collection of audio files

## change Object for Feedback Loop Prevention

### The Problem

Circular connections (A → B → C → A) create infinite loops. This commonly occurs when:
- Two parameters need bidirectional synchronization
- A UI control both sends and receives from the same source
- LOM observer output feeds back to the observed property

### The Pattern

```
source A
  ↓
change         ← only outputs when value differs from last output
  ↓
process → destination B
             ↓
           change     ← breaks the loop on the return path
             ↓
           → source A
```

### How It Works

`change` stores the last value it output. When a new value arrives:
- **Different from stored**: Outputs the value and updates stored value
- **Same as stored**: Suppresses the output (no message passes)

This breaks infinite loops because the second traversal of the loop carries the same value, which `change` blocks.

### Initializing with change -1

By default, `change` stores `0` as its initial value. This means the first `0` sent through it will be suppressed (it matches the stored default). If your counter or parameter starts at 0, this is a problem.

**Solution**: Initialize with a value that will never naturally occur:

```
change -1      ← initial stored value is -1
```

Now the first `0` passes through because `-1 ≠ 0`.

### Practical Example: Bidirectional Sync

```
observer (external value)
  ↓
change -1              ← prevents feedback when we set the value
  ↓
number (display)
  ↓
change -1              ← prevents feedback when observer updates
  ↓
prepend set value
  ↓
setter (writes back)
```

Without `change`, setting the value triggers the observer, which updates the number, which sets the value again — infinite loop.

### When to Use

- Bidirectional parameter binding (UI ↔ model)
- Observer + setter feedback paths
- Any circular signal flow where values stabilize after one round-trip

## dac~ / adc~ Consolidation

### The Issue

`dac~` and `adc~` can be placed multiple times in the same patch — all instances sharing the same channel numbers will route to the same audio hardware I/O. While technically valid, scattering multiple `dac~` or `adc~` objects across a patch makes signal flow harder to trace, especially when analyzing or modifying the patch programmatically.

### Recommendation

Consolidate audio I/O to a single location in the patch:

```
[all signal processing]
  ├── L channel
  └── R channel
        ↓
      dac~ 1 2    ← single dac~ at the bottom of the output section
```

If multiple sections need independent audio output control, route all signals to one `dac~` via `send~`/`receive~` rather than placing separate `dac~` objects in each section.

## Sources

- https://leico.github.io/TechnicalNote/Max/constant
- https://leico.github.io/TechnicalNote/Max/separate-samplingrate
- Production M4L device analysis
