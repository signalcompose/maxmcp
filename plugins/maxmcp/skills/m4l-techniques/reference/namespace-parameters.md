# M4L Namespaces & Parameter Persistence

Max for Live introduces device-level scoping and parameter persistence behaviors that differ from standard Max. Understanding these differences is critical for building reliable M4L devices.

## 🔴 必読: アンチパターン

以下は namespace / pattr 永続化で**症状から原因に辿り着くのが極めて困難**な誤実装。`pattr` を追加する前に必ず確認。

### ❌ Anti-pattern 1: pattr 名に `---` プレフィックスを使う

```
pattr ---volume    // ← セッション間で値が消える
```

**症状**: Live Set を保存して再ロードすると pattr の値がデフォルトに戻る。Live セッション内では動作する（ロード前の値は保持）。

**根本原因**: `---` は load のたびに別の ID に置換される（例: `042volume` → 次は `057volume`）。保存データのキーが一致しなくなる。

**正解**: pattr 名から `---` を取り除く。各デバイスインスタンスは pattr システムが自動的に分離する。

```
pattr volume       // ← 正常に永続化される
```

### ❌ Anti-pattern 2: bpatcher 内の unbound pattr で Float 型を使う

```
// bpatcher 内 (引数 namespace 使用)
pattr #1_internal_state    // UI バインドなし
// _parameter_type: 0 (Float) ← セッション間で値が消える
```

**症状**: bpatcher 内の状態保存用 pattr の値が再ロードで 0 にリセットされる。同じ pattr が main patcher にあると正常動作する。

**根本原因**: 未文書化（Cycling '74 にも明記なし）。bpatcher + 引数 namespace + UI 未バインド + Float 型の組み合わせで発現。

**正解**: `_parameter_type` を `3` (blob) に変更。

```
pattr #1_internal_state
// _parameter_type: 3 (blob) ← 値が永続化される
```

### ❌ Anti-pattern 3: `selected_parameter` の出力フォーマットを推測

```
live.observer (selected_parameter) → t i → ...   // ← フォーマット推測
```

**症状**: route / unpack の分岐に該当せず、observer 出力が黙って消える。Learn が機能しない。

**根本原因**: `selected_parameter` の出力は `id N` 形式だが、バースト発火・重複通知が発生する。フィルタチェーンを通さないと正しく処理できない。

**正解**: [LOM Observer Patterns](lom-observer-patterns.md) の `change 0 → thresh 0 → zl.ecils 1` フィルタチェーンを必ず通す。

---

## Namespace Prefixes: `---` vs `#0`

### `---` (Three Dashes) — Device-Wide Scope

In Max for Live, `---` is replaced with a unique identifier **per device instance**. All objects within the same M4L device share the same `---` value.

```
// Device instance A:
send ---mydata    → becomes "042mydata"
receive ---mydata → becomes "042mydata" (same device, matches)

// Device instance B:
send ---mydata    → becomes "057mydata" (different device, different ID)
```

**Use case**: Communication between abstractions/bpatchers within the same M4L device.

### `#0` — Patcher-Instance Scope

`#0` is replaced with a unique integer **per patcher load**. Each abstraction or bpatcher instance gets its own `#0` value, even within the same device.

```
// bpatcher instance 1:
send #0_data    → becomes "1234_data"

// bpatcher instance 2 (same device):
send #0_data    → becomes "5678_data" (different patcher, different ID)
```

**Use case**: Isolating communication within individual bpatcher instances.

### Comparison

| Feature | `---` | `#0` |
|---|---|---|
| Scope | Entire M4L device | Single patcher instance |
| Shared across bpatchers | Yes | No |
| Unique per device copy | Yes | Yes (per patcher) |
| Cross-device isolation | Yes | Yes |
| Works in standard Max | No (M4L only) | Yes |

### Combined Usage

Both can be used simultaneously without conflict:

```
send ---#0_param    → device-unique + patcher-unique
```

This creates a fully qualified name that is unique both across devices and across bpatcher instances within a device.

## pattr Must NOT Use `---` Prefix

### The Problem

If a `pattr` object's name starts with `---`, **saved parameter values cannot be restored** on device reload.

```
pattr ---volume    ← BROKEN: values reset to default on reload
```

**Why**: The `---` prefix generates a different ID each time the device loads (e.g., `042volume` on first load, `057volume` on second load). The saved data is keyed to the old ID and cannot be found.

### The Solution

Simply omit the `---` prefix for `pattr` names:

```
pattr volume       ← CORRECT: values persist across sessions
```

Testing confirms that multiple instances of the same device on different tracks do not cause `pattr` naming conflicts — each device instance maintains its own parameter storage independently.

**Rule**: Never use `---` in `pattr` names. The `pattr` system handles device-level isolation internally.

## Unbound pattr Persistence (Float → blob)

### The Problem

Unbound `pattr` objects (not connected to any UI) inside a `bpatcher` with argument-based namespacing fail to persist their values when the parameter type is **Float**. Values reset to 0 on reload.

```
// Inside bpatcher with @args prefix
pattr #1_internal_state    ← stores intermediate data, no UI
// Parameter Type: Float   ← values LOST on reload
```

### The Solution

Change the parameter type from **Float** to **blob** in the Parameter Inspector:

```
pattr #1_internal_state
// Parameter Type: blob    ← values PERSIST correctly
```

The exact cause is undocumented, but the blob type reliably persists values in this configuration.

## selected_parameter Monitoring

`live_set view selected_parameter` を `live.observer` で監視する場合、バースト出力や不正データのフィルタリングが必要。詳細なフィルタチェーンは [LOM Applied Patterns](lom-observer-patterns.md#selected_parameter-フィルタチェーン) を参照。

## Quick Reference

| Scenario | Solution |
|---|---|
| Share data across bpatchers in same device | Use `---` prefix |
| Isolate data per bpatcher instance | Use `#0` prefix |
| Both device + instance isolation | Use `---#0_` prefix |
| Store pattr values persistently | Do NOT use `---` in pattr name |
| Unbound pattr in bpatcher loses values | Change type to blob |
| selected_parameter gives wrong IDs | [フィルタチェーン](lom-observer-patterns.md#selected_parameter-フィルタチェーン) |

## Sources

- https://leico.github.io/TechnicalNote/Live/m4l-bpatcher-namespace
- https://leico.github.io/TechnicalNote/Live/pattr-name
- https://leico.github.io/TechnicalNote/Live/m4l-bpatcher-pattr-nobind
- https://leico.github.io/TechnicalNote/Live/selected-parameter
