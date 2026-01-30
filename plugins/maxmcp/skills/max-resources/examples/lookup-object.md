# Object Lookup Examples

## Example 1: Basic Object Search

**User request**: "How do I use cycle~?"

### Step 1: Search for the object

```bash
./scripts/search-objects.sh cycle
```

Output:
```
Searching for: cycle

[msp-ref]
cycle~
```

### Step 2: Get reference

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

```bash
./scripts/search-objects.sh filter msp-ref
```

Output:
```
Searching for: filter
Category: msp-ref
---
biquad~
filtercoeff~
filtergraph~
lores~
reson~
svf~
onepole~
```

## Example 3: Fuzzy Search

**User request**: "Looking for something to generate random numbers"

```bash
./scripts/search-objects.sh random
```

Output:
```
[max-ref]
random
drunk
urn

[msp-ref]
rand~
noise~
pink~
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

After getting a reference:

```bash
./scripts/get-reference.sh cycle~
```

Look at `<seealsolist>`:
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

## Example 8: Cache Management

### First-time setup

```bash
./scripts/check-cache.sh
# Output: NEEDS_BUILD

./scripts/build-index.sh
# Output: Index built successfully!
#   Total objects: 1175
#   Max version: 9.0.5
```

### After Max update

```bash
./scripts/check-cache.sh
# Output: VERSION_MISMATCH
#   Cached version: 9.0.4
#   Current version: 9.0.5

./scripts/build-index.sh
# Rebuilds cache
```

## Summary

| Task | Script | Example |
|------|--------|---------|
| Find object by name | `search-objects.sh` | `./scripts/search-objects.sh delay` |
| Get object reference | `get-reference.sh` | `./scripts/get-reference.sh delay~` |
| Search documentation | `search-fts.sh` | `./scripts/search-fts.sh "envelope"` |
| Check cache | `check-cache.sh` | `./scripts/check-cache.sh` |
| Build cache | `build-index.sh` | `./scripts/build-index.sh` |
