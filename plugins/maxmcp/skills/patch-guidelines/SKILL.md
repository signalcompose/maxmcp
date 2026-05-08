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

パッチはセクション単位で段階的に構築する。全体を一度に作らず、**5フェーズ**で進める。

### Phase 0: 設計（スキルのルールを設計制約として適用）

実装を開始する前に、スキルのルールをプランの設計制約として組み込む。

**⚠ Phase 0 を飛ばした場合**: 設計段階でルールを適用しないと、実装後に trigger の outlet 順序の間違い、UI オブジェクトの配置ミス（patching_rect で UI 優先配置 → 長距離・上向き接続の大量発生）、_parameter_order の依存チェーン不整合などが発覚し、パッチ全体の再構築が必要になる。

1. **信号フロー設計**: 全セクションの依存関係を描き、上→下フローを確保
   - patching_rect は接続構造優先で配置する（[Presentation Layout](reference/presentation-layout.md) の Dual-Mode Design 参照）
   - UI オブジェクトは処理チェーンの近くに配置（presentation_rect で見た目は別途制御）
2. **接続設計**: trigger の outlet 順序と下流の hot/cold inlet の整合性を設計
   - [Execution Model](reference/execution-and-messaging.md) の Connection Design Flow に従う
   - `pack` / `pak` に接続する trigger は、cold inlet → 右 outlet、hot inlet → 左 outlet
3. **パラメータ復元順序設計**: `_parameter_order` を復元依存チェーン全体で設計
   - cold inlet に接続するパラメータ → 小さい order（先に復元）
   - hot inlet に接続するパラメータ → 大きい order（後に復元 → 発火）
   - `_parameter_range` を設定するチェーン → 範囲内で値を復元するパラメータより先
4. **アトリビュート計画**: 各オブジェクトに必要なアトリビュートを [MCP Notes](reference/mcp-notes.md) のテンプレートで事前に列挙

5. **関連スキルの参照**: 作業内容に応じて以下のスキルをロードし、設計制約として適用する

   | スキル | 条件 | 参照すべきルール |
   |---|---|---|
   | `m4l-techniques` | M4L デバイス開発時（常時） | live.observer パターン、pattr 永続化、namespace ルール、_parameter_order 設計 |
   | `max-techniques` | poly~, pattr, signal 処理等 | pattr range 制限、cascading init |
   | `max-resources` | オブジェクトの outlet/inlet 構造が不明な時 | リファレンスページで確認してから設計 |

**Why Phase 0 が必要**: プランが承認された後にルールを適用しても、設計自体が間違っていれば大量の手戻りが発生する。

### Phase 1: セクション開発（各セクションのサイズ確定）

**前提条件**: Phase 0 の設計が完了していること（信号フロー設計、接続設計、_parameter_order 設計、アトリビュート計画）。

各セクションを独立して完成させる。セクション間の接続はこのフェーズでは行わない。

**⚠ セクション単位を飛ばして全体を一括構築した場合**: Phase 8 検証（重複・交差・上向き検出）で全オブジェクト・全パッチコードを対象に確認が必要になる。セクション単位であれば検証対象はセクション内のオブジェクトのみで済むが、一括構築では変更のたびにパッチ全体の大量のオブジェクトを再確認することになり、トークン消費と作業量が爆発する。

1. **セクションを計画**: パッチ全体を機能セクション（Input, Processing, Output 等）に分割
2. **1セクションずつ構築**:
   - 🔴 **そのセクションで使う LOM/特殊オブジェクトの reference を `Read` で先に取得**（m4l-techniques の MUST 参照ルール）。会話履歴が長くても、各セクション開始時に必ず再 Read する
   - オブジェクト追加（Operation Checklists の「add_max_object の後」を実行）
   - セクション内接続（Operation Checklists の「connect_max_objects の前」を実行）
   - `organize-patch`（セクション内モード）でレイアウト整理
   - **Phase 8 検証を実行**（上向き接続・オブジェクト重複・パッチコード交差を検出・修正）
   - **セクションのサイズ（幅・高さ）が確定する**
3. **次のセクションへ**: 現セクションが確定してから次に着手

このフェーズ完了時点で、各セクションは内部レイアウトが完成し、サイズが固定される。

### Phase 2: セクション配置（セクション間の位置最適化）

**前提条件**:
- □ Phase 1 で全セクションの内部レイアウトが確定しているか？
- □ 各セクションの Phase 8 検証（上向き接続・重複・交差）が完了しているか？
→ 未完了なら Phase 2 に進まない

**⚠ Phase 1 未完了で Phase 2 に進んだ場合**: セクション単位でまとめておけばグループとして移動するだけで済むが、セクションが未確定だと個々のオブジェクトの座標を1つずつ計算・移動することになる。セクションは簡易なグループ化であり、これを飛ばすとレイアウト作業の複雑さが爆発する。

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

**前提条件**:
- □ Phase 2 でセクション配置が確定しているか？
- □ 接続経路の予測で、patchcord が合理的な経路を取れる位置関係になっているか？
→ 未完了なら Phase 3 に進まない

**⚠ Phase 2 未完了で Phase 3 に進んだ場合**: セクション位置が確定していない状態で patchcord を接続すると、カオスなパッチコードが大量生産される。その状態でパッチコードの整理を試みても、より複雑なスパゲッティを生むだけで収束しない。

セクション配置が確定した後、セクション間の patchcord を接続する。

1. **セクション間接続を実行**: `connect_max_objects` でセクション間の patchcord を追加
2. **midpoints 設定**: 必要に応じて `set_patchline_midpoints` で経路を最適化
3. **最終検証**: `organize-patch`（Phase 8 検証）で重複・交差がないか確認

### Phase 4: 完成検証

**前提条件**:
- □ Phase 3 でセクション間接続が完了しているか？
- □ Phase 3 の最終検証（Phase 8: 重複・交差）が完了しているか？
→ 未完了なら Phase 4 に進まない

**⚠ Phase 4 を飛ばした場合**: 生成時のチェックリスト（add_max_object の後）は個々のオブジェクトを対象とするため、パッチ全体での一貫性（ペア間の設定整合、_parameter_order の依存チェーン整合）は検出できない。これらの問題はユーザーが実際に保存・復元を試すまで発覚せず、原因の特定が極めて困難になる。

パッチ全体を対象とした一括検証を行う。

#### 4-1. パラメータ設定の一括検証

全 live.* UI オブジェクトと全 pattr の設定を `get_object_attribute` で読み戻し、以下を確認:

**live.* UI オブジェクト**:
- `_parameter_initial_enable` が 1 か
- `_parameter_initial` が設定されているか
- `_parameter_type` と `_parameter_unitstyle` の整合性（Float(0) → Float(1)）

**pattr**:
- `parameter_enable 1`, `_parameter_invisible 1`, `_parameter_modmode 0`, `parameter_mappable 0` が設定されているか
- `_parameter_initial_enable` が 1 か
- `_parameter_range` がデータ型に応じた適切な範囲か

#### 4-2. ペア/グループ整合性チェック

対になるオブジェクト、同一グループのオブジェクトが同じ設定パターンを持つか確認:

- **min/max ペア**: `_parameter_type`, `_parameter_unitstyle`, `_parameter_initial_enable`, `_parameter_range` が一致するか
- **pattr グループ**: `parameter_enable`, `_parameter_invisible`, `_parameter_modmode`, `parameter_mappable` が同じパターンか
- **設定の非対称を検出した場合**: 意図的な差異でない限り、設定漏れとして修正

#### 4-3. `_parameter_order` の読み戻し検証

全パラメータの `_parameter_order` を `get_object_attribute` で読み戻し、以下を検証:

1. **order の連番確認**: 抜け・重複がないか
2. **依存チェーンとの照合**:
   - cold inlet に接続するパラメータが、同じオブジェクトの hot inlet に接続するパラメータより小さい order を持つか
   - `_parameter_range` を設定するチェーン（pattr → pack → prepend → UI）が、範囲内で値を復元する UI オブジェクトより先か
   - `scale` / `pak` / `pack` 等の複数 inlet オブジェクトで、hot inlet (0) に接続するパラメータが最後に復元されるか
3. **問題を検出した場合**: order を修正し、再度読み戻して確認

#### 4-4. レイアウト最終検証（Phase 8）

パッチ全体を対象とした最終レイアウト検証:
- 上向き接続の検出
- オブジェクト重複の検出
- パッチコードとオブジェクトの交差検出

（Phase 1 の各セクションで実施済みの Phase 8 検証を、パッチ全体で再度実行）

### Phase 5: 実信号検証（Empirical Verification）

**前提条件**:
- □ Phase 4 完成検証完了
- □ 全パラメータ・接続の構造的検証完了
→ 未完了なら Phase 5 に進まない

**⚠ Phase 5 を飛ばした場合**: 構造的に正しくても **初期化トリガ漏れ** や **応答フォーマット推測ミス** で動作しないパッチが「完成」として報告される。典型例:
- `live.path` を `live.observer` に直結したが load 時に発火せず、observer が永久に沈黙
- `live.observer` の出力形式を推測して `route` 設計したが、実形式が異なり全分岐に該当せず黙って消える
- `pattr` (autorestore=0) で初期値が入らず、依存する UI が初期表示できない

これらは構造検査では検出不可能で、ユーザーが Live にロードして初めて発覚する。原因特定に膨大な時間を要する。

#### 5-1. 初期化チェーンの存在確認

`get_objects_in_patch` で以下のオブジェクトの存在を確認し、各々に起動経路があるかチェック:

| オブジェクト | 起動経路に必須なもの |
|---|---|
| `live.path` | `loadbang` / `live.thisdevice` / sel outlet bang のいずれか |
| `live.observer` | inlet 1 への id 供給 + inlet 0 への property 設定の両方 |
| `live.thisdevice` | （自身が起点なので不要、ただし下流チェーンが繋がっているか確認） |
| `pattr` (autorestore=0) | 明示的な復元バン |
| `loadmess` / `loadbang` | 出力先が処理開始の起点になっているか |

不足していれば Phase 0 設計に戻し、起動経路を追加してから Phase 5 を再実行。

#### 5-2. プローブによる出力形式実証確認

**データ形式が「reference に明記されていない」または「推測に基づく」箇所には必ず `print` プローブを一時挿入する**。

挿入対象:
- `live.observer` outlet 0 直後（property 値の実フォーマット確認）
- `live.object` outlet 0 直後（get 応答の実フォーマット確認）
- 複雑な `route` の前後（分岐が意図通りか）
- `zl.ecils` / `unpack` 等の list 操作後

検証手順:
1. `add_max_object` で `print <varname>_probe` を挿入し、対象 outlet と接続
2. **ユーザーに実機トリガを依頼**（「Learn ボタンを押してください」「Live のパラメータを動かしてください」等）
3. `get_console_log` で実出力を取得
4. 取得形式が下流の `route` / `unpack` 設計と一致するか照合
5. 不一致なら Phase 0 設計を修正して再構築
6. 検証完了後、プローブを削除（または `comment` に確認済み形式をメモ）

#### 5-3. 検証結果のドキュメント化

確認した出力形式・トリガ仕様を `comment` オブジェクトでパッチ内に記録:

```
// observer (selected_parameter) outputs: "id N" (id 0 = no selection, verified 2026-05-08)
// route id passes N. sel 0 filters zero.
// live.path needs bang from t_capture to refire on each Learn enable.
```

これにより次回の作業時（または別の Claude セッション）で再推測が不要になる。

#### Phase 5 完了基準

- [ ] 5-1 の起動経路チェック全項目クリア
- [ ] 5-2 で推測に基づいていた出力形式を全て `print` で実証確認
- [ ] 5-3 でパッチ内に動作仕様を comment で残した
- [ ] プローブを全て削除（または無効化）

### Why This Order Matters

- Phase 0 で設計制約を組み込むため、実装段階での手戻りが最小化される
- Phase 1 でセクションサイズが確定するため、Phase 2 で正確な配置計算ができる
- Phase 2 でセクション位置が確定するため、Phase 3 で patchcord 経路が安定する
- Phase 4 でパッチ全体の構造的一貫性を検証するため、設定漏れや順序ずれが出荷前に検出される
- **Phase 5 でデータが実際に流れることを実証するため、初期化漏れや形式推測ミスが Live ロード前に検出される**
- 後フェーズでの手戻りが最小化される

## Operation Checklists

各 MCP 操作の前後に実行すべき確認事項。スキルの原則を操作レベルで適用するためのチェックリスト。

### add_max_object の前（事前ルール）

**🔴 obj_type には省略形を渡す（[Object Text Conventions](reference/object-text-conventions.md) Section 1）**:

| 違反 | 正解 |
|---|---|
| `obj_type="trigger"` | `obj_type="t"` |
| `obj_type="select"` | `obj_type="sel"` |
| `obj_type="bangbang"` | `obj_type="b"` |
| `obj_type="int"` | `obj_type="i"` |
| `obj_type="float"` | `obj_type="f"` |
| `obj_type="send"` | `obj_type="s"` |
| `obj_type="receive"` | `obj_type="r"` |

**🔴 `obj_type="message"` は固定値で使わない**:
固定値は `obj_type="t"` / `prepend` / `zl.reg` のいずれかを使う。判定は [Execution Model & Messaging](reference/execution-and-messaging.md) 冒頭の Decision Tree に従う。

**🔴 Float コンテキストでは引数を Float リテラルに**:
- `pak`, `pack`, `scale`, `+`, `-`, `*`, `/`, `pow`, `expr`, `change` 等で Float を扱う場合、引数 `[0., 0.]` のように `.` を必ず付ける
- 違反例: `obj_type="pak", arguments=[0, 0]` → text が `"pak 0 0"` になり Int モード化

### add_max_object の後

**⚠ このチェックリストを飛ばした場合**: 後工程での修正以前に、設定し忘れによるバグが大量発生する。パラメータ名がデフォルト（"live.numbox[1]"）のまま、presentation が 1 のまま、_parameter_initial_enable が 0 のまま等、原因特定に膨大な時間と労力を費やすのは Claude 自身。生成直後に確認すれば数秒で済む作業を、後から探すと何倍もかかる。

1. `get_objects_in_patch` でオブジェクトのテキストを確認し、以下を機械的に検証:
   - `@` 構文のアトリビュートが反映されているか
   - **text に `trigger ` / `select ` / `bangbang` / `bangbang ` / 単独の `int ` / 単独の `float ` / `send ` / `receive ` が含まれていないか** → 含まれていれば `replace_object_text` で省略形に修正（例: `trigger b b b b l` → `t b b b b l`）
   - **maxclass が `message` のオブジェクトが新規追加されていないか** → 追加されている場合、固定値か動的構築か判定:
     - 固定値の場合: [Execution Model & Messaging](reference/execution-and-messaging.md) 冒頭の Decision Tree に従って `t` / `prepend` / `zl.reg` に置換（`remove_max_object` + `add_max_object` で再構築）
     - 動的構築（ユーザー入力をそのまま流す等）の場合: 容認するが、その理由を `comment` で残す
   - **Float コンテキストの `pak 0 0` / `pack 0 0` / `scale 0 1 ...` のような Int モード引数がないか** → あれば `replace_object_text` で `pak 0. 0.` 等に修正
   - **pattr の場合、第一引数で varname が強制上書きされていないか** → `pattr <name>` で生成すると varname が `<name>` になる。後続の `set_object_attribute` は実 varname を使う必要がある
2. ロジック用オブジェクト（trigger, prepend, route, zl, gate, live.object, live.path, pattr, pack, scale 等）は `set_object_attribute` で `presentation 0` を設定
3. live.* UI オブジェクトの場合:
   - `_parameter_longname` を varname に合わせて設定
   - `_parameter_initial_enable 1` を設定
   - `_parameter_initial` を設定
   - `get_object_attribute` で `_parameter_type`, `_parameter_unitstyle`, `_parameter_range` が意図通りか確認
4. live.text の場合: `text` / `texton` ラベルを設定
5. live.comment の場合: `replace_object_text` でテキストを設定
6. pattr の場合:
   - `parameter_enable 1`, `_parameter_invisible 1`, `_parameter_modmode 0`, `parameter_mappable 0` を設定
   - `_parameter_range` をデータに応じた範囲に設定（Float: `[-100000, 100000]` 等）
   - `_parameter_type` をデータ型に応じて設定（Float: 0, blob: 3 等）
   - `_parameter_initial_enable 1` を設定
   - **pattr は第一引数で varname が強制上書きされる**: `add_max_object` の戻り値の varname を確認してから後続操作を実施（例: `pattr param_min` を作ると `varname=param_min` になる、リクエストした varname は破棄される）

7. **LOM 系オブジェクト（live.path / live.object / live.observer / live.thisdevice）の場合**:
   - 🔴 [m4l-techniques の MUST 参照ルール](../../m4l-techniques/SKILL.md) に従い、関連 reference を **Read で先に取得** してから接続設計に入る
   - `live.path` / `live.observer` を追加した場合、起動トリガ経路（loadbang / live.thisdevice / sel bang 等）が Phase 0 設計に含まれているか即座に確認。なければ Phase 0 に戻る
   - 出力フォーマットが reference に明記されていない property を扱う場合、**`print <varname>_probe` を同時に追加し、Phase 5 で実機確認する旨をメモ**
   - 自デバイスのパラメータ操作を伴う場合（Learn 系）、`live.thisdevice` + `canonical_parent` フィルタ計画があるか Phase 0 設計と照合

### connect_max_objects の前

**⚠ このチェックリストを飛ばした場合**: outlet/inlet の役割を推測で接続し、動作しない接続を作成する。ユーザーに指摘されて切断→リファレンス確認→再接続のサイクルが発生し、1回の確認で済む作業が3回以上のやり取りになる。trigger の outlet 順序を間違えると、hot/cold の実行順序が逆転し、値が格納される前に pack が発火する等のロジック不具合が発生する。

1. `get_object_io_info` で接続元の outlet 数と接続先の inlet 数を確認
2. 接続先の inlet が hot (左) か cold (その他) かを確認（[Execution Model](reference/execution-and-messaging.md) 参照）
3. `trigger` から接続する場合:
   - 右→左の実行順序を確認
   - cold inlet に接続する値を右側 outlet に配置（先に発火 → 格納）
   - hot inlet に接続する値を左側 outlet に配置（最後に発火 → 処理開始）
4. `pattr` から接続する場合:
   - outlet 0 (左) = 値出力、outlet 1 (中央) = bindto（UIバインド）、outlet 2 (右) = dumpout
   - 値を下流に送る場合は outlet 0、UI にバインドする場合は outlet 1
5. **outlet/inlet の役割が不明なオブジェクトは、推測で接続せず `max-resources` スキルでリファレンスを必ず確認する**。推測による接続は手戻りの最大の原因となる

### レイアウト変更の後（Phase 8 検証）

**⚠ この検証を飛ばした場合**: 上向き接続、オブジェクト重複、パッチコードとオブジェクトの交差がユーザーの目視確認まで検出されない。全て座標データから機械的に検出可能な問題であり、ユーザーに指摘される前に自分で修正すべき問題。

一連のレイアウト操作が完了した後に**必ず**実行する:

1. `get_objects_in_patch` で全オブジェクトの矩形 (position + size) を取得
2. `get_patchlines` で全パッチコードの start_point, end_point, midpoints を取得
3. 以下を検出・修正:
   - **上向き接続**: `start_point.y > end_point.y` かつミッドポイントなし → オブジェクト移動または U-shape ミッドポイント追加
   - **オブジェクト重複**: 矩形同士の交差 → 位置調整
   - **パッチコードとオブジェクトの交差**: セグメントがオブジェクト矩形を通過 → ミッドポイント追加またはオブジェクト移動
   - **ミッドポイント付き接続**: セグメント単位で上向き区間がないか確認

### _parameter_order の設計

**⚠ 依存チェーンを考慮せず個別に order を設定した場合**: pack だけでなく、多くのオブジェクトの cold inlet に必要な値が代入されないままオブジェクトが動作（誤動作）する。メッセージの順序に起因するバグは原因が非常に分かりづらく、特定と修正に膨大な時間と労力を費やすことになる。_parameter_range が設定される前に値を復元すると、デフォルト範囲 [0, 127] でクランプされ、保存した値が失われる。

パラメータを設定する前に、復元依存チェーン全体を設計する:

1. `pack` / `pak` の hot/cold inlet 構造を特定
2. `scale` 等の複数 inlet オブジェクトで、hot inlet (0) と cold inlet の依存関係を特定
3. cold inlet に接続するパラメータに小さい order を割り当て（先に復元 → 格納）
4. hot inlet に接続するパラメータに大きい order を割り当て（後に復元 → 発火）
5. `_parameter_range` を設定するチェーンを先に復元
6. 独立パラメータは最後の order に配置
7. **設定後の読み戻し検証**: 全パラメータの `_parameter_order` を `get_object_attribute` で読み戻し、依存チェーンとの整合性を確認。Phase 4（完成検証）でも再検証する

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
