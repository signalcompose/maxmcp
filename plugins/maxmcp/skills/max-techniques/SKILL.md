---
name: max-techniques
description: |
  Max/MSP implementation techniques and best practices. Use this skill when:
  - Working with poly~ for polyphonic or parallel processing
  - Using bpatcher for modular patch design
  - Managing parameters with pattr/pattrstorage
  - Needing safe constant parameter patterns
  - Handling sampling rate dependent behavior
invocation: user
---

# Max/MSP Implementation Techniques

Practical techniques and patterns for building robust Max/MSP patches, covering poly~/bpatcher architecture, parameter management, and common pitfalls.

## Categories

### poly~ & bpatcher

Modular patch architecture using `poly~` for voice/instance management and `bpatcher` for reusable UI components.

**Key topics**:
- `poly~` voice management (`target`, `thispoly~`, `#0`)
- Instance-specific messaging via `forward`/`receive` with `sprintf`
- `bpatcher` embedding, arguments, and presentation mode
- Combining `bpatcher` + `poly~` for per-voice UI
- `mc.poly~` caveats

See [poly~ & bpatcher Reference](reference/poly-bpatcher.md)

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
- `trigger` as the safest constant parameter source
- Sampling rate detection with `dspstate~` to avoid Nyquist issues

See [Tips Reference](reference/tips.md)
