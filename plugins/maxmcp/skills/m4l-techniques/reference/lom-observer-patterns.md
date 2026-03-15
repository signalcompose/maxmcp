# LOM Observer Patterns

`live.observer` を使った監視・フィルタリングの実践パターン。
[LOM Applied Patterns](lom-patterns.md) の補足として、observer 固有の制御パターンをまとめる。

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
                                                         sel 0
                                                           outlet 0 (=0): 自デバイス → 破棄
                                                           outlet 1 (≠0): 外部パラメータ → prepend id → learn_gate
```

**ポイント**:
- `canonical_parent` は `live.object` の `get canonical_parent` で取得する（`live.path goto` ではない）
- `!=` の右インレットに init チェーンから this_device_id を設定。初期値 0 のため、init 前は全パラメータ通過（安全側）
- 乗算フィルタ: `*` で条件フィルタ。`gate + i (store/recall)` より簡潔
- `t i i` で N を分配: outlet 1 → prepend id（ターゲット設定）、outlet 0 → * 右inlet（値保存）
