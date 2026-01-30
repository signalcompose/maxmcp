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

### Straight Lines

- Prefer vertical connections when possible
- Align objects to minimize cord crossings
- Use segmented patch cords for complex routing

### Avoid Crossings

When connections must cross:
1. Consider reorganizing object positions
2. Use send/receive for distant connections
3. Group related connections visually

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
