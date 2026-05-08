# LOM Observer Patterns

`live.observer` を使った監視・フィルタリングの実践パターン。
[LOM Applied Patterns](lom-patterns.md) の補足として、observer 固有の制御パターンをまとめる。

## 🔴 必読: 適用漏れ防止のためのアンチパターン

以下は Claude が頻繁に犯す誤実装。**「直感で組んだ接続」が以下に該当しないか必ずチェックする**。該当する場合は本ドキュメントの正規パターンに置き換える。

### ❌ Anti-pattern 1: live.path を live.observer に直結（トリガなし）

```
live.path live_set view → live.observer  // ← 動かない
```

**症状**: Learn ボタンを押しても何も起きない。observer 出力が一切発火しない。「id の処理が無視されている」ように見える。

**根本原因**: `live.path` は load 時に自動出力**しない**。明示的な `bang` または `path` メッセージを受けるまで沈黙する。

**正解**: 後述「live.observer の有効化・無効化パターン」の `t b b` を使用し、enable bang のたびに `live.path` を発火させる。

### ❌ Anti-pattern 2: observer 出力を route id でそのまま下流に渡す

```
live.observer → route id → t_capture → ...  // ← 重複・バーストで誤動作
```

**症状**: パラメータ選択するたびに複数回処理が走る。同じパラメータを選び直しても再アサインされる。Learn が意図せず連続発火する。

**根本原因**: `selected_parameter` 等は連続通知やバースト出力が発生する。

**正解**: 後述「selected_parameter フィルタチェーン」の `change 0 → thresh 0 → zl.ecils 1` を必ず通す。

### ❌ Anti-pattern 3: 出力フォーマットを推測して route 設計

```
// "たぶん id N が来るはず" と推測して
live.observer → route id → ...
```

**症状**: 構文的には正しいが route のどの分岐にもマッチせず黙って消える。デバッグ困難。

**根本原因**: property の型により出力形式が異なる。reference に書かれていない property は実機確認が必須。

**正解**: 構築直後に `print obs_probe` を一時挿入し、`get_console_log` で実形式確認 → 確認した形式に合わせて route 設計。これは [patch-guidelines](../../patch-guidelines/SKILL.md) Phase 5 で必須実行。

### ❌ Anti-pattern 4: 自デバイス除外フィルタなしで Learn を構築

```
live.observer (selected_parameter) → そのまま target に set id
```

**症状**: control_dial / curve_n 等の自デバイスパラメータを誤ってターゲットにできてしまう。フィードバックループや無限ループ。

**正解**: 後述「自デバイスパラメータの除外（canonical_parent チェック）」を必ず組み込む。

---

## live.observer の有効化・無効化パターン

`live.observer` の有効化・無効化は、右インレット（id 設定）で制御する。`id 0` を送ると監視が停止する。

`live.observer` は右インレットに `id N` を受け取ると監視対象を設定し、`id 0` を受け取ると監視を解除する。`sel` の非一致アウトレットから `0` を `prepend id` 経由で送ることで、シンプルに制御できる。

```
learn_toggle → sel 1
  ├→ outlet 0 (一致=1): 有効化チェーン
  │   t b b → zl.reg property selected_parameter → observer inlet 0
  │         → zl.reg path live_set view → live.path → observer inlet 1
  └→ outlet 1 (非一致=0): prepend id → observer inlet 1 (id 0 で無効化)
```

**有効化**: `sel 1` の outlet 0 が bang を出力し、`property` と `id` の両方を設定
**無効化**: `sel 1` の outlet 1 が `0` を出力し、`prepend id` で `id 0` に変換 → observer を停止

## selected_parameter フィルタチェーン

`live.observer` で `selected_parameter` を監視する場合、以下のフィルタチェーンを適用する。

`selected_parameter` の変更通知は、id 以外のデータ（プロパティ情報等）を含む場合があり、同じパラメータの連続通知や短時間のバースト出力も発生する。フィルタなしで処理すると誤動作や重複処理の原因となる。

```
live.observer → route id → change 0 → thresh 0 → zl.ecils 1 (outlet 1) → prepend id → gate
```

**各オブジェクトの役割**:

| オブジェクト | 役割 |
|---|---|
| `route id` | `id` プレフィックス付きのメッセージのみ通過。それ以外を排除 |
| `change 0` | 同じ id の連続通知を排除（重複防止） |
| `thresh 0` | 短時間に連続する出力をまとめ、最後の値のみ通過（バースト対策） |
| `zl.ecils 1` | リストの末尾1要素を切り出す。**右アウトレット（outlet 1）を使用**。左アウトレットは残り（空リスト）を出力するため使わない |
| `prepend id` | 数値を `id N` 形式に変換し、下流の gate / live.object に送る |
| `gate 1 0` | Learn ボタンの ON/OFF で通過を制御（初期状態: 閉） |

## 自デバイスパラメータの除外（canonical_parent チェック）

Learn 機能で `selected_parameter` を取得する際、自デバイスのパラメータを操作対象から除外する。

自デバイスのパラメータ（control_dial, curve_n 等）を操作対象にすると、フィードバックループや意図しない動作が発生する。別インスタンスの ParameterMapper のパラメータは正常に操作対象にできる（各インスタンスは固有の device id を持つため）。

**判定方法**: `canonical_parent`（LOM の子ナビゲーション）で、パラメータの親 Device id を取得し、自デバイスの id と比較する。

**初期化チェーン** (デバイスロード時に this_device id を取得):

```
live.thisdevice → zl.reg "path live_set this_device" → live.path → route id → != 右inlet
```

**フィルタチェーン** (zl.ecils と prepend id の間に挿入):

```
zl.ecils outlet 1 → N
  ↓
t b i
  outlet 1 (1st): N → t i i
  │                     outlet 1 (1st): N → prepend id → live.object 右inlet (ターゲット設定)
  │                     outlet 0 (2nd): N → * 右inlet (保存)
  outlet 0 (2nd): bang → zl.reg "get canonical_parent" → live.object 左inlet
                                                           ↓ canonical_parent id M
                                                         route canonical_parent → route id → M
                                                           ↓
                                                         != this_device_id → 0(同一) / 1(異なる)
                                                           ↓
                                                         * 左inlet (hot) → N * result
                                                           ↓
                                                         route 0
                                                           outlet 0 (=0): 自デバイス → 破棄
                                                           outlet 1 (≠0): 外部パラメータ → prepend id → learn_gate
```

**ポイント**:
- `canonical_parent` は `live.object` の `get canonical_parent` で取得する（`live.path goto` ではない）
- `!=` の右インレットに init チェーンから this_device_id を設定。初期値 0 のため、init 前は全パラメータ通過（安全側）
- 乗算フィルタ: `*` で条件フィルタ。`gate + i (store/recall)` より簡潔
- `t i i` で N を分配: outlet 1 → prepend id（ターゲット設定）、outlet 0 → * 右inlet（値保存）
