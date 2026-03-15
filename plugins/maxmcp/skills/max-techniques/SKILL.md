---
name: max-techniques
description: |
  Max/MSP implementation techniques and best practices. Use this skill when:
  - Working with poly~ for polyphonic or parallel processing
  - Using bpatcher for modular patch design
  - Managing parameters with pattr/pattrstorage
  - Building audio signal chains or MIDI processing logic
  - Handling sampling rate dependent behavior
  - Multi-stage initialization patterns (loadbang, delay chains)
user-invocable: true
---

# Max/MSP Implementation Techniques

Practical techniques and patterns for building robust Max/MSP patches, covering poly~/bpatcher architecture, parameter management, and common pitfalls.

## Categories

### poly~ Techniques

Practical techniques for `poly~` voice management and multi-instance communication.

**Key topics**:
- Voice subpatcher template (adsr~ + thispoly~ mute pattern)
- Instance-specific messaging via `thispoly~` + `sprintf` + `forward`
- Communication hierarchy (global / instance-scoped / per-voice)
- Argument forwarding with transformation
- `mc.poly~` caveats

See [poly~ Techniques Reference](reference/poly-techniques.md)

### bpatcher Techniques

Reusable component patterns with `bpatcher`.

**Key topics**:
- Dynamic `send`/`receive` names via arguments
- Argument inheritance in nested bpatchers
- Combining `bpatcher` + `poly~` for per-voice UI

See [bpatcher Techniques Reference](reference/bpatcher-techniques.md)

### pattr & Parameter Management

State persistence and parameter control using `pattr`, `pattrstorage`, and the Parameter Inspector.

**Key topics**:
- Parameter binding and storage with `pattr`/`pattrstorage`
- Parameter Inspector settings (mode, initial value, range)
- Initialization timing (`live.thisdevice` vs Parameter Inspector)
- Avoiding `pattr` naming collisions in `bpatcher`
- Range limitation workarounds (default 0-127)

See [pattr & Parameters Reference](reference/pattr-parameters.md)

### General Tips

Proven patterns for safe and reliable patch behavior.

**Key topics**:
- Sampling rate detection with `dspstate~` to avoid Nyquist issues
- Increment/decrement counter pattern
- `change` for feedback loop prevention
- `closebang` cleanup, output safety chain
- Normalized parameter interface, numbered sample file loading

See [Tips Reference](reference/tips.md)

### Cascading Multi-Stage Initialization

Sequential initialization pattern using chained `delay` → `trigger` → `send` for standalone apps and installations.

**Key topics**:
- Multi-stage `delay → t b b → send` building block
- Staircase layout rules
- M4L variant comparison

See [Cascading Init Reference](reference/cascading-init.md)

