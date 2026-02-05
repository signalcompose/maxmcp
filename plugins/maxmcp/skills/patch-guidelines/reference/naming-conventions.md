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
