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

## Sources

- https://leico.github.io/TechnicalNote/Max/constant
- https://leico.github.io/TechnicalNote/Max/separate-samplingrate
