# Layout Rules for Max/MSP Patches

Detailed positioning and spacing guidelines for creating organized, readable patches.

## Grid System

### Base Grid

Use a consistent grid for all object placement:

- **Grid unit**: 20 pixels
- **Object alignment**: Snap to grid for consistency
- **Minimum spacing**: 1 grid unit (20px) between objects

### Standard Positions

```
Column 1: x = 50
Column 2: x = 170
Column 3: x = 290
Column 4: x = 410

Row 1: y = 50
Row 2: y = 100
Row 3: y = 150
...
```

## Object Dimensions

### Standard Object Sizes

| Object Type | Typical Width | Height |
|-------------|---------------|--------|
| Basic object (button, toggle) | 20-30 | 20 |
| Text object (message, number) | 50-100 | 20 |
| Signal object (cycle~, *~) | 60-80 | 20 |
| UI object (slider, dial) | 30-50 | 80-140 |
| Subpatcher (p, bpatcher) | 100-200 | 60-100 |
| Comment | Variable | 20 |

### Width Calculation

For objects with arguments, estimate width:
- Base width: 40px
- Per character in arguments: ~7px
- Example: `cycle~ 440` ≈ 40 + (10 × 7) = 110px

## Spacing Guidelines

### Horizontal Spacing

- **Between columns**: 100-120px (center to center)
- **Parallel paths**: 100px minimum
- **Related objects**: 80px
- **Signal chains**: 100px

### Vertical Spacing

- **Between rows**: 40-60px
- **After comments**: 30px
- **Section breaks**: 80-100px
- **Before/after subpatchers**: 60px

## Section Organization

### Standard Patch Layout

```
┌─────────────────────────────────────────┐
│  INPUTS & PARAMETERS (y: 50-150)        │
│  ├── Audio inputs (adc~)                │
│  ├── MIDI inputs (midiin)               │
│  └── User controls (sliders, dials)     │
├─────────────────────────────────────────┤
│  PROCESSING (y: 200-400)                │
│  ├── Signal processing                  │
│  ├── MIDI processing                    │
│  └── Logic/routing                      │
├─────────────────────────────────────────┤
│  MODULATION & CONTROL (y: 450-550)      │
│  ├── LFOs, envelopes                    │
│  └── Automation                         │
├─────────────────────────────────────────┤
│  OUTPUTS (y: 600-700)                   │
│  ├── Audio outputs (dac~)               │
│  ├── MIDI outputs (midiout)             │
│  └── Displays (scope~, meter~)          │
└─────────────────────────────────────────┘
```

## Patchcord Management

### Principle: Vertical Straight Lines First

The most readable patches keep patchcords as **vertical straight lines**. Achieve this by aligning the source outlet and destination inlet on the same X coordinate.

**Example**:
```
live.button (x=18)     trigger (x=18)
  outlet (x=27.5)       inlet (x=27.5)   ← same X = straight line
       │
       ↓  (vertical, no midpoints needed)
```

**Rule**: Before adding midpoints, first try repositioning objects so that outlet and inlet share the same X coordinate.

### Width Adjustment for Straight Lines

When two objects have multiple inlets/outlets connected (e.g., `filtercoeff~` → `biquad~`), adjust **object width** via `patching_rect` so that outlet and inlet X coordinates align, eliminating diagonal patchcords.

**Priority**: Align the **rightmost connected inlet** first, then adjust the source object's width to match.

```
filtercoeff~  (width adjusted to align rightmost outlet with biquad~'s rightmost inlet)
  │  │  │  │  │
  ↓  ↓  ↓  ↓  ↓   ← all vertical straight lines
biquad~
```

### Multi-Connection Alignment Rules

When multiple patchcords converge or diverge, align connected objects by Y coordinate.

#### Multiple Outlets → One Inlet

Place all source objects at the **same Y position**:

```
  objA (y=200)    objB (y=200)    objC (y=200)   ← same Y
    │                │                │
    └────────────────┼────────────────┘
                     ↓
                  dest (y=260)
```

#### One Outlet → Multiple Inlets

Place all destination objects at the **same Y position**:

```
                source (y=200)
                     │
            ┌────────┼────────┐
            ↓        ↓        ↓
  objA (y=260)  objB (y=260)  objC (y=260)   ← same Y
```

#### Multiple Outlets → Multiple Inlets

Place all source objects at the **same Y** and all destination objects at the **same Y**. Use **staggered horizontal lanes** (L-shape midpoints) to separate each source's non-direct connections at distinct Y levels:

```
  int1 (y=439)     int2 (y=439)     int3 (y=439)      ← sources: same Y
    │                 │                 │
    │ int1 lane ──────│─────────────────│── y=476
    │                 │ int2 lane ──────│── y=500
    │                 │                 │ int3 lane ── y=518
    ↓                 ↓                 ↓
    a (y=535)    b (y=535)    c (y=535)    d (y=535)    ← dests: same Y
```

**Rules**:
- Direct connections (same X for outlet and inlet) use **straight vertical lines** with no midpoints
- Non-direct connections use **L-shape midpoints** with a horizontal segment
- Each source gets a **unique horizontal lane Y** to prevent overlapping patchcords
- Lane Y values are staggered between source Y and destination Y (e.g., source_y+17, source_y+41, source_y+59 for 3 sources across a 76px gap)

**Example**:
```
int1→a: straight (same X=884.5)
int1→c: L-shape at y=476   ← int1's lane
int2→c: straight (same X=960.5)
int2→b: L-shape at y=502   ← int2's lane
int3→d: straight (same X=1010.5)
int3→a: L-shape at y=518   ← int3's lane
```

### Midpoint Routing Patterns

Use `set_patchline_midpoints` to route patchcords around obstacles when straight lines are not possible.

#### Pattern 1: L-Shape (2 midpoints)

For redirecting a connection to an adjacent column without crossing objects:

```
Source outlet (x=97.5, y=476)
       │
       ↓
   M1 (97.5, 488) ──→ M2 (27.5, 488)
                            │
                            ↓
                   Dest inlet (27.5, 520)
```

**When to use**: One object's outlet needs to reach another column's inlet. Route horizontally at a consistent Y, then drop vertically.

```javascript
set_patchline_midpoints({
  midpoints: [
    { x: 97.5, y: 488 },   // horizontal from source
    { x: 27.5, y: 488 }    // turn corner toward dest
  ]
})
```

**Spacing rule**: Place the horizontal segment ~12px below the source outlet (`source_y + 12`).

#### Pattern 2: U-Shape (4 midpoints)

For feedback loops or long-distance return paths that must avoid crossing the main signal flow:

```
Source outlet (x=27.5, y=984)
       │
       ↓
   M1 (27.5, 1001) ──→ M2 (-1.5, 1001)
                            │
                            ↑  (travels upward outside patch)
                            │
                        M3 (-1.5, 509)
                            │
                            ↓
   M4 (49.5, 509)  ←──────┘
       │
       ↓
   Dest inlet (49.5, 520)
```

**When to use**: A feedback line must travel a long distance upward. Route **outside** the main patch area (negative X or far right) to keep the interior clean.

```javascript
set_patchline_midpoints({
  midpoints: [
    { x: 27.5, y: 1001 },   // down from source
    { x: -1.5, y: 1001 },   // move to outside lane
    { x: -1.5, y: 509 },    // travel up in outside lane
    { x: 49.5, y: 509 }     // re-enter toward dest
  ]
})
```

**Spacing rules**:
- Outside lane X: Use negative X (e.g., `-1.5`) for left-side routing, or `rightmost_object_x + 20` for right-side
- Vertical clearance: Place horizontal segments ~12-17px beyond the source/dest objects

### Routing Decision Flowchart

```
Is outlet X == inlet X?
  ├─ YES → No midpoints needed (straight vertical line)
  └─ NO → Can objects be repositioned?
       ├─ YES → Move objects to align X, then straight line
       └─ NO → Is it a feedback (upward) connection?
            ├─ YES → U-Shape (4 midpoints)
            └─ NO → L-Shape (2 midpoints)
```

### Avoid Crossings

When connections must cross:
1. **Reposition objects** to align outlet/inlet X coordinates
2. **Use midpoints** to route around obstacles (see patterns above)
3. **Use send/receive** for distant connections across sections
4. **Use outside lanes** (negative X or far right) for long feedback paths

## Comments and Documentation

### Comment Placement

- **Section headers**: Above the section, bold or larger text
- **Object notes**: To the right of the object
- **Block comments**: Above groups of related objects

### Comment Styling

```
Section Header (large)  → Position above section, y offset -30
Object annotation       → Position to right, x offset +100
Block comment          → Position above group, same x as first object
```

## Using get_avoid_rect_position

Always use this tool to find safe positions:

```javascript
// Find position for a new object near (100, 200)
const pos = await mcp.get_avoid_rect_position({
  patch_id: "patch_123",
  near_x: 100,
  near_y: 200,
  width: 80,   // estimated object width
  height: 20   // estimated object height
});

// Use returned position
await mcp.add_max_object({
  patch_id: "patch_123",
  object_type: "cycle~",
  position: [pos.x, pos.y],
  // ...
});
```

## Multi-Column Layouts

### Parallel Processing

For parallel signal paths, use column layout:

```
Col 1 (x=50)     Col 2 (x=170)    Col 3 (x=290)
─────────────    ─────────────    ─────────────
  cycle~           noise~           saw~
    │                │                │
   *~ 0.3          *~ 0.2           *~ 0.5
    │                │                │
    └────────────────┼────────────────┘
                     │
                   +~ +~
                     │
                   dac~
```

### Control/Signal Separation

Keep control logic separate from signal path:

```
Left Side (Controls)    Right Side (Audio)
────────────────────    ─────────────────
  [freq slider]         cycle~ ←─── connected
      │                    │
      │                   *~
      └──────────────────→ gain slider
                           │
                         dac~
```
