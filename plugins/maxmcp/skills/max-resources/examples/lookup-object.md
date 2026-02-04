# Object Lookup Examples

## Example 1: Basic Object Lookup

**User request**: "How do I use cycle~?"

### Step 1: Find the reference file

```bash
find /Applications/Max.app/Contents/Resources/C74/docs/refpages \
    -name "cycle~.maxref.xml" -type f
```

Output:
```
/Applications/Max.app/Contents/Resources/C74/docs/refpages/msp-ref/cycle~.maxref.xml
```

### Step 2: Get summary (using helper script)

```bash
./scripts/get-reference.sh cycle~ --summary
```

Output:
```
Object: cycle~
Category: msp-ref
Path: /Applications/Max.app/.../msp-ref/cycle~.maxref.xml
---

Digest:
Sinusoidal oscillator

Description:
Use the cycle~ object to generate a sinusoidal waveform.
The object uses a 512-sample wavetable which can be
replaced by loading an audio file.
```

### Step 3: Full reference (if needed)

```bash
./scripts/get-reference.sh cycle~
```

Returns complete XML with inlets, outlets, methods, attributes.

## Example 2: Finding Filter Objects

**User request**: "What filter objects are available in MSP?"

### Direct filesystem search

```bash
find /Applications/Max.app/Contents/Resources/C74/docs/refpages/msp-ref \
    -name "*filter*.maxref.xml" -type f
```

Or list all MSP objects and grep:

```bash
ls /Applications/Max.app/Contents/Resources/C74/docs/refpages/msp-ref/*.maxref.xml | \
    xargs -n1 basename | sed 's/.maxref.xml//' | grep -i filter
```

Output:
```
biquad~
filtercoeff~
filtergraph~
lores~
reson~
svf~
```

## Example 3: Search by Keyword

**User request**: "Looking for something to generate random numbers"

### Search filenames

```bash
find /Applications/Max.app/Contents/Resources/C74/docs/refpages \
    -name "*random*.maxref.xml" -o -name "*rand*.maxref.xml" -o -name "*noise*.maxref.xml"
```

### Search within XML content

```bash
grep -r "random" /Applications/Max.app/Contents/Resources/C74/docs/refpages \
    --include="*.maxref.xml" -l | head -10
```

## Example 4: Full-Text Search

**User request**: "How do I do FM synthesis?"

```bash
./scripts/search-fts.sh "FM synthesis"
```

Output:
```
Full-text search: FM synthesis
---
title                path                          excerpt
-----------------    --------------------------    ----------------
FM Synthesis         /userguide/audio/fm          ...frequency >>>modulation<<< creates...
Oscillators          /userguide/audio/oscillators ...used for >>>FM synthesis<<<...
```

## Example 5: Finding Related Objects

After reading a reference file, look at `<seealsolist>`:

```bash
./scripts/get-reference.sh cycle~
```

Extract related objects:
```xml
<seealsolist>
    <seealso name="phasor~"/>
    <seealso name="wave~"/>
    <seealso name="buffer~"/>
    <seealso name="cos~"/>
</seealsolist>
```

Then get related references:

```bash
./scripts/get-reference.sh phasor~ --summary
./scripts/get-reference.sh wave~ --summary
```

## Example 6: Understanding Object Parameters

**User request**: "What arguments does metro take?"

```bash
./scripts/get-reference.sh metro
```

Look at `<objarglist>`:
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

```bash
./scripts/get-reference.sh biquad~
```

Look at `<inletlist>`:
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

Or extract directly:

```bash
grep -A2 '<inlet' /Applications/Max.app/.../msp-ref/biquad~.maxref.xml
```

## Example 8: List All Objects in a Category

**User request**: "What Max objects are available?"

```bash
# List all Max control objects
ls /Applications/Max.app/Contents/Resources/C74/docs/refpages/max-ref/*.maxref.xml | \
    wc -l
# Output: ~300

# List first 20
ls /Applications/Max.app/Contents/Resources/C74/docs/refpages/max-ref/*.maxref.xml | \
    head -20 | xargs -n1 basename | sed 's/.maxref.xml//'
```

## Summary

| Task | Method | Example |
|------|--------|---------|
| Find object by name | `find` or helper | `./scripts/get-reference.sh delay~` |
| Search by pattern | `find -name "*pattern*"` | `find ... -name "*filter*"` |
| Search content | `grep -r` | `grep -r "oscillator" .../refpages` |
| Search documentation | `search-fts.sh` | `./scripts/search-fts.sh "envelope"` |
| List category | `ls` | `ls .../refpages/msp-ref/` |
