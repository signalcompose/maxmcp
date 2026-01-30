# Max Reference Page Format (XML)

Reference pages use `.maxref.xml` format with structured metadata about each object.

## File Location Pattern

```
/Applications/Max.app/Contents/Resources/C74/docs/refpages/{category}/{object}.maxref.xml
```

Categories:
- `max-ref` - Max objects
- `msp-ref` - MSP (audio) objects
- `jit-ref` - Jitter objects
- `m4l-ref` - Max for Live objects
- `gen-ref` - Gen objects

## XML Structure

### Complete Example: cycle~

```xml
<?xml version="1.0" encoding="utf-8" standalone="yes"?>
<?xml-stylesheet href="./_c74_ref.xsl" type="text/xsl"?>

<c74object name="cycle~" module="msp" category="MSP Synthesis">
    <digest>Sinusoidal oscillator</digest>

    <description>
        Use the cycle~ object to generate a sinusoidal waveform.
        The object uses a 512-sample wavetable which can be
        replaced by loading an audio file.
    </description>

    <discussion>
        cycle~ uses linear interpolation for its wavetable lookup.
        For higher-quality output, consider using wave~.
    </discussion>

    <metadatalist>
        <metadata name="author">Cycling '74</metadata>
        <metadata name="tag">MSP</metadata>
        <metadata name="tag">MSP Synthesis</metadata>
    </metadatalist>

    <inletlist>
        <inlet id="0" type="signal/float">
            <digest>Frequency (Hz) or signal</digest>
            <description>
                Sets the frequency when a float is received.
                When connected to a signal, the frequency is
                modulated at signal rate.
            </description>
        </inlet>
        <inlet id="1" type="signal/float">
            <digest>Phase offset (0-1)</digest>
            <description>
                Sets the phase offset of the wavetable lookup.
            </description>
        </inlet>
    </inletlist>

    <outletlist>
        <outlet id="0" type="signal">
            <digest>Sinusoidal output signal</digest>
            <description>
                Outputs a sinusoidal signal ranging from -1 to 1.
            </description>
        </outlet>
    </outletlist>

    <objarglist>
        <objarg name="frequency" optional="1" type="number">
            <digest>Initial frequency in Hz</digest>
            <description>
                Sets the initial frequency. Default is 0 Hz.
            </description>
        </objarg>
        <objarg name="buffer-name" optional="1" type="symbol">
            <digest>Buffer to use as wavetable</digest>
            <description>
                Optional buffer~ name to use instead of the
                built-in 512-sample sine wave.
            </description>
        </objarg>
    </objarglist>

    <methodlist>
        <method name="float">
            <arglist>
                <arg name="frequency" type="float" />
            </arglist>
            <digest>Set frequency</digest>
            <description>
                A float in the left inlet sets the frequency in Hz.
            </description>
        </method>
        <method name="set">
            <arglist>
                <arg name="buffer-name" type="symbol" />
            </arglist>
            <digest>Set buffer name</digest>
            <description>
                The set message followed by a buffer name
                loads a new wavetable.
            </description>
        </method>
    </methodlist>

    <attributelist>
        <attribute name="interp" get="1" set="1" type="int" size="1">
            <digest>Interpolation mode</digest>
            <description>
                0 = none, 1 = linear (default), 2 = cubic
            </description>
        </attribute>
    </attributelist>

    <seealsolist>
        <seealso name="phasor~"/>
        <seealso name="wave~"/>
        <seealso name="buffer~"/>
        <seealso name="cos~"/>
    </seealsolist>
</c74object>
```

## Key Elements

### Essential Elements

| Element | Description |
|---------|-------------|
| `<c74object>` | Root element with `name`, `module`, `category` |
| `<digest>` | One-line description |
| `<description>` | Full description |
| `<inletlist>` | Input specifications |
| `<outletlist>` | Output specifications |
| `<objarglist>` | Creation arguments |
| `<methodlist>` | Available methods |

### Optional Elements

| Element | Description |
|---------|-------------|
| `<discussion>` | Additional technical notes |
| `<metadatalist>` | Tags and author info |
| `<attributelist>` | Object attributes |
| `<seealsolist>` | Related objects |
| `<misc>` | Additional notes |

## Parsing Examples

### Extract digest and description (bash)

```bash
grep -o '<digest>[^<]*</digest>' cycle~.maxref.xml | sed 's/<[^>]*>//g'
# Output: Sinusoidal oscillator

sed -n '/<description>/,/<\/description>/p' cycle~.maxref.xml | sed 's/<[^>]*>//g'
```

### Extract inlets/outlets (using xmllint)

```bash
xmllint --xpath "//inlet" cycle~.maxref.xml
xmllint --xpath "//outlet" cycle~.maxref.xml
```

### Extract for MCP tool usage

Key information to extract for `add_max_object`:
1. Object name (`<c74object name="...">`)
2. Required arguments (`<objarglist>` where `optional="0"`)
3. Default values (from `<description>`)

Key information for `set_object_attribute`:
1. Attribute names (`<attribute name="...">`)
2. Attribute types (`type="..."`)
3. Valid values (from `<description>`)
