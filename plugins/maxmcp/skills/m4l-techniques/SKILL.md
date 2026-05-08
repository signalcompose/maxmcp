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

## 🔴 MUST: パターン参照の強制（最重要）

**LOM 系オブジェクト（`live.path` / `live.object` / `live.observer` / `live.thisdevice` / `pattr`）を `add_max_object` で追加する前に、該当する reference ドキュメントを Read で必ず先に読み込む。**

| 追加するオブジェクト | 必ず Read すべきドキュメント |
|---|---|
| `live.observer` | `reference/lom-observer-patterns.md` 全文 |
| `live.path`（動的構築・selection 監視） | `reference/lom-patterns.md` + `reference/lom-observer-patterns.md` |
| `live.object`（Learn 系・自デバイス除外が必要） | `reference/lom-observer-patterns.md` の canonical_parent セクション |
| `pattr`（永続化・型指定） | `reference/live-parameter-rules.md` Section 3, 5, 6 |
| `live.*` UI（live.dial, live.numbox, live.text 等） | `reference/live-parameter-rules.md` Section 1, 2 + **`../patch-guidelines/reference/mcp-notes.md` Section 5（生成テンプレート全項目）** |
| LOM 操作を含む M4L デバイス全般 | `reference/tips.md` Two-Stage Init + 本 SKILL.md「🔴 MUST: LOM を使う M4L デバイスは `live.thisdevice` 必須」 |

### 🔴 `_parameter_order` を持つ M4L デバイスを構築する場合

**自分の頭で順序を組み立てない**。デバイス内の全パラメータに `_parameter_order` を明示し、以下のルールに従って順序を決定する。判断に迷ったら `reference/live-parameter-rules.md` Section 6 を Read（典型例として ParameterMapper の order 表が掲載されている）。

**順序決定ルール（オブジェクト種別ごと）**:

1. **`_parameter_range` 供給 → 受領の順**: ある pattr の値が別の UI/pattr の `_parameter_range` を決定する場合、供給側を低 order、受領側を高 order に置く。受領側が復元される時点で範囲が確定していないと値がクランプされる。
2. **`pack` を介する依存**: `pack` は inlet 0 のみ hot。cold inlet (右側) に入るパラメータを低 order、hot inlet (inlet 0) に入るパラメータを高 order にすると、hot 受信で全 cold 値が揃った状態で発火する。
3. **`pak` を介する依存**: `pak` は全 inlet が hot で、各 inlet 受信のたびに発火する。したがって「cold/hot で並べる」ロジックは適用できない。代わりに、**チェーンの最終状態を確定させるパラメータを最高 order** に置き、それより前の order での発火は中間状態として許容する（最高 order の発火時点で全 inlet が正しい値で揃う）。
4. **`scale` / `expr` 等の演算入力**: 演算側の係数 (cold inlet) を低 order、トリガ側 (hot inlet) を高 order に置く。
5. **独立パラメータ**: 範囲・発火依存のないもの（Learn toggle 等）は末尾、または他制約に影響しない位置に置く。

**🚫 禁止**:
- 「pack と pak を同じ感覚で並べる」: pak は全 hot のため cold/hot ルールは無効
- 「order を 0 のままにする」: デフォルト 0 は順序不定で再現性がない
- 「典型構成 (Learn + min/max + ダイアル + 表示) でルールを再設計する」: Section 6 の order 表に一致するなら、そのままコピーしてよい。逸脱する場合は理由をコメントで明示

### 🚫 禁止事項

1. **「設計を頭の中で組み立ててから接続する」**: これが適用漏れの最大原因。reference に書かれている既存パターンを忘れる/見落とす。
2. **「reference を読まずに add_max_object する」**: たとえ「以前読んだ」でも毎回 Read する。会話履歴が長いほど忘却が進む。
3. **「出力フォーマットを推測する」**: `live.observer` / `live.object get` の応答を確認せず route / unpack 設計する。

### ✅ 必須手順（毎回）

1. **Read** で該当 reference を取得（要約しない、全文）
2. reference に記載されたパターン図をそのまま再現するつもりで `add_max_object` / `connect_max_objects` を計画
3. 不確実な箇所には **Phase 5（Empirical Verification）で `print` プローブ + `get_console_log`** で実証確認

### 自己チェック質問（LOM チェーン構築前）

以下に **reference を見ずに** 答えられない場合は、reference を再 Read する:

1. `live.path` の発火源は何か？ （loadbang / message / sel outlet bang のどれか）
2. `live.observer` の inlet 0 / inlet 1 にそれぞれ何を送るか？
3. 監視を無効化する経路は？（何を送ると停止するか）
4. 出力フォーマットは？（`id N` / 値 / list / property prefix 付き）
5. selected_parameter 監視時、フィルタチェーン（change / thresh / zl.ecils）は何のために必要か？

**1 つでも reference 無しで即答できないなら**: 推測で実装する前に reference を Read する。

## 🔴 MUST: LOM を使う M4L デバイスは `live.thisdevice` 必須

### ❌ Anti-pattern: M4L で LOM を使うのに `live.thisdevice` を配置しない

```
// LOM チェーンが load 時に発火しない誤実装
live.path live_set view → live.observer  // ← live.path にトリガが来ない
pattr param_min → ...                     // ← 復元値が下流に届かない可能性
```

**症状**:
- Learn ボタンを押しても observer が反応しない
- パッチをロードしても `_parameter_range` が初期化されない
- ユーザーが Live Set を再起動するたびに動作しなくなる

**根本原因**: M4L デバイスの load タイミングは `loadbang` だけでは不十分。Live のパラメータシステム初期化や LOM 接続が完了する前に発火してしまうことがある。

### ✅ 正解: `live.thisdevice` を起点とする初期化チェーン

```
live.thisdevice
  ↓ (デバイス + LOM 完全初期化後に発火)
t b b
  ├→ s ---init_lom         (LOM チェーン起動: live.path bang 等)
  └→ delay 100
       ↓
       s ---init_params    (パラメータ復元の二段目)
```

**ルール**: **LOM (`live.path` / `live.observer` / `live.object`) を含む M4L デバイスは、`live.thisdevice` を必ず配置し、LOM 関連オブジェクトに bang を送る経路を持つ**。

詳細は [reference/tips.md](reference/tips.md) の Two-Stage Initialization セクション、および 3 段階以上のカスケードが必要な場合は [../max-techniques/reference/cascading-init.md](../max-techniques/reference/cascading-init.md) を参照。

## M4L デバイス構築チェックリスト

M4L デバイスを構築する際、以下の順序で設計・実装する。

### 設計段階（patch-guidelines Phase 0 と併用）

1. **保存方式を決定**: Live 直接管理 vs pattrstorage 管理（[Live Parameter Rules](reference/live-parameter-rules.md) Section 5 参照）
2. **パラメータの復元依存チェーンを設計**: `_parameter_order` を hot/cold inlet の依存関係に基づいて決定（[Live Parameter Rules](reference/live-parameter-rules.md) Section 6 参照）
3. **pattr の命名**: `---` プレフィックスを使わない（[Namespace & Parameters](reference/namespace-parameters.md) 参照）
4. **pattr のデータ型を決定**: Float 値は `_parameter_range [-100000, 100000]`、テキストは `_parameter_type 3 (blob)`（[Live Parameter Rules](reference/live-parameter-rules.md) Section 3 参照）

### 実装段階

5. **live.observer パターン**: property はメッセージで設定、`@property` 構文は使わない（[Live Parameter Rules](reference/live-parameter-rules.md) Section 1 参照）
6. **live.object の API**: プロパティ設定は `set <property> <value>`、取得は `get <property>`（`<property> <value>` ではない）
7. **live.* UI の unitstyle**: `_parameter_type` が Float (0) なら `_parameter_unitstyle` も Float (1) に設定（[Live Parameter Rules](reference/live-parameter-rules.md) Section 2 参照）
8. **pattr outlet 構造**: outlet 0 = 値出力、outlet 1 = bindto（UI バインド）、outlet 2 = dumpout。UI にバインドする場合は outlet 1 をパッチコードで接続

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
