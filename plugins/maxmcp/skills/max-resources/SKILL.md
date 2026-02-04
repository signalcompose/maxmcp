---
name: max-resources
description: |
  Access Max/MSP built-in resources. Use when:
  - User asks about Max objects ("How do I use cycle~?")
  - User wants to find examples or snippets
  - User needs object reference information (inlets, outlets, methods)
argument-hint: "[<object-name>|fts <query>]"
---

# Max Resources Skill

Access Max/MSP built-in resources directly from Max.app: reference pages, snippets, example patches, and user guides.

## Agentic Search Approach

This skill uses **direct filesystem exploration** with Claude Code's built-in tools. No pre-built indexes required - information is always current.

**Use Claude Code's dedicated tools instead of Bash commands:**

| Purpose | Use This Tool | Not This |
|---------|---------------|----------|
| Find files by pattern | **Glob** | ~~find~~ |
| Search file contents | **Grep** | ~~grep~~ |
| Read file contents | **Read** | ~~cat~~ |

## Resource Locations

All resources are located within Max.app:

```
/Applications/Max.app/Contents/Resources/
├── C74/docs/refpages/           # Object references (XML)
│   ├── max-ref/                 # Max objects (~300)
│   ├── msp-ref/                 # MSP/audio objects (~200)
│   ├── jit-ref/                 # Jitter/video objects (~400)
│   ├── m4l-ref/                 # Max for Live (~50)
│   └── gen-ref/                 # Gen objects (~100)
├── C74/docs/userguide/          # User guides
│   ├── content/                 # Guide content (JSON)
│   └── userguide_search.sqlite  # Full-text search database
├── C74/snippets/                # Code snippets (.maxsnip)
│   ├── max/
│   ├── msp/
│   └── jitter/
└── Examples/                    # Example patches (.maxpat)
    ├── effects/
    ├── synths/
    ├── sequencing/
    └── jitter-examples/
```

## Search Methods Using Claude Code Tools

### 1. Object Reference Lookup

When user asks about a specific object (e.g., "How do I use cycle~?"):

```
# Use Glob to find the reference file
Glob: pattern="**/cycle~.maxref.xml"
      path="/Applications/Max.app/Contents/Resources/C74/docs/refpages"

# Use Read to get the content
Read: file_path="/Applications/Max.app/.../msp-ref/cycle~.maxref.xml"
```

**Reference file naming**: `{object-name}.maxref.xml`
- Objects with `~` suffix: audio/signal objects (MSP)
- Objects without `~`: control/message objects (Max)

### 2. Object Search (Pattern Matching)

When user searches for objects by keyword:

```
# Find objects matching a pattern
Glob: pattern="**/*filter*.maxref.xml"
      path="/Applications/Max.app/Contents/Resources/C74/docs/refpages"

# Search within XML content
Grep: pattern="frequency"
      path="/Applications/Max.app/Contents/Resources/C74/docs/refpages"
      glob="*.maxref.xml"
```

### 3. Full-Text Search (SQLite FTS)

For comprehensive documentation search, use Max's built-in FTS database via helper script:

```bash
./scripts/search-fts.sh "oscillator" 20
```

This uses Max's pre-built SQLite FTS database (no index building required).

### 4. Example Patches

```
# List example categories
Glob: pattern="*"
      path="/Applications/Max.app/Contents/Resources/Examples"

# Find examples containing specific objects
Grep: pattern="cycle~"
      path="/Applications/Max.app/Contents/Resources/Examples"
      glob="*.maxpat"
```

### 5. Snippets

```
# List snippet categories
Glob: pattern="*"
      path="/Applications/Max.app/Contents/Resources/C74/snippets"

# Find snippets
Glob: pattern="**/*.maxsnip"
      path="/Applications/Max.app/Contents/Resources/C74/snippets"
```

## XML Reference Format

Reference files use `.maxref.xml` format. Key elements to extract:

```xml
<c74object name="cycle~" module="msp" category="MSP Synthesis">
    <digest>Sinusoidal oscillator</digest>
    <description>Full description...</description>

    <inletlist>
        <inlet id="0" type="signal/float">
            <digest>Frequency (Hz)</digest>
        </inlet>
    </inletlist>

    <outletlist>
        <outlet id="0" type="signal">
            <digest>Output signal</digest>
        </outlet>
    </outletlist>

    <objarglist>
        <objarg name="frequency" optional="1" type="number">
            <digest>Initial frequency</digest>
        </objarg>
    </objarglist>

    <methodlist>
        <method name="float">...</method>
    </methodlist>

    <attributelist>
        <attribute name="interp" type="int">...</attribute>
    </attributelist>

    <seealsolist>
        <seealso name="phasor~"/>
    </seealsolist>
</c74object>
```

## Key XML Elements to Extract

| Element | Content | Use For |
|---------|---------|---------|
| `<digest>` | One-line summary | Quick overview |
| `<description>` | Full description | Detailed explanation |
| `<inletlist>/<inlet>` | Input specs | Understanding inputs |
| `<outletlist>/<outlet>` | Output specs | Understanding outputs |
| `<objarglist>/<objarg>` | Creation arguments | Object instantiation |
| `<methodlist>/<method>` | Available methods | Messages the object accepts |
| `<attributelist>/<attribute>` | Object attributes | Configurable properties |
| `<seealsolist>/<seealso>` | Related objects | Finding alternatives |

## Helper Scripts (Optional)

Lightweight helper scripts for operations that require Bash:

| Script | Purpose | When to Use |
|--------|---------|-------------|
| `search-fts.sh` | SQLite FTS query | Full-text documentation search |
| `get-reference.sh` | Get object reference | Convenient summary extraction |
| `list-examples.sh` | List example patches | Browse examples |
| `get-snippet.sh` | Get code snippets | Browse snippets |

**Note**: Prefer using Glob/Grep/Read tools directly. Use scripts only when SQLite access is needed.

## When to Use

Use this skill when users ask about:
- Max object usage ("How do I use cycle~?")
- Object parameters, inlets, outlets, methods
- Example patches ("Show me FM synthesis examples")
- Snippets and code patterns
- Max concepts and tutorials

**Trigger words**: "Max object", "reference", "how to use", "example", "snippet", "documentation", "inlet", "outlet", "method"

## Workflow Examples

### Example 1: Object Lookup

User: "How do I use metro?"

1. **Glob** to find: `**/metro.maxref.xml`
2. **Read** the XML file
3. Extract digest, description, inlets, outlets
4. Present formatted information

### Example 2: Search for Objects

User: "What filter objects are available?"

1. **Glob** to search: `**/*filter*.maxref.xml`
2. List matching objects
3. **Read** top results to extract digests
4. Present categorized list

### Example 3: Find Examples

User: "Show me reverb examples"

1. **Grep** in Examples directory for "reverb"
2. Or **Glob** for `**/*reverb*.maxpat`
3. List matching patches with paths

## Max.app Path Detection

If Max.app is not at the default location, use Bash to detect:

```bash
mdfind "kMDItemCFBundleIdentifier == 'com.cycling74.Max'" | head -1
```

## Detailed Documentation

For format specifications:
- `references/resource-paths.md` - Full path reference
- `references/refpage-format.md` - XML format details
- `references/maxpat-format.md` - Patch JSON format
- `references/mcp-recreation.md` - Example-to-patch workflow
- `examples/lookup-object.md` - Lookup examples
- `examples/recreate-patch.md` - Patch recreation examples
