# Naming Conventions for Max/MSP Objects

Standards for varnames and object identification in MaxMCP patches.

## Varname Requirements

### When Varnames Are Required

Always assign varnames to:
- Objects that will be connected programmatically
- Objects whose attributes will be modified
- Objects that need to be referenced later
- Key processing objects in the signal chain
- UI elements that control parameters

### When Varnames Are Optional

Varnames may be omitted for:
- Simple utility objects (like comments)
- Objects that won't be referenced
- Temporary or intermediate objects

## Naming Patterns

### General Format

```
{category}_{function}[_{modifier}]
```

Examples:
- `osc_main` - Main oscillator
- `filt_lowpass_1` - First lowpass filter
- `env_amp` - Amplitude envelope

### Category Prefixes

| Prefix | Category | Examples |
|--------|----------|----------|
| `osc_` | Oscillators | `osc_sine`, `osc_saw`, `osc_lfo` |
| `filt_` | Filters | `filt_lowpass`, `filt_hipass`, `filt_biquad` |
| `env_` | Envelopes | `env_amp`, `env_filt`, `env_pitch` |
| `gain_` | Gain/Volume | `gain_master`, `gain_osc1`, `gain_sub` |
| `mix_` | Mixers | `mix_main`, `mix_fx`, `mix_aux` |
| `fx_` | Effects | `fx_delay`, `fx_reverb`, `fx_chorus` |
| `ctrl_` | UI Controls | `ctrl_freq`, `ctrl_cutoff`, `ctrl_res` |
| `in_` | Inputs | `in_audio_l`, `in_midi`, `in_osc` |
| `out_` | Outputs | `out_audio_l`, `out_midi`, `out_scope` |
| `lfo_` | LFOs | `lfo_vibrato`, `lfo_tremolo`, `lfo_pan` |
| `seq_` | Sequencers | `seq_main`, `seq_bass`, `seq_drums` |
| `buf_` | Buffers | `buf_sample`, `buf_grain`, `buf_delay` |
| `sub_` | Subpatchers | `sub_synth`, `sub_fx`, `sub_midi` |
| `msg_` | Messages | `msg_init`, `msg_preset`, `msg_bang` |
| `num_` | Number boxes | `num_freq`, `num_amp`, `num_time` |

### Modifier Suffixes

Use modifiers for multiple similar objects:

| Suffix | Meaning | Example |
|--------|---------|---------|
| `_l`, `_r` | Left/Right channels | `gain_out_l`, `gain_out_r` |
| `_1`, `_2`, `_3` | Numbered instances | `osc_1`, `osc_2`, `osc_3` |
| `_main` | Primary instance | `mix_main` |
| `_aux` | Auxiliary | `out_aux` |
| `_dry`, `_wet` | Dry/Wet signals | `gain_dry`, `gain_wet` |
| `_pre`, `_post` | Pre/Post processing | `gain_pre`, `gain_post` |

## Signal Flow Naming

### Audio Chain Example

```
in_audio → gain_input → filt_pre → fx_comp → mix_main → gain_master → out_dac
```

### MIDI Chain Example

```
in_midi → ctrl_velocity → seq_main → note_out → out_midi
```

## Naming Rules

### Do's

- Use lowercase letters only
- Separate words with underscores
- Keep names concise but descriptive
- Use consistent prefixes within a patch
- Include channel indicators for stereo objects

### Don'ts

- Avoid spaces (Max doesn't allow them anyway)
- Avoid special characters except underscore
- Don't use Max reserved words
- Don't start with numbers
- Don't use overly long names (max ~20 chars)

## Examples by Object Type

### Oscillators

```
osc_sine_440    # Sine oscillator at 440Hz
osc_saw_lfo     # Sawtooth used as LFO
osc_noise       # Noise generator
```

### Filters

```
filt_lp_24      # 24dB lowpass filter
filt_hp_main    # Main highpass filter
filt_bp_vowel   # Bandpass for vowel formant
```

### Envelopes

```
env_adsr_amp    # ADSR for amplitude
env_ar_filt     # AR envelope for filter
env_curve_pitch # Curved envelope for pitch
```

### UI Controls

```
ctrl_dial_freq      # Dial for frequency
ctrl_slider_amp     # Slider for amplitude
ctrl_toggle_bypass  # Toggle for bypass
ctrl_button_trig    # Button for trigger
```

### Effects

```
fx_delay_ping       # Ping-pong delay
fx_reverb_plate     # Plate reverb
fx_dist_tube        # Tube distortion
```

## Subpatcher Naming

### Internal Objects

For objects inside subpatchers, include context:

```
# Inside p synth_voice
voice_osc_1
voice_filt
voice_env_amp
voice_gain_out
```

### Inlet/Outlet Labels

Name inlets and outlets clearly:

```
# Inlets
inlet_freq      # Frequency input
inlet_gate      # Gate input
inlet_mod       # Modulation input

# Outlets
outlet_sig_l    # Left signal output
outlet_sig_r    # Right signal output
outlet_env      # Envelope output
```

### Subpatcher Argument Documentation

When a subpatcher receives arguments (via `bpatcher @args` or `poly~ @args`), document them with a `comment` object placed near the top of the patch. Use the format `#N : description` for each argument:

```
comment: "#1 : unique_prefix
#2 : base key - used start position of sample
#3 : Sample file prefix
#4 : distance between samples(ms)
#5 : Sample file count"
```

**Placement**: Position the comment near the top-left of the patch, close to the first `inlet` object, so it is immediately visible when the subpatcher is opened.

**Why document arguments?**
- Arguments (`#1`, `#2`, ...) are replaced at load time — their meaning is invisible in the running patch
- Without documentation, understanding what values to pass requires reading the entire subpatcher
- Serves as the "function signature" for the subpatcher

### Inlet/Outlet Comment Annotation

The `comment` attribute on `inlet` and `outlet` objects serves as a self-documenting interface. It appears as a tooltip when hovering over the corresponding inlet/outlet in the parent patch, helping users connect the correct data types.

**Format**: `(type) description`

```
inlet  @comment "(list) note data"
inlet  @comment "(bang) change rhythm palette"
outlet @comment "(signal) L channel Out"
outlet @comment "(signal) R channel Out"
outlet @comment "(bang) when buffer read done"
outlet @comment "(symbol) playing sound name"
```

**Common type annotations**:

| Type | Meaning | Example |
|------|---------|---------|
| `(signal)` | Audio signal (~) | `(signal) L channel Out` |
| `(bang)` | Bang trigger | `(bang) when buffer read done` |
| `(int)` | Integer value | `(int) MIDI note number` |
| `(float)` | Float value | `(float) frequency in Hz` |
| `(list)` | List of values | `(list) note data` |
| `(symbol)` | Symbol/string | `(symbol) playing sound name` |

**Why annotate every inlet/outlet?**
- Parent patch users see the expected data type on hover — no need to open the subpatcher
- Prevents common wiring mistakes (e.g., connecting a message outlet to a signal inlet)
- Acts as living documentation that stays with the object

### Intermediate Data Flow Comments

For complex data transformations within a patch, place `comment` objects at key processing stages to document how the data format changes. This is especially useful when list order or data type changes mid-flow.

**Example — note data reordering**:

```
in 1
  ↓          comment: "(int) note\n(int) velocity"
swap
  ↓          comment: "(int) velocity\n(int) note"
unpack 0 0
```

**Example — calculation annotation**:

```
pak 0. 0.    comment: "duration = end - start"
  ↓
* 1.         comment: "speed * duration"
```

**When to add intermediate comments**:
- After `swap`, `zl.rev`, `unpack`/`pack` that change data order
- At arithmetic stages where the meaning of the value changes
- Before complex branching where multiple outlets carry different data
- At the boundary between "what comes in" and "what goes out" of a processing section

## Special Cases

### Multiple Voices

For polyphonic patches:

```
voice_1_osc
voice_1_filt
voice_1_env

voice_2_osc
voice_2_filt
voice_2_env
```

### Parameter Groups

For related parameters:

```
freq_coarse
freq_fine
freq_mod_depth
freq_mod_rate
```
