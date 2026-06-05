# レイアウト検証・配置支援ツール 設計提案（引き継ぎ資料）

**作成日**: 2026-06-03
**対象読者**: MaxMCP を開発する Claude / 開発者
**ステータス**: 実装中（Section 7 の Step 1 `geometry.{h,cpp}`、Step 2 `validate_layout`、Step 3 `get_io_position`、Step 4 `suggest_alignment` / `align_objects` は実装済み。Step 5 `render_patch_preview` 以降は未着手）

---

## 0. この資料の目的

MaxMCP を使った M4L デバイス構築の実運用（ParameterMapper の構築）で、**オブジェクト配置（レイアウト）整理が最も手戻りの多い工程**だと判明した。本資料は、その原因分析と、解決のためにサーバ側へ追加すべきツール群の設計を、後続の実装担当（Claude）へ引き継ぐためのもの。

### 何が問題だったか（実例ベース）

レイアウト整理は現在 `organize-patch` スキルの **Phase 8（重複・交差検証）** として規定されているが、その実体は **「AI が座標を読んで頭の中で幾何計算する手順」** である。実運用で次の失敗が繰り返された:

1. **outlet/inlet を縦に揃える幅の計算ミス**
   - 現状 AI は近似式 `pos_i = left + 9.5 + (w-19)*i/(n-1)` を手計算している。これは**オブジェクト種別ごとに異なる端インセットを無視した近似**であり、ズレる。
2. **パッチコードがオブジェクトを横切るかの判定ミス**
   - 直線（コード）と矩形（オブジェクト）の交差を AI が手計算。実際に `live.object` の幅決定で1回間違え、ユーザーに「まだ route min max name に被っている」と指摘された。
3. **重なり・上向き接続の見落とし**
   - 全ペアの矩形比較・全コードの y 比較を目視で行うため、オブジェクト数が増えると漏れる。

### 根本原因と方針

これらは **「決定論的に計算可能なのに LLM に推論させている」** ことが原因。散文の手順書を増やしても精度は上がらない。**幾何計算をサーバ側（C++、ユニットテスト可能）へ移し、AI には検証済みの結果とフィードバックループを与える**のが正しい解決策。

> 重要: データ取得の足回りは既にある。`get_objects_in_patch {mode:"layout"}` が `position`/`size` を、`get_patchlines {mode:"geometry"}` が `start_point`/`end_point`/`midpoints` を返す（[mcp-tools-reference.md](mcp-tools-reference.md) L142, L477 参照）。**足りないのは判定ロジックそのもの**。

---

## 1. 提案ツール一覧（優先度順）

| 優先 | ツール | 役割 | 解消する失敗 |
|---|---|---|---|
| ★★★ | `validate_layout` | Phase 8 をサーバ側で機械判定し、所見リストを返す | 上記 2, 3 |
| ★★★ | `get_io_position` | inlet/outlet 座標を**較正済みの計算で返す**（SDK の矩形＋個数＋drawfirstin から算出。SDK に位置の直接 getter は無い） | 上記 1 |
| ★★☆ | `suggest_alignment` | outlet↔inlet を揃える幅・位置を計算して返す | 上記 1 |
| ★★☆ | `render_patch_preview` | パッチの簡易描画（ASCII/SVG/PNG）を返し AI に「見せる」 | 全般（自己修正） |
| ★☆☆ | `apply_layout_fixes` | `validate_layout` の所見を自動修正（U字迂回・ずらし） | 全般（将来） |

---

## 2. `validate_layout`（最優先）

### 目的
`organize-patch` Phase 8 の全チェックをサーバ側で実行し、構造化された所見を返す。AI は「保存前に必ずこれを通す」フィードバックループとして使う。**読み取り専用**（パッチを変更しない）。

### 入力

```json
{
  "patch_id": "string (required)",
  "scope_varnames": ["string"],      // 省略時は全体。指定時はそのオブジェクト群に限定（セクション内モード）
  "mode": "patching | presentation", // 既定 patching。presentation 時は presentation_rect / presentation==1 のみ対象
  "checks": ["upward","overlap","cord_object","cord_cord","presentation_overlap"], // 省略時は全部
  "epsilon": 2.0                      // 許容誤差(px)。省略時 2.0
}
```

### 出力

```json
{
  "patch_id": "...",
  "clean": false,
  "summary": {"upward":1,"overlap":2,"cord_object":1,"cord_cord":0,"presentation_overlap":0},
  "findings": [
    {
      "type": "cord_object",
      "severity": "error",
      "cord": {"src_varname":"target_id_prefix","outlet":0,"dst_varname":"set_obj","inlet":1},
      "object": "prop_route",
      "detail": "cord segment passes through object rect; crossing at (876.8, 860.0)",
      "suggestion": "widen set_obj so inlet 1 x >= 961, or add midpoint to detour right of prop_route"
    },
    {
      "type": "overlap",
      "severity": "error",
      "objects": ["curve_scale","disp_pak"],
      "detail": "patching_rect overlap area 120 px^2 (x:679-690 y:1180-1200)"
    },
    {
      "type": "upward",
      "severity": "warning",
      "cord": {"src_varname":"a","outlet":0,"dst_varname":"b","inlet":0},
      "detail": "start.y 1360 > end.y 800, num_midpoints 0"
    }
  ]
}
```

`findings` が空配列なら `clean:true`。AI はこれを見て修正→再検証を回す。

### 各チェックのアルゴリズム（実装者向け詳細）

座標系: パッチング座標は左上原点・下方向 +y。矩形は `[x, y, w, h]`、`right=x+w`, `bottom=y+h`。

**0. upward（上向き接続）**
- 各 patchline について、ミッドポイントを含むポリラインを segment 列に分解（start→m1→…→mn→end）。
- 各 segment で `seg.start.y - seg.end.y > epsilon` なら上向き。ミッドポイント無しのコードが上向きの場合は severity=error、ミッドポイント有り（迂回意図あり）の中間 segment は warning。
- 注意: ほぼ水平（|dy|<=epsilon）は除外。

**1. overlap（オブジェクト重複）— AABB 交差**
- 全ペア (i,j) で:
  `overlap = ! (A.right <= B.x + eps || B.right <= A.x + eps || A.bottom <= B.y + eps || B.bottom <= A.y + eps)`
- 重なり面積も算出して detail に含める。
- O(n²) で可（典型 < 200 個）。将来は格子バケットで高速化。

**2. cord_object（コード×オブジェクト交差）— 線分 vs 矩形（slab/Liang-Barsky）**
- 各コードの各 segment について、**コードの始点/終点オブジェクト（src/dst varname）を除く**全オブジェクト矩形と交差判定。
- 線分 P0→P1 と矩形 [xmin,xmax]×[ymin,ymax] の交差は Liang-Barsky:
  - `d = P1 - P0`。`p = [-dx, dx, -dy, dy]`, `q = [P0.x-xmin, xmax-P0.x, P0.y-ymin, ymax-P0.y]`。
  - 各 k で `p==0 && q<0` なら平行かつ外側→非交差。`t = q/p` を u1(入)/u2(出) に振り分け。`u1>u2` なら非交差。`0<=u1<=u2<=1` の区間が矩形内。
- 交差ありなら、交差開始点 (P0 + u1·d) を detail に記録。
- 端インセット（コードは inlet/outlet の実座標から出る）を正確にするため、**始点/終点には `get_io_position` と同じ SDK 実座標を使う**こと（近似式を使わない）。

**3. cord_cord（コード同士の重なり）**
- 軸平行 segment 同士のみ対象（垂直×垂直で同 x±eps、水平×水平で同 y±eps）。区間が重なれば所見。斜め同士は実害が薄いので既定では対象外（オプション化可）。

**4. presentation_overlap**
- `mode:"presentation"` 時、`presentation==1` のオブジェクトの `presentation_rect` に対して overlap と同じ判定。意図的な重ね（panel 背景など）は varname/maxclass で除外できるよう除外リスト引数を将来追加。

### 実装上の指針
- 追加場所: `src/tools/layout_tools.{h,cpp}` を新設（`object_tools` / `connection_tools` と同じ構成）。`mcp_server.cpp` のツール登録に追加。
- **幾何計算は `src/utils/geometry.{h,cpp}` に純粋関数として切り出し、`tests/` でユニットテスト必須**。本ツールの価値は「LLM から幾何を奪う」ことなので、その計算が信頼できることをテストで担保するのが肝。
- データ取得は既存の layout / geometry 抽出経路（`patch_helpers`, `connection_tools`）を再利用。MCP 往復ではなく内部関数として共有構造体を取り回す。

---

## 3. `get_io_position`（最優先・近似式を「較正済み実装」に置き換える）

### 目的
AI が毎回その場で計算している近似式 `pos_i = left + 9.5 + (w-19)*i/(n-1)` を**廃止**し、サーバ側の**1つの較正済み・テスト済み実装**に集約する。これで幅合わせの推測ミスが消える。

> ⚠️ **重要な前提（SDK 調査済み）**: Max SDK に **特定 inlet/outlet のピクセル座標を返す公開関数は存在しない**。確認できたのは `jbox_get_patching_rect`/`_position`/`_size`、`inlet_count()`/`outlet_count()`（個数）、`jbox_get_drawfirstin`（先頭 inlet 描画有無）、`jbox_getinlet/getoutlet`（inlet/outlet の**オブジェクトポインタ**、位置ではない）まで。`t_jbox` 内部の `d_inletvoffset` 等は存在するが「Do not access its members directly」と明記され ABI 依存で非推奨。
>
> したがって本ツールは「SDK から真値を引く」のではなく、**Max の inlet/outlet 等間隔配置ルールを決定論的に再現し、定数を実機較正してテストで固定する**もの。価値は「LLM の毎回計算」を「1回書いて較正・テストした計算」へ移すことにある。

### 入力 / 出力
```json
// 入力
{"patch_id":"...", "varname":"curve_scale", "side":"inlet|outlet"}
// 出力
{"varname":"curve_scale","side":"inlet",
 "count":6,
 "positions":[{"index":0,"x":462.5,"y":1180.0}, {"index":1,...}, ...]}
```

### 実装メモ
- 入力データ: `jbox_get_patching_rect`（矩形）＋ `inlet_count()`/`outlet_count()`（個数）＋ `jbox_get_drawfirstin`（先頭 inlet を描画しない種別の補正）。
- 計算: Max は inlet を上辺に、outlet を下辺に等間隔配置する。`n==1` は端から一定オフセット、`n>=2` は左端〜右端に等間隔。中心 x を算出して返す（上辺 y = `rect.y`、下辺 y = `rect.y + rect.height`）。
- **定数の較正が必須**: inlet の視覚幅・端インセット（近似式の `9.5` / `19` に相当）は推測で固定せず、**代表的なオブジェクト（newobj、live.* UI、numbox 等）の実機 inlet 位置を測って決め、ユニットテストにフィクスチャとして焼き込む**。種別差（UI オブジェクトと newobj で見た目幅が違う）に注意。
- `drawfirstin` が false の種別は視覚 inlet 数が論理数とずれるため補正する。
- `validate_layout` との関係（**実装時の確定事項**）: `validate_layout` は既存コードの cord 端点を `jpatchline_get_startpoint/endpoint`（Max の実座標＝グラウンドトゥルース）で取得しており、近似式より正確。よって**統合しない**。`get_io_position` の式は「未接続の nub の座標を算出する」用途のフォールバックであり、両者は `geometry::Rect/Point` という共通語彙を介して整合する。
- 限界の明示: これは Max 内部描画の再現であり、Max 側の仕様変更で定数がずれる可能性がある。テストが回帰検出器になる。

### 実装結果（2026-06-04, Max 9 で校正）

- 代表 8 種（newobj `cycle~`/`pak`, `number`, `live.numbox`, `live.dial`, `gain~`, `slider`, `live.gain~`, `toggle`）をコード端点で実測した結果、**`edge_inset = single_inset = 9.5` が全種で普遍**。per-class テーブルは不要（`io_calibration_for` は将来の拡張フックとして空テーブルで保持）。
- inlet y = `rect.y` / outlet y = `rect.bottom()` も普遍で、**縦長オブジェクトの特別扱いは不要**（高さは下辺に織り込まれる）。
- 唯一の例外: 幅が極端に狭い場合（`gain~` 幅22px）に nub が ~0.5px 丸められる。幅を広げると消える。フィクスチャに記録済み。
- 純粋実装は `src/utils/io_geometry.{h,cpp}`、フィクスチャは `tests/unit/test_io_geometry.cpp`、グルーは `src/tools/layout_tools.cpp`。

---

## 4. `suggest_alignment`（配置プリミティブ）

### 目的
「この outlet を、この inlet と同じ x に揃えたい」を一発で解く。

### 例: outlet↔inlet を縦一直線にする幅を返す
```json
// 入力
{"patch_id":"...",
 "anchor":{"varname":"curve_n","side":"outlet","index":0},   // 揃える基準
 "target":{"varname":"curve_scale","side":"inlet","index":5}} // これが anchor.x に乗るよう調整
// 出力
{"recommended_patching_rect":[453.0,1180.0,226.0,20.0],
 "rationale":"inlet5 x must equal 669.5; with left=453 → width=226"}
```
- 内部で `get_io_position` を使い、target の inlet index が anchor.x に一致する width（または left）を逆算して返す。
- 付随する `align_objects`（`align_left`/`align_right`/`align_top`/`align_bottom`、`align_hcenter`/`align_vcenter` 揃え・`distribute_h`/`distribute_v` 等間隔分配）も実装済み。複数オブジェクトの共有 bounding box を基準に、移動するオブジェクトの推奨 `patching_rect` のみを read-only で返す。

---

## 5. `render_patch_preview`（視覚フィードバック）

### 目的
現在 AI は**パッチを見られず座標から想像している**。結果を「見て直す」手段を与えると自己修正力が大きく上がる。

### 段階的な実現案
1. **ASCII / SVG スキーマ図（低コスト・まず着手）**: オブジェクト矩形を縮尺してテキスト/SVG で描画。varname ラベルとコードを線で表現。所見（重なり・交差）をハイライト。MCP の text/`image` コンテンツで返す。
2. **PNG スクリーンショット（理想）**: Max の `jpatcher` からパッチウィンドウを画像化（`jgraphics` / patcher snapshot の可否を調査）。AI はマルチモーダルなので PNG が最も効く。
- パッチング/プレゼンテーションの両モードを切替可能に。

---

## 6. `apply_layout_fixes`（将来・任意）
`validate_layout` の所見を入力に、安全な自動修正を適用:
- 上向きコード → U 字ミッドポイント（外周レーン経由）で解消
- 軽微な重なり → 一方を最小移動でずらす
- cord×object 交差 → ミッドポイント迂回 or オブジェクト移動の提案を適用
リスクがあるため、まず提案（dry-run）→ 承認 → 適用の二段構えを推奨。

---

## 7. 実装順序の推奨

1. **`src/utils/geometry.{h,cpp}` + ユニットテスト**（AABB 交差・線分×矩形・線分重なり・上向き判定）
2. **`validate_layout`**（上記 geometry を使用）— 失敗の第1原因（交差・重なり・上向き見落とし）を構造的に除去
3. **`get_io_position`** — 幅合わせ近似式を撲滅
4. **`suggest_alignment` / `align_objects`**
5. **`render_patch_preview`**（ASCII → PNG）
6. **`apply_layout_fixes`**（将来）

## 8. 受け入れ基準（テスト）

- **ユニット**: 各幾何述語を既知フィクスチャで検証（交差する/しない境界、epsilon 境界、ミッドポイント付きコード）。
- **統合**: 意図的に壊したパッチ（重なり1・上向き1・交差1）で所見が過不足なく出る／クリーンなパッチで `clean:true`。
- **フィクスチャ**: ParameterMapper パッチ（本リポジトリ外で構築済み、49 オブジェクト）を回帰テスト素材として `test-patches/` に取り込むとよい。

## 9. 連動して更新すべきドキュメント

- [mcp-tools-reference.md](mcp-tools-reference.md): 新ツールの I/O 仕様を追記（ツール数 26 → +N）。
- [INDEX.md](INDEX.md): 索引更新。
- **`plugins/maxmcp/skills/organize-patch/SKILL.md` Phase 8**: 「AI が座標を読んで手で判定」する記述を「`validate_layout` を呼び、所見を修正→再検証」に置き換える。これが本対応の最終目的（手順書ではなくツール＋運用ルール化）。
- `plugins/maxmcp/skills/patch-guidelines/reference/layout-rules.md`: 幅合わせ近似式の記述を「`get_io_position` を使う」に改める。

---

## 付録: 今回の根拠となった実測値（参考フィクスチャ）

ParameterMapper（patch id 例 `patch_altXO9Sx`）で確認された実例:
- `target_id_prefix` out0 (839.5, 700) → `set_obj` in1 のコードが `prop_route`（route min max name, x520–869, y860）を横切る問題。width 400（in1=843.5）では `param_name` 列は回避できても `prop_route` を回避できず、width 550（in1=993.5）でコード x=876.8 @ y860 となり 869 を 7.8px クリアして解決。
  → **この種の逆算こそ `suggest_alignment` + `validate_layout` が機械的に出すべきもの**。
- `scale` の `classic` 既定値問題（別件、[object-text-conventions.md Section 3] に反映済み）も、`validate_*` 系とは別に「アトリビュート整合検証ツール」の候補になりうる（将来の拡張余地）。
