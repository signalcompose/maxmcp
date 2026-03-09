# LOM Applied Patterns

Reusable patterns for working with the Live Object Model in production Max for Live devices.
These patterns assume familiarity with the LOM pipeline covered in [Live Object Model Reference](live-object-model.md).

## Dynamic Path Construction (pak + zl.join)

### The Problem

Building LOM paths like `live_set tracks N clip_slots M` where N and M change at runtime. Static paths only work for fixed targets.

### The Pattern

```
[track number]      [scene number]
  ↓                   ↓
pak tracks 0        pak clip_slots 0
                      ↓
                    t b l
                    │   │
                    │   └→ zl.join (right inlet: store list)
                    └───→ zl.join (left inlet: bang triggers join)
                           ↓
                         prepend path live_set
                           ↓
                         live.path
```

### How It Works

1. `pak tracks N` creates the list `tracks 2` (pak triggers on any inlet change)
2. `pak clip_slots M` creates `clip_slots 3`
3. `t b l` separates the output into two steps:
   - First (right→left): the list `clip_slots 3` goes to `zl.join`'s right inlet (cold — stored, no output)
   - Then: bang goes to `zl.join`'s left inlet (hot — triggers join with the stored tracks list)
4. `zl.join` combines both lists: `tracks 2 clip_slots 3`
5. `prepend path live_set` creates: `path live_set tracks 2 clip_slots 3`
6. `live.path` resolves the full path to an object ID

### Key Insight

The `t b l` ordering is critical. Without it, `zl.join` might fire before both parts are ready. The trigger ensures the clip_slots list is stored (cold inlet) before the bang triggers the join (hot inlet).

### Generalization

This pattern works for any multi-segment LOM path:

```
path live_set tracks N devices M
path live_set scenes N
path live_set tracks N clip_slots M clip
```

Extend with additional `pak` + `zl.join` stages for deeper paths.

## Dynamic Range via LOM List Counting

### The Problem

A `live.numbox` for track/scene selection needs its range to match the actual number of tracks/scenes in the current Live set. The count changes when the user adds or removes tracks.

### The Pattern

```
(init signal, e.g. r ---lb2)
  ↓
pack path live_set
  ↓
live.path → (outlet 0: path string, outlet 1: id)
  ↓
t tracks l              ← or: t scenes l
│        │
│        └→ live.path (right inlet: observe path)
│
├→ prepend property
│    ↓
│  live.observer        ← monitors "tracks" property
│    ↓
│  zl.filter id         ← remove "id" tokens from list
│    ↓
│  zl.len               ← count remaining items
│    ↓
│  - 1                  ← convert to 0-based max index
│    ↓
│  pak 0 N              ← [min, max] range pair
│    ↓
│  prepend _parameter_range
│    ↓
│  live.numbox          ← range updated dynamically
```

### How It Works

1. `live.observer` monitors the `tracks` property of `live_set`
2. When tracks change, observer outputs a list like: `id 1 id 2 id 3`
3. `zl.filter id` removes the `id` tokens, leaving: `1 2 3`
4. `zl.len` counts the items: `3`
5. `- 1` converts to 0-based max index: `2` (valid range: 0, 1, 2)
6. `pak 0 N` creates the range pair `0 2`
7. `prepend _parameter_range` sends `_parameter_range 0 2` to set the numbox range

### Why zl.filter id Is Needed

`live.observer` outputs ID lists in the format `id 1 id 2 id 3` (alternating "id" tokens and actual IDs). Without filtering, `zl.len` would return 6 instead of 3.

### Reset on Range Change

When the range shrinks (e.g., a track is deleted), the current value might exceed the new maximum. Use `t 0 l` after `prepend _parameter_range` to:

```
prepend _parameter_range
  ↓
t 0 l
│   └→ live.numbox (set new range via _parameter_range)
└───→ live.numbox (reset value to 0)
```

This ensures the value is always within the valid range.

## highlighted_clip_slot Visual Feedback

### The Problem

When navigating clips from a Max device, the user has no visual indication in Live's Session View of which clip slot is currently selected.

### The Pattern

```
(initialization: r ---lb1)
  ↓
pack path live_set view
  ↓
live.path
  ↓ (outlet 1: id of live_set view)
  → live.object (right inlet: set target)

(clip navigation result)
  ↓
live.path (resolves clip_slot path)
  ↓ (outlet 1: clip_slot id)
  → prepend set highlighted_clip_slot
     ↓
     live.object (left inlet: set property)
```

### How It Works

1. At initialization, resolve `live_set view` to get the Session View's object ID
2. Set this ID as the target of a `live.object`
3. When the user navigates to a new clip slot, resolve its path to an ID
4. Send `set highlighted_clip_slot <id>` to the `live.object`
5. Live's Session View highlights the corresponding clip slot

### Notes

- `highlighted_clip_slot` is a property of `live_set view`, not of individual clip slots
- The value is a clip slot ID (obtained from `live.path` outlet 1)
- This provides immediate visual feedback without launching the clip
- Useful for clip selectors, step sequencers, and navigation devices

## live.text Button Mode + call Method

### The Problem

Triggering a LOM method (like firing a clip) from a UI button requires converting a button press into a method call on `live.object`.

### The Pattern

```
live.text @mode 0       ← button mode (momentary)
  ↓ (bang on press)
t fire                  ← convert bang to method name symbol
  ↓
prepend call            ← create "call fire" message
  ↓
live.object             ← executes the method on the target object
```

### live.text Attributes

| Attribute | Value | Effect |
|-----------|-------|--------|
| `mode` | `0` | Button mode (momentary, outputs bang on press) |
| `mode` | `1` | Toggle mode (alternates between on/off states) |
| `text` | `"Launch"` | Label shown in off/default state |
| `texton` | `"Playing"` | Label shown in on/active state |
| `bgcolor` | `[r, g, b, a]` | Background color in default state |
| `activebgcolor` | `[r, g, b, a]` | Background color in active state |

### Generalization

Replace the trigger argument to call any LOM method:

```
t fire          → call fire          (launch clip)
t stop          → call stop          (stop clip)
t select_device → call select_device (select device in Live)
t jump          → call jump          (jump in arrangement)
```

### Why Not Use message?

A `message` box with `call fire` would work, but:
- Clicking the message box accidentally triggers the method
- The right inlet allows the content to be overwritten
- `trigger` + `prepend` is safer and more explicit

See the "Constant Parameters with trigger" pattern in the General Max/MSP Tips for more details.

