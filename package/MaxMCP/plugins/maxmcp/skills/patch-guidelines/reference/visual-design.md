# Visual Design for Presentation Mode

Patterns for styling Max/MSP device presentation views using panels, colors, and decorative elements.

## Two-Layer Panel Background

### The Pattern

Use two overlapping `panel` objects to create a sense of depth:

```
panel (outer)   →  Full bleed background, no rounded corners
  └→ panel (inner)  →  Inset area, rounded corners, semi-transparent
```

### Outer Panel (Background)

| Attribute | Value | Purpose |
|-----------|-------|---------|
| `presentation_rect` | `[-0.8, -1.5, W+2, H+2]` | Extends beyond device edges to eliminate gaps |
| `rounded` | `0` | Sharp corners (hidden by device frame) |
| `bgcolor` alpha | `1.0` | Fully opaque |
| `border` | `0` | No border |

**Key technique**: Use slightly negative x/y and slightly oversized width/height to ensure the background fills the entire device frame with no edge gaps.

### Inner Panel (Content Area)

| Attribute | Value | Purpose |
|-----------|-------|---------|
| `presentation_rect` | `[margin, margin, W-2*margin, H-2*margin]` | Inset from edges |
| `rounded` | `8` | Rounded corners for visual softness |
| `bgcolor` alpha | `0.7` | Semi-transparent, letting outer panel show through |
| `border` | `0` | No border |

**Key technique**: The semi-transparent inner panel over the opaque outer panel creates a subtle depth effect without shadows or gradients.

### Example Values

```
Device size: ~192 x 174

Outer: [-0.8, -1.5, 191.8, 173.9]  bgcolor: [0.467, 0.157, 0.631, 1.0]
Inner: [6.8, 8.1, 145.5, 155.0]    bgcolor: [0.286, 0.075, 0.400, 0.7]
```

## Unified Color Palette

### Single-Hue Multi-Brightness Approach

Use one hue with 3 brightness levels plus white for text:

| Role | Brightness | Example RGBA | Usage |
|------|-----------|-------------|-------|
| **Dark** | Low | `[0.341, 0.090, 0.475, 1.0]` | Button bg, numbox active bg |
| **Medium** | Mid | `[0.467, 0.157, 0.631, 1.0]` | Outer panel bg, info button bg |
| **Light** | High | `[0.553, 0.188, 0.749, 1.0]` | Active/pressed state |
| **Text** | White | `[1.0, 1.0, 1.0, 1.0]` | All text on dark backgrounds |

### Color Consistency Rules

- All `live.button` objects share the same `bgcolor` (dark)
- All `live.button` objects share the same `activebgoncolor` (light)
- `live.numbox` `activebgcolor` matches button `bgcolor` (dark)
- `live.text` `activebgcolor` matches button `activebgoncolor` (light)
- Outer panel and decorative elements share the medium brightness

### live.text Color Inversion Pattern

`live.text` (button mode) uses inverted colors between states:

| State | Background | Text |
|-------|-----------|------|
| Off/Default | Dark (`bgcolor`) | Dark (`textcolor` — appears invisible or subtle) |
| On/Active | Light (`activebgcolor`) | White (`activetextcolor`) |

## Transparent Border Technique

### The Pattern

Set all border-related attributes to alpha = 0 (transparent):

```
bordercolor:       [1.0, 1.0, 1.0, 0.0]   ← transparent
focusbordercolor:  [1.0, 1.0, 1.0, 0.0]   ← transparent
tricolor:          [1.0, 1.0, 1.0, 0.0]   ← transparent (numbox triangle)
```

### Why Not border: 0?

For `panel` objects, `border: 0` disables the border entirely. But for `live.*` objects, the border is always drawn — the only way to hide it is to set `bordercolor` alpha to 0.

### Affected Attributes by Object

| Object | Attributes to Set Alpha 0 |
|--------|--------------------------|
| `live.button` | `bordercolor` |
| `live.numbox` | `bordercolor`, `focusbordercolor`, `tricolor` |
| `live.text` | `bordercolor` |
| `panel` | `bordercolor` (or use `border: 0`) |

### Result

Elements are distinguished purely by background color differences rather than drawn borders, creating a flat, modern appearance.

## Branding Zone Pattern

### The Pattern

Place decorative/informational elements in a corner, outside the main interaction area:

```
┌──────────────────────────────────┐
│  ┌─────────────────────┐        │
│  │                     │        │
│  │  Interactive area   │        │
│  │                     │        │
│  │                     │   [i]  │  ← info button
│  │                     │  [logo]│  ← brand logo
│  └─────────────────────┘        │
└──────────────────────────────────┘
```

### Implementation

- **Info button** (`textbutton`): Small square (19x20), same bgcolor as outer panel (blends in)
- **Logo** (`fpic`): Small image (18x16), placed below info button
- Both share the same x coordinate for vertical alignment
- Both are positioned between the inner panel edge and the outer panel edge

### textbutton as Link Trigger

`textbutton` → `message` with `;\rmax launchbrowser <URL>` opens a web link:

```
textbutton "i"
  ↓ (bang on click)
message ";\rmax launchbrowser https://..."
```

This provides a non-intrusive way to link to documentation or the developer's website.

## Unified Font Size

Set all UI elements to the same font size for visual consistency:

| Object | Attribute | Value |
|--------|-----------|-------|
| `live.numbox` | `fontsize` | `11.0` |
| `live.text` | `fontsize` | `11.0` |
| `textbutton` | `fontsize` | `12.0` (slightly larger for "i" readability at small size) |

