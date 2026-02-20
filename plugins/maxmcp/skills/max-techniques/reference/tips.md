# General Max/MSP Tips

Proven patterns for safe and reliable patch behavior.

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

## Sources

- https://leico.github.io/TechnicalNote/Max/constant
- https://leico.github.io/TechnicalNote/Max/separate-samplingrate
