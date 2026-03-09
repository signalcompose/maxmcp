# Presentation Mode Layout

Guidelines for arranging UI objects in presentation mode, where the goal shifts from patching clarity to user operability.

## Dual-Mode Design Principle

Every UI object in a Max patch exists in two layouts simultaneously:

- **Patching mode (patching_rect)**: Optimized for signal flow visibility and connection access
- **Presentation mode (presentation_rect)**: Optimized for end-user interaction

The same object serves different purposes in each mode, and its size and position should reflect that.

### What Changes Between Modes

| Aspect | Patching Mode | Presentation Mode |
|--------|---------------|-------------------|
| **Position** | Near connected objects (minimize patchcord length) | Grouped by user interaction logic |
| **Width** | Minimum needed for connections | Expanded for easy mouse/touch interaction |
| **Height** | Usually unchanged | Usually unchanged (Max enforces minimums) |
| **Grouping** | By signal flow (inputs → processing → output) | By user task (navigation, display, actions) |

### Example: live.numbox

```
Patching:      [48 x 17]     ← narrow, near connected objects
Presentation:  [129 x 17]    ← 2.7x wider, fills available panel width
```

Width expands for easier interaction; height stays at 17px (the object's minimum display height).

### Example: Directional Buttons

```
Patching:                           Presentation:
  left [32x32]   right [32x32]           [up]
  (x=520)        (x=590)            [left] [right]
                                          [down]
  up [32x32]     down [32x32]
  (x=830)        (x=902)            Cross/D-pad layout
                                    matching directional intent
  Grouped by function               Grouped by spatial meaning
  (Track pair / Scene pair)         (directional cross)
```

In patching mode, left/right buttons sit near the Track processing chain, and up/down buttons sit near the Scene processing chain. In presentation mode, all four form a directional cross that matches their spatial meaning.

## Layout Strategies

### Expand Interactive Elements

Controls that users interact with frequently should be sized for comfortable use:

| Element | Patching Width | Presentation Width | Rationale |
|---------|---------------|-------------------|-----------|
| `live.numbox` | 48px (default) | Full panel width | Easier drag interaction |
| `live.text` | 60px | 50-60px | Button-sized, adequate for click |
| `live.button` | 32x32 | 23x23 | Can shrink to fit a compact layout |

**Rule**: Prioritize width for elements that require dragging (numbox, slider). Click targets (buttons) can be smaller if the layout demands it.

### Shrink Decorative Elements

Branding and informational elements that are prominent in patching mode should shrink in presentation:

| Element | Patching Size | Presentation Size | Rationale |
|---------|--------------|-------------------|-----------|
| Logo image (`fpic`) | 117x100 | 18.5x15.8 | Present but unobtrusive |
| Info button (`textbutton`) | 100x20 | 19.3x19.6 | Small square, corner placement |

**Rule**: Decorative elements should not compete with interactive elements for space.

### Reorganize by User Intent

Patching mode groups objects by signal flow. Presentation mode groups them by user task:

```
Patching layout:                    Presentation layout:

  Track section:                      Navigation area:
    left, right buttons                 D-pad (up/down/left/right)
    Track numbox                        Launch button
    Track processing chain
                                      Display area:
  Scene section:                        Track numbox (wide)
    up, down buttons                    Scene numbox (wide)
    Scene numbox
    Scene processing chain            Branding (corner):
                                        Info button, logo
  Launch section:
    live.text, fire trigger
```

## Selective Inclusion

Presentation mode is **opt-in per object**. Only objects that the end user needs to see or interact with should be included. All internal processing objects remain hidden.

### What to Include

| Include | Examples | Reason |
|---------|----------|--------|
| User controls | `live.button`, `live.numbox`, `live.slider`, `live.text`, `live.dial` | Direct user interaction |
| Value displays | `live.numbox` (read-only), `live.meter~`, `multislider` | Visual feedback |
| Background panels | `panel` | Layout structure and visual framing |
| Branding/info | `fpic` (logo), `textbutton` (info link) | Device identity |

### What to Exclude

| Exclude | Examples | Reason |
|---------|----------|--------|
| Processing logic | `trigger`, `change`, `int`, `+`, `clip`, `pak` | No user interaction |
| LOM infrastructure | `live.path`, `live.object`, `live.observer` | Internal plumbing |
| Message routing | `prepend`, `pack`, `zl.*`, `send`, `receive` | No visual purpose |
| Comments | `comment` | Developer notes, not for end users |

### Typical Ratio

In a production device, only a small fraction of objects appear in presentation mode. A typical ratio is 10-15% — for example, a device with ~80 total objects might have only ~12 in the presentation view. The remaining 85-90% are internal processing that the user never sees.

## Enabling Presentation Mode

To include an object in the presentation view:

```
set_object_attribute: attribute = "presentation", value = 1
```

To set the presentation position and size (independent of patching_rect):

```
set_object_attribute: attribute = "presentation_rect", value = [x, y, width, height]
```

**Important**: `presentation_rect` is completely independent of `patching_rect`. Changing one does not affect the other. This is what enables the dual-mode design.

