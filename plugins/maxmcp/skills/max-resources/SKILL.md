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

This skill uses **direct filesystem exploration** instead of pre-built indexes. The AI dynamically searches Max.app resources as needed, ensuring information is always current and eliminating cache maintenance.

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

## Search Methods

### 1. Object Reference Lookup (Direct)

When user asks about a specific object (e.g., "How do I use cycle~?"):

```bash
# Find the reference file
find /Applications/Max.app/Contents/Resources/C74/docs/refpages \
    -name "cycle~.maxref.xml" -type f

# Read the XML file directly
cat /Applications/Max.app/.../refpages/msp-ref/cycle~.maxref.xml
```

**Reference file naming**: `{object-name}.maxref.xml`
- Objects with `~` suffix: audio/signal objects (MSP)
- Objects without `~`: control/message objects (Max)

### 2. Object Search (Pattern Matching)

When user searches for objects by keyword:

```bash
# Find objects matching a pattern
find /Applications/Max.app/Contents/Resources/C74/docs/refpages \
    -name "*filter*.maxref.xml" -type f

# Search within XML content (grep)
grep -r "frequency" /Applications/Max.app/.../refpages/ --include="*.maxref.xml" -l
```

### 3. Full-Text Search (SQLite FTS)

For comprehensive documentation search, use Max's built-in FTS database:

```bash
# Helper script available
./scripts/search-fts.sh "oscillator" 20

# Or direct SQLite query
sqlite3 "/Applications/Max.app/.../userguide/userguide_search.sqlite" \
    "SELECT title, path, snippet(pages_fts, -1, '**', '**', '...', 30)
     FROM pages_fts WHERE pages_fts MATCH 'oscillator' LIMIT 20"
```

### 4. Example Patches

```bash
# List example categories
ls /Applications/Max.app/Contents/Resources/Examples/

# Find examples by keyword
find /Applications/Max.app/Contents/Resources/Examples \
    -name "*.maxpat" | xargs grep -l "cycle~"

# Or use helper
./scripts/list-examples.sh synths
```

### 5. Snippets

```bash
# List snippet categories
ls /Applications/Max.app/Contents/Resources/C74/snippets/

# Find snippets
find /Applications/Max.app/Contents/Resources/C74/snippets \
    -name "*.maxsnip" | head -20

# Or use helper
./scripts/get-snippet.sh msp
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

## Quick Extraction Commands

```bash
# Get object digest (one-line summary)
grep -o '<digest>[^<]*</digest>' file.maxref.xml | head -1 | sed 's/<[^>]*>//g'

# Get inlets
grep -A2 '<inlet' file.maxref.xml

# Get outlets
grep -A2 '<outlet' file.maxref.xml

# Get methods
grep -o '<method name="[^"]*"' file.maxref.xml | sed 's/.*name="//;s/"//'

# Get attributes
grep -o '<attribute name="[^"]*"' file.maxref.xml | sed 's/.*name="//;s/"//'

# Get related objects
grep -o '<seealso name="[^"]*"' file.maxref.xml | sed 's/.*name="//;s/"//'
```

## Helper Scripts

Lightweight helper scripts for common operations:

| Script | Purpose | Usage |
|--------|---------|-------|
| `search-fts.sh` | Full-text search via SQLite | `./scripts/search-fts.sh "query" [limit]` |
| `get-reference.sh` | Get object reference | `./scripts/get-reference.sh cycle~ [--summary]` |
| `list-examples.sh` | List example patches | `./scripts/list-examples.sh [category] [search]` |
| `get-snippet.sh` | Get code snippets | `./scripts/get-snippet.sh [category] [search]` |

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

1. Find reference: `find ... -name "metro.maxref.xml"`
2. Read the XML file
3. Extract digest, description, inlets, outlets
4. Present formatted information

### Example 2: Search for Objects

User: "What filter objects are available?"

1. Search: `find ... -name "*filter*.maxref.xml"`
2. List matching objects
3. For top results, extract digests
4. Present categorized list

### Example 3: Find Examples

User: "Show me reverb examples"

1. Search examples: `find .../Examples -name "*reverb*" -o -name "*verb*"`
2. Or grep inside patches: `grep -r "reverb" .../Examples --include="*.maxpat" -l`
3. List matching patches with paths

## Max.app Path Detection

If Max.app is not at the default location:

```bash
# Find Max.app via Spotlight
mdfind "kMDItemCFBundleIdentifier == 'com.cycling74.Max'" | head -1

# Get version
/usr/libexec/PlistBuddy -c "Print CFBundleShortVersionString" \
    /Applications/Max.app/Contents/Info.plist
```

## Detailed Documentation

For format specifications:
- `references/resource-paths.md` - Full path reference
- `references/refpage-format.md` - XML format details
- `references/maxpat-format.md` - Patch JSON format
- `references/mcp-recreation.md` - Example-to-patch workflow
- `examples/lookup-object.md` - Lookup examples
- `examples/recreate-patch.md` - Patch recreation examples
