---
name: m4l-techniques
description: |
  Max for Live development techniques and best practices. Use this skill when:
  - Accessing Ableton Live parameters from Max for Live (Live Object Model)
  - Using live.path, live.object, live.observer
  - Building dynamic LOM paths or monitoring Live set properties
  - Managing M4L device namespaces (--- vs #0)
  - Handling pattr persistence issues in M4L context
  - Working with Push2 parameter mapping
  - Converting between Live levels and dBFS values
user-invocable: true
---

# Max for Live Development Techniques

Practical techniques for building robust Max for Live devices, covering the Live Object Model, device namespaces, parameter persistence, and common pitfalls.

## Categories

### Live Object Model (LOM)

The foundation for programmatic control of Ableton Live from Max for Live devices.

**Key topics**:
- Path-based navigation (`live.path`)
- ID generation and session-scoped lifecycle
- Parameter control with `live.object` (get/set/call)
- Real-time monitoring with `live.observer`

See [Live Object Model Reference](reference/live-object-model.md)

### LOM Applied Patterns

Reusable patterns for building production M4L devices with the Live Object Model.

**Key topics**:
- Dynamic LOM path construction with `pak` + `zl.join`
- Dynamic parameter range from live.observer list counting
- Visual feedback via `highlighted_clip_slot`
- Triggering LOM methods with `live.text` button mode

See [LOM Applied Patterns Reference](reference/lom-patterns.md)

### LOM Observer Patterns

`live.observer` を使った監視・フィルタリングの実践パターン。

**Key topics**:
- `live.observer` の有効化・無効化パターン
- `selected_parameter` フィルタチェーン
- `canonical_parent` による自デバイスパラメータ除外

See [LOM Observer Patterns Reference](reference/lom-observer-patterns.md)

### Namespaces & Parameter Persistence

M4L-specific naming and state management patterns.

**Key topics**:
- `---` (device-wide) vs `#0` (instance-local) namespace prefixes
- Why `pattr` names must NOT use `---` prefix
- Unbound `pattr` persistence fix (Float → blob)
- `selected_parameter` monitoring workaround

See [Namespace & Parameters Reference](reference/namespace-parameters.md)

### Tips & Reference Values

Practical tips for M4L device development.

**Key topics**:
- Logarithmic controller mapping (input^exponent)
- Live level values and dBFS reference table
- Push2 Automapping Index for parameter display order

See [Tips Reference](reference/tips.md)

### Live Parameter Rules

M4L デバイス固有のコーディングルール。Live パラメータシステムとの正しい連携方法。

**Key topics**:
- `live.observer` のアトリビュート設定方法と property メッセージ
- `live.*` UI オブジェクトの `_parameter_unitstyle` 設定
- `pattr` の `parameter_enable` と永続化設定
- `live.dial` の表示要素制御（`showname` / `shownumber`）
- パラメータ保存方式の選択（Live 直接管理 vs pattrstorage）
- `_parameter_order` による復元順序の完全定義

See [Live Parameter Rules Reference](reference/live-parameter-rules.md)
