# Execution Model & Messaging Patterns

Max の実行モデル（hot/cold inlet）と、安全なメッセージ出力パターン。

## Hot Inlet / Cold Inlet

The fundamental execution model of Max objects. Understanding this convention is essential for correct patch design.

### The Fundamental Rule

Nearly all Max objects follow this convention:

- **Leftmost inlet = Hot**: Receiving a value triggers immediate output
- **All other inlets = Cold**: Receiving a value stores it internally without producing output

The stored cold-inlet value is used the next time the hot inlet triggers.

### Example: The + Object

```
  [hot]  [cold]
    ↓      ↓
    +  0
    ↓
  (sum)
```

- Send `5` to right inlet (cold): stores 5, no output
- Send `3` to left inlet (hot): outputs `3 + 5 = 8`
- Send `10` to left inlet (hot): outputs `10 + 5 = 15` (5 is still stored)

### Why trigger Outputs Right-to-Left

`trigger` (t) sends outputs from the rightmost outlet first, proceeding left. This is not arbitrary — it is designed to work with the hot/cold inlet convention:

```
t b -1
│    │
│    └→ + (right inlet = cold: stores -1, no output)
└───→ int (left inlet = hot: triggers output through +)
```

1. Right outlet fires first → value reaches downstream cold inlets (stored, no output)
2. Left outlet fires last → value reaches downstream hot inlets (triggers processing)
3. Result: all values are in place when processing begins

### Exceptions: All-Hot Objects

Some objects intentionally make every inlet hot:

| Object | Behavior |
|--------|----------|
| `pak` | Any inlet triggers list output (compare with `pack`: only left inlet is hot) |
| `buddy` | Outputs only when all inlets have received values |

### Practical Implications

Understanding hot/cold is essential for:

- **Correct trigger argument ordering**: Place the hot-inlet-bound output on the left
- **Choosing pack vs pak**: `pack` when you want controlled triggering, `pak` when any change should trigger
- **Debugging silent patches**: If nothing happens when you send a value, you're likely hitting a cold inlet

## Safe Messaging Patterns

Safe patterns for outputting constant values, manipulating messages, and storing lists. These patterns replace the use of `message` boxes, which are unsafe in production patches.

### Avoid message Boxes

`message` boxes have multiple risks:
- Clicking the box in locked mode sends the message
- Right inlet allows overwriting the stored value
- Easy to trigger accidentally during editing

Use `trigger` or dedicated objects instead.

### Constant Parameters with trigger

Many Max objects that store values are vulnerable to accidental modification:

| Object | Risk |
|---|---|
| `int` / `float` | Right inlet overwrites stored value |
| `message` | Clicking the box sends the message AND right outlet allows overwriting |
| `button` / `bangbang` | Clicking in edit mode or stray connections can trigger unintended bangs |
| `loadmess` | Only fires on patch load, not on demand |
| `pack` / `pak` | Inlets accept new values, changing stored data |

`trigger` with typed arguments is the safest way to output constant values:

```
trigger i 440 f 0.5 b
         │     │    └── bang
         │     └── float 0.5 (always)
         └── int 440 (always)
```

**Why trigger is safe**:
- Arguments are fixed at creation — no inlet can change them
- No clickable UI that could accidentally fire
- No right inlet for value injection
- Outputs are deterministic regardless of input (any input triggers the sequence)
- Right-to-left output order is predictable

**Usage pattern**:
```
bang / any message
  ↓
trigger 440 0.5 b
  │      │    └── to envelope (bang)
  │      └── to gain (*~) (0.5)
  └── to oscillator (cycle~) (440)
```

**Type specifiers**: `i` (int), `f` (float), `b` (bang), `l` (list), `s` (symbol), or literal values.

### Single-Value Messages: trigger

For sending a specific constant value (e.g., `1`, `clear`), use `trigger` instead of `message`:

```
bang
  ↓
t 1         → always outputs 1
```

```
bang
  ↓
t clear     → always outputs "clear"
```

**Why not message?** A `message` box with `1` can be clicked accidentally, and its value can be overwritten via the right inlet. `trigger` has neither problem.

### Multiple Commands with Shared Prefix: trigger + prepend

When generating multiple commands with the same prefix (e.g., `get min`, `get max`, `get name`), use `trigger` to output keywords and a shared `prepend` to add the prefix:

```
t b b b
  ↓       ↓       ↓
t name  t max   t min
  ↓       ↓       ↓
  └── prepend get ─┘
          ↓
    (output: get name / get max / get min)
```

**Benefits**:
- Single `prepend` instead of duplicating the prefix (DRY)
- Easy to add/remove commands
- `trigger` outlet ordering enables consistent layout alignment

### Setting Values Without Output: prepend set

To set an object's value without triggering output (equivalent to `set $1`), use `prepend set`:

```
incoming value
  ↓
prepend set
  ↓
number / flonum / message    → value is set, no output triggered
```

This is safer and more readable than using `message` with `set $1`.

### Appending to Messages: append

To add a value at the end of a message or list:

```
incoming value
  ↓
append hz
  ↓
→ "440 hz"
```

### List Storage: zl.reg

To store and recall a list, use `zl.reg`:

```
list input
  ↓
zl.reg
  ↓ (bang to recall)
stored list output
```

**Why not message or coll?** `zl.reg` is purpose-built for list storage — no click risk, no file overhead.

### Building Specific Lists: pack

To construct a specific list from individual values:

```
pack 0 0. symbol
  │   │    └── symbol inlet
  │   └── float inlet
  └── int inlet (triggers output)
```

`pack` outputs the list when the leftmost inlet receives a value. Use `pak` if any inlet should trigger output.

## Sources

- https://leico.github.io/TechnicalNote/Max/constant
