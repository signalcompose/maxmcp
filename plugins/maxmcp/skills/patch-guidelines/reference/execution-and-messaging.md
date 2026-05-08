# Execution Model & Messaging Patterns

Max の実行モデル（hot/cold inlet）と、安全なメッセージ出力パターン。

## 🔴 必読: 適用漏れ防止のためのアンチパターン（message ボックス禁止系）

以下は Claude が頻繁に犯す誤実装。**`add_max_object obj_type="message"` を使う前に、必ず以下のディシジョンツリーで判定する**。

### Decision Tree: 固定値を出力したい時、何を使うか

```
[出力したい内容は？]
        │
        ├── 単一の固定値 (例: 1, 0, "clear", "bang")
        │     → t <値>             (例: t 1, t 0, t clear)
        │
        ├── プレフィックス付き固定メッセージ (例: "id 0", "set value 0.85")
        │     → prepend <prefix>   (例: prepend id, prepend set value)
        │       + 上流から値 (または bang) を流す
        │
        ├── 共通プレフィックスを持つ複数の固定メッセージ
        │   (例: "get min", "get max", "get name")
        │     → t b b b → 各サブ t (キーワード) → 共通 prepend (prefix)
        │     詳細は「Multiple Commands with Shared Prefix」セクション参照
        │
        ├── 複数引数の固定メッセージ (例: "property selected_parameter")
        │     → zl.reg <値1> <値2> ...
        │       (引数で初期値保持、bang で出力)
        │
        ├── リスト構築 (動的引数)
        │     → pack / pak (Section 「Building Specific Lists」参照)
        │
        └── 動的に変わるテキスト/メッセージ (ユーザー入力等)
              → message ボックス可（ただし右 inlet に注意）
```

### ❌ Anti-pattern 1: 固定値を message ボックスで出力

```
message "property selected_parameter" → live.observer  // ← 禁止
message "id 0" → live.observer                          // ← 禁止
message "0" → live.numbox                                // ← 禁止
```

**症状**: 編集中にクリックして誤発火、右 inlet 経由で値が上書きされる、パッチコードを引っ掛けて意図しない発火。

**正解（上記の例の置換）**:

| ❌ message | ✅ 正解 |
|---|---|
| `message "0"` | `t 0` |
| `message "1"` | `t 1` |
| `message "id 0"` | bang → `prepend id` (0 を bang から prepend で構築) または `zl.reg id 0` |
| `message "property selected_parameter"` | `zl.reg property selected_parameter` |
| `message "set value 0.85"` (固定値) | `t 0.85 → prepend set value` |
| `message "set value $1"` (動的値) | `prepend set value`（上流から $1 を流す）|

### ❌ Anti-pattern 2: 共通 prefix の固定メッセージを複製

```
message "get min" ──┐
message "get max" ──┼─→ live.object   // ← prefix "get" を3回複製
message "get name" ─┘
```

**症状**: prefix を変更したい時に複数箇所修正が必要、メッセージ数が増えるとレイアウトが崩れる。

**正解**: trigger + 共通 prepend パターン (本ファイル「Multiple Commands with Shared Prefix」参照)

```
bang
  ↓
t b b b
  ↓     ↓     ↓
t name t max t min
  ↓     ↓     ↓
  └── prepend get ──→ live.object
```

### ❌ Anti-pattern 3: live.observer の有効化を message で書く

```
sel 1 outlet 0 → message "property selected_parameter" → live.observer
                                                          // ← message 禁止
```

**正解**: lom-observer-patterns.md の正規パターン (`t b b → zl.reg property selected_parameter` + `t b b → zl.reg path live_set view → live.path`)。

---

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

## Connection Design Flow（接続設計フロー）

`trigger` と cold inlet を持つオブジェクトを組み合わせる際、接続を作る**前に**以下の手順で設計する。
ここでは `pack` を例に説明するが、`+`、`int` など cold inlet を持つすべてのオブジェクトに同じ原則が適用される。

### Step 1: 下流の pack/pak の inlet 構造を特定

```
pack 0. 0.
  inlet 0 (左) = hot（発火トリガー）
  inlet 1 (右) = cold（値格納のみ）
```

### Step 2: trigger の outlet 順序を設計

trigger は右→左の順に発火する。cold inlet に送る値を右側、hot inlet に送る値を左側に配置:

```
正しい設計:
  t b i
    outlet 1 (右, 先に発火) → cold inlet に値を格納
    outlet 0 (左, 後に発火) → hot inlet に値を送信（発火）

誤った設計:
  t b i
    outlet 0 (左, 後に発火) → cold inlet（値は格納されるが発火しない）
    outlet 1 (右, 先に発火) → hot inlet（cold の値が未格納のまま発火）
```

### Step 3: _parameter_order も同じ原則で設計

Live パラメータの復元順序も hot/cold と同じ原則:

```
_parameter_order 設計:
  1. cold inlet に接続するパラメータ（先に復元 → 格納）
  2. hot inlet に接続するパラメータ（後に復元 → 発火）
  3. _parameter_range を設定するチェーンのパラメータ（範囲設定が先）
  4. その範囲内で値を復元するパラメータ（値復元が後）
  5. 独立パラメータ（最後）
```

### Step 4: 配置を接続順序に合わせる

trigger の outlet 順序（右→左）に合わせて、下流オブジェクトを左右に配置:

```
t b b b
  outlet 0 (左, 最後)    outlet 1 (中央)    outlet 2 (右, 最初)
         ↓                     ↓                    ↓
  [最後に実行]          [2番目に実行]         [最初に実行]
```

視覚的な左→右の並びと、実行順序の右→左が対応するように配置する。

## Sources

- https://leico.github.io/TechnicalNote/Max/constant
