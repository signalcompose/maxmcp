---
name: patch-guidelines
description: |
  Guidelines for creating Max/MSP patches with MaxMCP. Use this skill when:
  - Creating new Max patches via MaxMCP
  - Adding Max objects with add_max_object
  - Connecting objects with connect_max_objects or managing patchcords
  - Planning patch layout and organization
  - Building audio/MIDI processing patches
  - Designing presentation mode UI (panels, colors, object visibility)
  - Setting varnames or naming conventions for Max objects
  - Asking about MaxMCP patch creation best practices
user-invocable: true
---

# MaxMCP Patch Creation Guidelines

This skill provides comprehensive guidelines for creating well-organized, maintainable Max/MSP patches using MaxMCP's MCP tools.

## Pre-Creation Checklist

Before creating a patch, verify:

1. **MaxMCP Connection**: Ensure maxmcp agent is running and connected
2. **Target Patch**: Use `get_frontmost_patch` or `list_active_patches` to identify the target
3. **Existing Objects**: Use `get_objects_in_patch` to understand current state
4. **Layout Planning**: Plan object positions before creation

## Core Principles

### 1. Signal Flow Direction

Always follow the **top-to-bottom, left-to-right** signal flow convention:

```
[Input Sources]     ← Top of patch
      ↓
[Processing]        ← Middle
      ↓
[Output/Display]    ← Bottom of patch
```

### 2. Object Placement Strategy

Use `get_avoid_rect_position` to find safe positions that don't overlap existing objects:

```javascript
// Before adding an object, find a safe position
const position = await mcp.get_avoid_rect_position({
  patch_id: "...",
  near_x: 100,
  near_y: 200,
  width: 80,
  height: 20
});
```

### 3. Grid-Based Layout

Align objects to a consistent grid:
- **Horizontal spacing**: 100-120 pixels between columns
- **Vertical spacing**: 40-60 pixels between rows
- **Section gaps**: 80-100 pixels between logical sections

### 4. Section Organization

Group related objects into functional sections:
- **Input section**: Top area for external inputs (adc~, midiin, etc.)
- **Processing section**: Middle area for signal processing
- **Control section**: Parameters, UI elements
- **Output section**: Bottom area for outputs (dac~, midiout, etc.)

## Patching Workflow

パッチはセクション単位で段階的に構築する。全体を一度に作らず、**3フェーズ**で進める。

### Phase 1: セクション開発（各セクションのサイズ確定）

各セクションを独立して完成させる。セクション間の接続はこのフェーズでは行わない。

1. **セクションを計画**: パッチ全体を機能セクション（Input, Processing, Output 等）に分割
2. **1セクションずつ構築**:
   - オブジェクト追加 → セクション内接続 → 動作確認
   - `organize-patch`（セクション内モード）でレイアウト整理
   - **セクションのサイズ（幅・高さ）が確定する**
3. **次のセクションへ**: 現セクションが確定してから次に着手

このフェーズ完了時点で、各セクションは内部レイアウトが完成し、サイズが固定される。

### Phase 2: セクション配置（セクション間の位置最適化）

確定した各セクションを、セクション間接続を考慮して合理的な位置に配置する。

1. **接続計画**: どのセクションのどのオブジェクトが他セクションと接続するか把握
2. **配置パターン決定**: セクション間の依存関係に応じて直列・並列を選択
3. **セクション単位で移動**: 接続元と接続先が近くなるよう、セクションごとブロック移動
   - セクション内のオブジェクト間の相対位置は変えない
   - セクション間のギャップを維持（直列: 縦 80-100px、並列: 横 80-100px）
4. **接続経路の予測**: patchcord が合理的な経路を取れる位置関係になっているか確認

**配置パターン**:

```
直列配置（順次依存）:        並列配置（合流型依存）:

[Section A]                 [Section A]  [Section B]
  +80-100px gap                  ↓            ↓
[Section B]                    ← 合流 →
  +80-100px gap              [Section C]
[Section C]
```

- **直列**: A → B → C のように順次依存する場合、縦に配置
- **並列**: A と B が独立して動作し、C が両方の結果を必要とする場合、A と B を横に並べて配置し、C を下に配置

### Phase 3: セクション間接続

セクション配置が確定した後、セクション間の patchcord を接続する。

1. **セクション間接続を実行**: `connect_max_objects` でセクション間の patchcord を追加
2. **midpoints 設定**: 必要に応じて `set_patchline_midpoints` で経路を最適化
3. **最終検証**: `organize-patch`（Phase 8 検証）で重複・交差がないか確認

### Why This Order Matters

- Phase 1 でセクションサイズが確定するため、Phase 2 で正確な配置計算ができる
- Phase 2 でセクション位置が確定するため、Phase 3 で patchcord 経路が安定する
- 後フェーズでの手戻りが最小化される

## Object Creation Best Practices

### Varname Conventions

Always provide meaningful varnames for important objects:

```javascript
await mcp.add_max_object({
  patch_id: "...",
  object_type: "cycle~",
  args: "440",
  varname: "osc_main",  // Meaningful, descriptive name
  position: [100, 200]
});
```

**Naming patterns**:
- `osc_*` for oscillators
- `filt_*` for filters
- `env_*` for envelopes
- `gain_*` for gain controls
- `ctrl_*` for UI controls
- `in_*` / `out_*` for I/O

### Connection Guidelines

Connect objects using their varnames:

```javascript
await mcp.connect_max_objects({
  patch_id: "...",
  source_varname: "osc_main",
  source_outlet: 0,
  dest_varname: "gain_master",
  dest_inlet: 0
});
```

**Connection rules**:
- Connect signal outlets (0) to signal inlets
- Verify inlet/outlet indices before connecting
- Use `get_patch_info` to check object connections

## Common Patterns

### Audio Signal Chain

```
cycle~ → *~ → dac~
```

1. Create oscillator at top
2. Create gain control (*~) below
3. Create dac~ at bottom
4. Connect in order

### MIDI Processing

```
midiin → midiparse → [processing] → noteout
```

### Subpatcher Organization

For complex patches, use subpatchers (p object):
- Group related functionality
- Use send/receive for communication
- Document inputs/outputs

## Reference Documentation

### Execution Model & Messaging Patterns (MUST READ)

Max の実行モデル（hot/cold inlet）と安全なメッセージ出力パターン。全てのパッチ作業の前提知識。

**Key topics**:
- Hot (leftmost) vs cold (other) inlets — Max の基本実行モデル
- Why `trigger` outputs right-to-left
- `pack` vs `pak` (controlled vs any-inlet triggering)
- Avoiding `message` boxes — use `trigger`, `prepend`, `append` instead
- `trigger` as the safest constant parameter source
- `zl.reg` for safe list storage, `pack` / `pak` for list construction

See [Execution Model & Messaging Reference](reference/execution-and-messaging.md)

### Object Text Conventions (MUST READ)

オブジェクトテキストの記述規則と効率的なコーディングパターン。

**Key topics**:
- Use abbreviations (`trigger` → `t`, `int` → `i`, etc.)
- Explicit type via arguments (`scale 0. 1.` not `scale 0 1`)
- `scale` exponent argument for integrated curve mapping (replacing `pow` + `scale`)
- Multiplication filter pattern (`*`) as a compact alternative to `gate + i`

See [Object Text Conventions Reference](reference/object-text-conventions.md)

### Layout & Visual Design

- [Layout Rules](reference/layout-rules.md) - Detailed positioning and spacing rules
- [MCP Notes](reference/mcp-notes.md) - MCP tool-specific notes (attribute setting, presentation workarounds)
- [Naming Conventions](reference/naming-conventions.md) - Varname and object naming standards
- [Presentation Layout](reference/presentation-layout.md) - Dual-mode design: patching vs presentation positioning
- [Visual Design](reference/visual-design.md) - Panel backgrounds, color palettes, border styling
- [JavaScript Guide](reference/javascript-guide.md) - v8/v8ui scripting recommendations

## MCP Tools Quick Reference

| Tool | Purpose |
|------|---------|
| `list_active_patches` | List registered patches |
| `get_frontmost_patch` | Get currently focused patch |
| `get_objects_in_patch` | List objects in a patch |
| `get_patch_info` | Get patch metadata |
| `add_max_object` | Create a new object |
| `set_object_attribute` | Modify object properties |
| `get_object_value` | Get current value (number, slider, etc.) |
| `connect_max_objects` | Create patchcord |
| `disconnect_max_objects` | Remove patchcord |
| `get_patchlines` | List all patchcords with coordinates and midpoints |
| `set_patchline_midpoints` | Add/remove midpoints to fold patchcords |
| `remove_max_object` | Delete an object |
| `get_avoid_rect_position` | Find safe position |
| `get_console_log` | Retrieve Max console messages |
