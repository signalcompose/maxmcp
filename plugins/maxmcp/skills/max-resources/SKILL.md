# Max Resources Skill

Access Max/MSP built-in resources: reference pages, snippets, example patches, and user guides.

## When to Use

Use this skill when users ask about:
- Max object usage ("How do I use cycle~?")
- Object parameters, inlets, outlets
- Example patches ("Show me FM synthesis examples")
- Snippets and code patterns
- Max concepts and tutorials

**Trigger words**: "Max object", "reference", "how to use", "example", "snippet", "documentation"

## Quick Reference

### Script Locations

All scripts are in `plugins/maxmcp/skills/max-resources/scripts/`:

| Script | Purpose |
|--------|---------|
| `build-index.sh` | Build object index cache |
| `check-cache.sh` | Verify cache validity |
| `search-objects.sh` | Search indexed objects |
| `search-fts.sh` | Full-text search (SQLite) |
| `get-reference.sh` | Get object reference page |
| `list-examples.sh` | List example patches |
| `get-snippet.sh` | Get code snippets |

### Resource Paths (Max.app)

```
/Applications/Max.app/Contents/Resources/
├── C74/docs/refpages/       # Object references (XML)
│   ├── max-ref/             # Max objects
│   ├── msp-ref/             # MSP (audio)
│   ├── jit-ref/             # Jitter (video)
│   ├── m4l-ref/             # Max for Live
│   └── gen-ref/             # Gen
├── C74/docs/userguide/      # User guides (JSON)
├── C74/snippets/            # Code snippets
└── Examples/                # Example patches
```

## Usage Patterns

### 1. Cache Management

First-time setup or after Max update:

```bash
# Check cache status
./scripts/check-cache.sh

# Build/rebuild index
./scripts/build-index.sh
```

Cache location: `~/.maxmcp/cache/`

### 2. Object Search

```bash
# Search by name
./scripts/search-objects.sh cycle

# Search in specific category
./scripts/search-objects.sh filter msp-ref
```

### 3. Get Reference

```bash
# Full reference (XML)
./scripts/get-reference.sh cycle~

# Summary only
./scripts/get-reference.sh metro --summary
```

### 4. Example Patches

```bash
# List categories
./scripts/list-examples.sh

# List patches in category
./scripts/list-examples.sh synths

# Search within category
./scripts/list-examples.sh effects reverb
```

### 5. Snippets

```bash
# List snippet categories
./scripts/get-snippet.sh

# List snippets in category
./scripts/get-snippet.sh msp

# Get specific snippet
./scripts/get-snippet.sh msp filter
```

### 6. Full-Text Search

```bash
# Search user guides
./scripts/search-fts.sh "oscillator"
./scripts/search-fts.sh "FM synthesis" 10
```

## Common Objects Quick Reference

### Audio (MSP)
- `cycle~` - Sine oscillator
- `phasor~` - Ramp oscillator
- `noise~` - White noise
- `*~` `+~` `-~` `/~` - Signal math
- `dac~` `adc~` - Audio I/O
- `buffer~` `play~` `groove~` - Sample playback
- `biquad~` `svf~` `onepole~` - Filters

### Control (Max)
- `metro` - Timer/clock
- `counter` - Counting
- `random` - Random numbers
- `gate` `switch` `router` - Routing
- `pack` `unpack` - List operations
- `coll` `dict` - Data storage

### UI Objects
- `slider` `dial` `number` - Value input
- `button` `toggle` - Triggers
- `multislider` - Multiple values
- `waveform~` `scope~` - Visualization

## Integration with patch-guidelines

When recreating patches from examples:

1. Use this skill to find and analyze example patches
2. Extract object list and connections from JSON
3. Apply `patch-guidelines` rules for layout and naming
4. Use MCP tools to create the actual patch

See `references/mcp-recreation.md` for detailed workflow.

## Detailed Documentation

For format specifications and advanced usage:
- `references/resource-paths.md` - Path discovery, version detection
- `references/refpage-format.md` - XML reference format
- `references/maxpat-format.md` - Patch JSON format
- `references/mcp-recreation.md` - Example-to-patch workflow
- `examples/lookup-object.md` - Search examples
- `examples/recreate-patch.md` - Patch recreation examples
