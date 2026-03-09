# Architecture Patterns for Max/MSP Patches

Common structural patterns found in production Max patches, useful as templates when building new patches with MaxMCP.

## Subpatcher Patterns

### poly~ Voice Subpatcher

Standard structure for polyphonic voice patches used inside `poly~`:

```
in                          ← Data from parent poly~
  ↓
zl / trigger                ← Parse & branch incoming data
  ↓
unpack                      ← Extract parameters (pitch, velocity, duration, etc.)
  ├─→ adsr~ + thispoly~    ← Envelope generator + voice management (left column)
  └─→ route                ← Parameter demux for per-voice routing (center)
       ↓
wave~ / cycle~ / play~      ← Sound source (right column)
  ↓
*~ or times~                ← Apply envelope as gain
  ↓
out~ × 2                   ← Stereo output to parent
```

**Key conventions**:
- `in` at top, `out~` at bottom
- `thispoly~` near `adsr~` for mute-on-release (`adsr~` outlet 1 → `thispoly~` inlet 0)
- Use `loadmess` to set initial `thispoly~` state (e.g., steal mode)

### Column Separation in Subpatchers

Separate **control/message** logic from **audio signal** processing:

```
Left Column (x≈50)          Right Column (x≈600+)
───────────────────         ──────────────────────
in, zl, trigger             receive, int, trigger
drunk, delay                -, *, <, select
unpack, route               wave~, line~, pack
adsr~, thispoly~            times~ × 2, out~ × 2
```

**Cross-column connections** (control → audio) use **L-shape midpoints** to keep the layout clean. Place the horizontal segment near the destination when the vertical distance is large.

## Signal Flow Patterns

### Route Demultiplexer

Distribute parameters from a single data stream to multiple processing paths:

```
unpack (x=46, y=404)
  ↓ (outlet 1)
route 0 1 2 3 4 5 6 7 8 (x=369, y=616)
  │ │ │ │ │ │ │ │ │
  ↓ ↓ ↓ ↓ ↓ ↓ ↓ ↓ ↓     ← 9 outlets
  t t t t t t t t t        ← trigger objects at same Y (y=649)
  └─┴─┴─┴─┴─┼─┴─┴─┴─┘
             ↓
         - (y=718)         ← convergence point
```

**Layout rules**:
- All `trigger` objects at the **same Y** (fan-out row)
- Equal horizontal spacing between triggers (~24px)
- Diagonal fan-in lines to the convergence point are **acceptable** (no midpoints needed) because the visual pattern clearly shows convergence

### Stereo Mirroring

Duplicate identical processing chains for L/R channels:

```
Left channel (x≈636)    Right channel (x≈836)
────────────────────    ─────────────────────
trigger                 trigger
  ↓                       ↓
< (compare)             < (compare)
  ↓                       ↓
select                  select
  ↓                       ↓
trigger                 trigger
  ↓                       ↓
int                     int
```

**Convention**: ~200px horizontal offset between L/R columns. Both chains receive from the same source via fan-out.

### Destination-Near L-Shape

When a connection spans a large vertical distance (e.g., envelope to output gain), place the L-shape horizontal segment **near the destination** rather than near the source:

```
adsr~ (x=46, y=496)
  │
  │  (long vertical run, 888px)
  │
  M1 (56, 1406) ──→ M2 (656, 1406)    ← horizontal near dest
                          │
                          ↓
                    times~ (x=636, y=1430)   ← 24px below horizontal
```

**When to use**: Source-to-destination distance > 500px. Placing the horizontal segment near the source would create an unnecessarily long diagonal approach to the destination.

**Spacing**: Place horizontal segment 20-30px above the destination inlet.

## Connection Patterns

### Fan-In Convergence (No Midpoints)

When many sources at the same Y converge to a single destination, diagonal lines are acceptable and even preferred over midpoints:

```
t(x=369) t(x=393) t(x=417) ... t(x=561)   ← same Y
  \         \         \            /
   \         \         \          /
    \         \         \        /
     ─────────────→ - (x=646)              ← single destination
```

**Rule**: Fan-in from objects at the same Y to a single inlet creates a **visual funnel** that is immediately readable. Adding midpoints to each line would clutter the patch.

**Threshold**: Acceptable up to ~10 converging lines with dx up to ~280px.

### Long-Distance Envelope Application

A common pattern where an envelope generator in the control column must reach the audio output in a different column:

```
adsr~ (control column, x≈50)
  │
  ├─→ thispoly~ (same column)       ← outlet 1: voice management
  │
  └─→ times~ L (x≈636, y≈1430)     ← outlet 0: L-shape to audio column
  └─→ times~ R (x≈836, y≈1430)     ← outlet 0: L-shape to audio column
```

Both L-shape connections share the **same horizontal Y** (e.g., y=1406), creating parallel horizontal segments that are easy to follow.

## Large Patch Patterns

### Staircase Delay Cascade (Boot Sequence)

Sequential initialization pattern using chained `delay` → `trigger` → `send` triplets, each offset down-right from the previous:

```
loadbang (x=1850, y=2264)
  ↓
delay → trigger (x=1850, y=2296)
  ├─→ send (long vertical, y≈3189)    ← init message 1
  ├─→ textbutton (nearby)             ← visual indicator
  └─→ next delay (x=1905, y=2351)     ← +55px right, +54px down
       ├─→ send (y≈3142)              ← init message 2
       └─→ next delay (x=1953, y=2406)
            └─→ ... (continues for N stages)
```

**When to use**: Startup initialization that must send messages in a specific order with timing delays. Each stage waits for its `delay` before firing, ensuring dependent subsystems initialize sequentially.

**Layout rules**:
- Each triplet offsets ~50px right and ~54px down (staircase visual)
- `send` targets are far below (dy > 500px), connected via long vertical straight lines
- `textbutton` indicators sit near each `send` target for visual confirmation
- Typically 4-8 stages in production patches

### pattr Parameter Grid

Organized grid layout for `pattrstorage`-based parameter management:

```
Column 1 (x≈1607)    Column 2 (x≈1973)    Column 3 (x≈2348)    Column 4 (x≈2685)
──────────────────    ──────────────────    ──────────────────    ──────────────────
  pattr (binding)       pattr (binding)       pattr (binding)       pattr (binding)
  prepend "set"         prepend "set"         prepend "set"         prepend "set"
  number (UI)           number (UI)           number (UI)           number (UI)
  comment (label)       comment (label)       comment (label)       comment (label)
  ↓                     ↓                     ↓                     ↓
  pattr (binding)       pattr (binding)       pattr (binding)       pattr (binding)
  ...                   ...                   ...                   ...
```

**Structure**: Each column contains vertically stacked `pattr` → `prepend` → `number` → `comment` groups. A single `pattrstorage` object (placed separately) binds all `pattr` objects.

**Layout rules**:
- Columns spaced ~340-370px apart
- Each parameter group: `pattr` (y) → `prepend` (y+30) → `number` (y+60)
- `comment` labels placed beside `number` objects for identification
- Multiple rows per column, with ~150px vertical spacing between parameter groups
- `pattrstorage` placed in the processing section, separate from the parameter grid

### send/receive for Cross-Section Communication

In large patches (300+ objects), use `send`/`receive` pairs instead of long patchcords to connect distant sections:

```
Section A (y≈100)              Section B (y≈1800)
─────────────────              ─────────────────
  toggle                         receive "enable"
    ↓                              ↓
  send "enable"                  gate
                                   ↓
                                 processing...
```

**When to use**:
- Patch width > 1500px or height > 2000px
- Connection would cross 3+ unrelated sections
- Same signal needed by multiple distant destinations

**Naming convention**: Use descriptive names that indicate the signal's purpose (e.g., `"enable"`, `"tempo"`, `"filepath"`).

**Trade-off**: `send`/`receive` makes signal flow invisible — the connection cannot be traced visually. Mitigate by:
- Placing `comment` objects near each `receive` noting the source
- Using consistent naming across the patch
- Limiting to genuinely distant connections (don't use for short distances where a patchcord would be clearer)
