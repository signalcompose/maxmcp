---
name: organize-patch
description: |
  Organize and tidy up Max/MSP patch layout. Use this skill when:
  - User asks to organize, tidy, or clean up a patch layout
  - After completing a series of patch edits (add/remove/connect objects)
  - Automatically triggered for partial layout adjustment after individual operations
  - Optimizing patchcord routing and object positioning
user-invocable: true
---

# Organize Patch - MaxMCP パッチレイアウト整理スキル

MaxMCP で作成・編集した Max/MSP パッチのレイアウトを自動整理するスキル。
オブジェクトの配置、サイズ、パッチコードの経路を最適化し、可読性の高いパッチに整形する。

## 動作モード

このスキルは2つのモードで使用する。`patch-guidelines` の4フェーズワークフローに対応。**セクション内モードがデフォルト**。

### セクション内モード（デフォルト）

単一セクション内のオブジェクトのみを対象に整理する。セクション外のオブジェクトには影響しない。

- **用途**: 各セクションの開発完了時にセクション内レイアウトを確定させる
- **対象**: 指定セクション内のオブジェクトとセクション内パッチコードのみ
- **Phase 8 検証**: セクション内に限定して実行
- **結果**: セクションのサイズ（幅・高さ）が確定する
- **サイズ変更時の自動遷移**: 整理前後でセクションの幅または高さが変わった場合、`patch-guidelines` Phase 2（セクション配置）を実行し、下流・隣接セクションの位置を調整する

### フルパッチモード

パッチ全体を対象に整理する。従来の動作と同等。

- **用途**: セクション間接続完了後の最終整理、または明示的に全体整理を要求された場合
- **対象**: パッチ内の全オブジェクトとパッチコード

## 起動条件

### 手動起動
- ユーザーが「パッチを整理して」「レイアウトを整えて」等を要求した時 → **セクション内モード**（直前に変更したセクションを対象）
- ユーザーが「パッチ全体を整理して」等、明示的に全体を指定した時 → **フルパッチモード**
- ユーザーが「このセクションを整理して」等を要求した時 → **セクション内モード**

### 自動起動（部分整理）
以下の操作を行った後、影響を受けるオブジェクトとその周辺のパッチコードに対して自動的にレイアウト調整を実行する:

- **オブジェクトの追加**: 新オブジェクトを適切な位置に配置し、接続先の inlet/outlet 整列を調整。ロジック専用オブジェクトは `presentation 0` に設定
- **オブジェクトの削除**: 削除により空いたスペースを詰めるか、周辺オブジェクトの位置を調整
- **パッチコードの接続・切断**: 新しい接続のパッチコード経路を調整。必要に応じてオブジェクトの位置調整またはミッドポイント追加
- **オブジェクトのテキスト変更**（`replace_object_text`）: サイズ変更に伴う inlet/outlet 位置の変化を反映し、接続先との整列を再調整
- **オブジェクトの移動・リサイズ**（`set_object_attribute` で `patching_rect` / `presentation_rect` を変更）: 移動先で周辺オブジェクトとの整列を調整

**プレゼンテーション管理**: オブジェクトの追加時、UIとして表示が不要なオブジェクト（ロジック用の trigger, prepend, pack, route, pattr 等）は `presentation 0` に設定する。プレゼンテーションに含めるのは、ユーザーが直接操作・閲覧する UI オブジェクト（live.dial, live.numbox, live.text, textedit, live.comment 等のラベル）のみ

**Phase 8 検証のタイミング**: 個々の操作ごとに Phase 8 検証を実行しない。一連の操作（オブジェクト追加・移動・接続等）が完了した後にまとめて1回実行する。これによりトークン消費と往復回数を削減する

**パッチ読み込み直後の検証は不要**: ユーザーがパッチを開き直した（リロードした）直後は、位置検証（Phase 8）を自動実行しない。パッチの状態は保存時のまま維持されているため、リロードだけでレイアウトが崩れることはない。検証はユーザーが明示的に要求した場合、または一連の操作が完了した後にのみ実行する

## 前提条件

- MaxMCP が接続済みであること
- 対象パッチが `list_active_patches` で取得可能であること
- `patch-guidelines` スキルがロード済みであること（レイアウトルール参照）

## ワークフロー

### モード別の Phase 適用

| Phase | セクション内モード | フルパッチモード |
|-------|:---:|:---:|
| Phase 1: パッチ分析 | 対象セクションのみ | パッチ全体 |
| Phase 2: セクション分割 | スキップ（セクション確定済み） | 実行 |
| Phase 3: メインフロー特定 | セクション内で特定 | パッチ全体で特定 |
| Phase 4: オブジェクトサイズ調整 | 対象セクションのみ | 全オブジェクト |
| Phase 5: オブジェクト再配置 | 対象セクション内のみ | 全オブジェクト |
| Phase 6: パッチコード経路最適化 | セクション内パッチコードのみ | 全パッチコード |
| Phase 7: セクションヘッダー追加 | 対象セクションのみ | 全セクション |
| Phase 8: 重複・交差検証 | 対象セクション内のみ | パッチ全体 |

### Phase 1: パッチ分析

1. **オブジェクト取得**: `get_objects_in_patch` で全オブジェクトの位置・サイズ・テキストを取得
2. **接続取得**: `get_patchlines` で全パッチコードの始点・終点・ミッドポイントを取得
3. **信号フロー解析**: 接続情報からメインフロー（最長の処理チェーン）を特定

### Phase 2: セクション分割

接続のトポロジーに基づき、パッチを論理セクションに分割する:

- 各セクションは機能的にまとまったオブジェクト群
- `send` / `receive` の境界でセクションを分ける
- 典型的なセクション例:
  - Init（初期化）
  - Input/UI（入力・ユーザーインターフェース）
  - Processing（信号処理）
  - LOM（Live Object Model 操作）
  - Output（出力）

### Phase 3: メインフロー特定とレイアウト設計

1. **メインフロー特定**: 各セクション内で最も処理ステップが多いチェーンをメインフローとする
2. **レイアウト方針決定**:
   - メインフローを垂直の背骨として配置
   - 副入力はメインフローに対して斜め接続（許容範囲内）
   - セクション間は 80-100px の間隔
3. **上→下フローの確保**: 可能な限り全ての接続が上から下に流れるようセクション順序を決定

### Phase 4: オブジェクトサイズ調整

各オブジェクトのテキスト内容に基づきサイズを調整する。幅の計算式・標準サイズは [Layout Rules](../patch-guidelines/reference/layout-rules.md) の Object Dimensions セクションを参照。

**サイズ調整のルール**:
- テキスト内容が収まる最小幅に設定（余白含む）
- `live.dial`, `live.numbox`, `live.text` 等の UI オブジェクトは固有サイズを維持
- `panel` オブジェクトはプレゼンテーション専用のためパッチングサイズは変更しない
- メインフロー上の接続オブジェクトは、outlet/inlet の X 座標が揃うよう幅を統一

**サイズ確認の注意**:
- `fontsize` を変更すると `live.numbox`, `live.comment`, `live.text` 等のサイズが自動変更される
- `appearance` を変更すると `live.dial` のサイズが変わる
- サイズを設定した後は `get_object_attribute` で `patching_rect` / `presentation_rect` の実際値を確認する（設定値と異なる場合がある）

### Phase 5: オブジェクト再配置

`set_object_attribute` で `patching_rect` を設定。すべてのルールは [Layout Rules](../patch-guidelines/reference/layout-rules.md) を参照:
- Spacing Guidelines — グリッド間隔、最小間隔、L字型ギャップ
- Multi-Outlet Width Alignment Formula — マルチアウトレット幅計算
- Comments and Documentation — セクションヘッダー配置

**配置順序**:
1. メインフローのオブジェクトを垂直に配置（同一 X 座標）
2. 副入力・分岐先を適切な X オフセットで配置
3. セクション間の接続を考慮して Y 座標を調整
4. 末端まで全フローをトレースし、全ルールを一括適用した結果としてレイアウトを確定する

### Phase 6: パッチコード経路最適化

`set_patchline_midpoints` でパッチコードの経路を設定。すべてのルールは [Layout Rules](../patch-guidelines/reference/layout-rules.md) の Patchcord Management セクションを参照:
- Routing Decision Flowchart — 直線/L字/U字の判定
- Detour Priority — 迂回時のオブジェクト移動優先
- Gap Routing — ギャップ通過ルーティング計算
- Fanout Ordering — 複数L字型の重複回避

### Phase 7: セクションヘッダー追加

各セクションの先頭に `comment` オブジェクトを配置:

```
--- Section Name ---
```

- 既存のセクションコメントがあれば再利用
- 位置: セクション最上部オブジェクトの 15px 上
- **パッチコードとの干渉回避**: セクション先頭オブジェクトに上からパッチコードが入る場合、コメントをパッチコードの経路と重ならない位置（左側にオフセット等）に配置する。パッチコードの下にコメントが隠れて読めなくなることを防ぐ

### Phase 8: 重複・交差検証（必須・省略不可）

レイアウト完了後、全オブジェクトとパッチコードの重複・交差がないか検証する。**このフェーズは省略できない。** セクション構築完了時、およびレイアウト変更後に必ず実行する。

**検証手順**:
1. `get_objects_in_patch` で全オブジェクトの `position` と `size` を取得
2. `get_patchlines` で全パッチコードの `start_point`、`end_point`、`midpoints` を取得
3. 以下の5種類の干渉を検出する

**0. 上向き接続の検出**:
- 全パッチコードの `start_point.y` と `end_point.y` を比較
- `start_point.y > end_point.y` かつ `num_midpoints == 0` の接続 = 上向き接続（禁止）
- ミッドポイント付きの接続も、各セグメント単位で上向き区間がないか確認
- 上向き接続を検出した場合: オブジェクト移動で解消するか、U-shape ミッドポイント（4点）で外部レーンを経由させる

**1. オブジェクト同士の重複**:
- 各オブジェクトの矩形 (x, y, x+width, y+height) を比較
- 矩形が重なるペアがあれば位置を調整

**2. パッチコードとオブジェクトの交差**:
- パッチコードの各セグメント（始点→M1、M1→M2、...、Mn→終点）を直線として計算
- 各直線が通過するオブジェクトの矩形と交差しないか確認
- 交差がある場合、オブジェクトの位置をずらすか（優先）、ミッドポイントで迂回する
- **例外**: パッチコードの始点・終点のオブジェクトは交差として検出しない

**3. パッチコード同士の重複**:
- 同じ y 座標に水平セグメントが複数ある場合、x 範囲が重なっていないか確認
- 同じ x 座標に垂直セグメントが複数ある場合、y 範囲が重なっていないか確認
- 重複がある場合、一方のミッドポイントを 5px 以上ずらして分離する

**4. プレゼンテーションモードの重複**:
- `presentation 1` のオブジェクトについて、`presentation_rect` を取得
- 各オブジェクトの presentation 矩形同士が重なっていないか確認（意図的な重ね配置を除く）
- オブジェクトの `patching_rect` 変更時に `presentation_rect` が意図せず変わっていないか確認（live.* オブジェクトは showname/shownumber 等の変更でサイズが自動変更される場合がある）

**修正後の再検証**: 修正を行った場合、影響を受ける周辺のパッチコード・オブジェクトについて再度検証する。プレゼンテーションモードの検証も含む

## 入力パラメータ

| パラメータ | 必須 | 説明 |
|---|---|---|
| patch_id | Yes | 整理対象のパッチID（`list_active_patches` または `get_frontmost_patch` で取得） |
| section_varnames | No | セクション内モード時、対象セクションに含まれるオブジェクトの varname リスト。省略時はフルパッチモードで動作 |

## 使用する MCP ツール

| ツール | フェーズ | 用途 |
|---|---|---|
| `get_objects_in_patch` | 分析 | 全オブジェクト情報取得 |
| `get_patchlines` | 分析 | 全接続情報取得 |
| `get_object_io_info` | 分析 | inlet/outlet 数の確認 |
| `set_object_attribute` | 再配置 | `patching_rect` でサイズ・位置変更 |
| `set_patchline_midpoints` | 経路最適化 | パッチコードのミッドポイント設定 |
| `add_max_object` | ヘッダー | セクションコメント追加 |
| `replace_object_text` | ヘッダー | コメントテキスト設定 |
| `remove_max_object` | クリーンアップ | 不要コメントの削除 |

## 参照スキル・ドキュメント

- `patch-guidelines` > `reference/layout-rules.md` — グリッド・間隔ルール
- `patch-guidelines` > `reference/mcp-notes.md` — MCP ツール固有の注意事項
- `patch-guidelines` > `reference/presentation-layout.md` — プレゼンテーションモード（変更しない）
- `patch-guidelines` > `reference/visual-design.md` — パネル・色設計（変更しない）

## 制約事項

- **プレゼンテーションモードの積極的変更はしない**: `presentation_rect` の位置・サイズを意図的に変更しない。ただし `patching_rect` の変更に伴い `presentation_rect` が意図せず変化する場合があるため、Phase 8 で検証し、変化が検出された場合は元の値に復元する
- **接続は変更しない**: パッチコードの追加・削除は行わない（ミッドポイントのみ操作）
- **オブジェクトの追加・削除は最小限**: セクションコメントの追加のみ許可
- **send/receive は変更しない**: 既存の send/receive 構造を維持

## 実行例

```
User: パッチを整理して
Assistant:
  1. get_objects_in_patch → 47 objects
  2. get_patchlines → 54 connections
  3. 5 sections identified: Init, Learn, ParamInfo, UI, Processing
  4. Main flow: Learn observer chain (12 objects, vertical)
  5. Resizing 30 objects to content-appropriate widths
  6. Repositioning all objects (top-to-bottom flow)
  7. Adding midpoints to 2 upward connections (U-shape)
  8. Adding 5 section header comments
  Done: patch organized
```
