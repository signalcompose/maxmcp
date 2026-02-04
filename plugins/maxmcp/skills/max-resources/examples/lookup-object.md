# Object Lookup Examples

## Tool Usage

**Always prefer Claude Code's dedicated tools over Bash commands:**

| Task | Tool | Example |
|------|------|---------|
| Find files | **Glob** | `pattern="**/cycle~.maxref.xml"` |
| Search content | **Grep** | `pattern="oscillator" glob="*.maxref.xml"` |
| Read files | **Read** | `file_path="/path/to/file.xml"` |

## Example 1: Basic Object Lookup

**User request**: "How do I use cycle~?"

### Step 1: Find the reference file

```
Glob: pattern="**/cycle~.maxref.xml"
      path="/Applications/Max.app/Contents/Resources/C74/docs/refpages"
```

Result:
```
/Applications/Max.app/Contents/Resources/C74/docs/refpages/msp-ref/cycle~.maxref.xml
```

### Step 2: Read the reference

```
Read: file_path="/Applications/Max.app/Contents/Resources/C74/docs/refpages/msp-ref/cycle~.maxref.xml"
```

### Step 3: Extract key information

From the XML, extract:
- `<digest>` - One-line summary
- `<description>` - Full description
- `<inletlist>` - Inputs
- `<outletlist>` - Outputs
- `<objarglist>` - Creation arguments

## Example 2: Finding Filter Objects

**User request**: "What filter objects are available in MSP?"

### Using Glob for pattern search

```
Glob: pattern="**/*filter*.maxref.xml"
      path="/Applications/Max.app/Contents/Resources/C74/docs/refpages/msp-ref"
```

Result:
```
filtercoeff~.maxref.xml
filtergraph~.maxref.xml
```

### Alternative: List all and filter conceptually

```
Glob: pattern="*.maxref.xml"
      path="/Applications/Max.app/Contents/Resources/C74/docs/refpages/msp-ref"
```

Then identify filter-related objects: biquad~, lores~, reson~, svf~, onepole~

## Example 3: Search by Keyword in Content

**User request**: "Looking for objects related to random numbers"

### Search within XML content

```
Grep: pattern="random"
      path="/Applications/Max.app/Contents/Resources/C74/docs/refpages"
      glob="*.maxref.xml"
      output_mode="files_with_matches"
```

Result includes files mentioning "random" in their documentation.

## Example 4: Full-Text Search

**User request**: "How do I do FM synthesis?"

Use the helper script for SQLite FTS:

```bash
./scripts/search-fts.sh "FM synthesis"
```

Output:
```
Full-text search: FM synthesis
---
title                path                          excerpt
-----------------    --------------------------    ----------------
FM Synthesis         /userguide/audio/fm          ...frequency modulation...
```

## Example 5: Finding Related Objects

After reading a reference, look at `<seealsolist>`:

```xml
<seealsolist>
    <seealso name="phasor~"/>
    <seealso name="wave~"/>
    <seealso name="buffer~"/>
    <seealso name="cos~"/>
</seealsolist>
```

Then look up related objects:

```
Glob: pattern="**/phasor~.maxref.xml"
      path="/Applications/Max.app/Contents/Resources/C74/docs/refpages"
```

## Example 6: Understanding Object Parameters

**User request**: "What arguments does metro take?"

```
Glob: pattern="**/metro.maxref.xml"
      path="/Applications/Max.app/Contents/Resources/C74/docs/refpages"
```

Then Read the file and extract `<objarglist>`:

```xml
<objarglist>
    <objarg name="interval" optional="1" type="number">
        <digest>Time interval in ms</digest>
    </objarg>
</objarglist>
```

And `<methodlist>` for messages:

```xml
<methodlist>
    <method name="int">
        <digest>Set interval and start</digest>
    </method>
    <method name="bang">
        <digest>Start metro</digest>
    </method>
    <method name="stop">
        <digest>Stop metro</digest>
    </method>
</methodlist>
```

## Example 7: Checking Inlets and Outlets

**User request**: "How many inlets does biquad~ have?"

```
Read: file_path="/Applications/Max.app/.../msp-ref/biquad~.maxref.xml"
```

Extract `<inletlist>`:

```xml
<inletlist>
    <inlet id="0" type="signal">
        <digest>Signal input</digest>
    </inlet>
    <inlet id="1" type="signal/float">
        <digest>a0 coefficient</digest>
    </inlet>
    <!-- ... more inlets ... -->
</inletlist>
```

## Example 8: List All Objects in a Category

**User request**: "What Max objects are available?"

```
Glob: pattern="*.maxref.xml"
      path="/Applications/Max.app/Contents/Resources/C74/docs/refpages/max-ref"
```

Returns all Max control objects (~300 files).

## Summary

| Task | Tool | Pattern Example |
|------|------|-----------------|
| Find object by name | Glob | `**/delay~.maxref.xml` |
| Search by pattern | Glob | `**/*filter*.maxref.xml` |
| Search content | Grep | `pattern="oscillator" glob="*.maxref.xml"` |
| Read reference | Read | Direct file path |
| Full-text search | Bash | `./scripts/search-fts.sh "query"` |
| List category | Glob | `*.maxref.xml` in category dir |
