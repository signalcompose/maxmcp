# MCP Tool Notes

MaxMCP の MCP ツールを使用する際の注意事項。Max の挙動と MCP ツールの制約に起因する、ツール利用時に必要な追加操作。

## 1. アトリビュートの設定方法

**ルール**: オブジェクト生成時に確定するアトリビュートは、`@` 構文でオブジェクトテキストに直接記述する。`set_object_attribute` では設定できない場合がある。

**理由**: `@` 構文は Max 全般で有効なオブジェクト生成時のアトリビュート指定方法。Jitter オブジェクト（`jit.gl.mesh @draw_mode triangles` 等）をはじめ広く使われる標準機能。一方、`set_object_attribute` で設定可能なアトリビュートは限定される。

**使い分け**:

| 設定方法 | 対象 | 例 |
|---|---|---|
| `@` テキスト記述 | オブジェクト固有のアトリビュート | `jit.gl.mesh @draw_mode triangles`, `live.dial @showname 0` |
| `set_object_attribute` | 外部アトリビュート | `patching_rect`, `presentation`, `presentation_rect`, `bgcolor`, `_parameter_range` |

**検証**: `get_object_attribute` で設定が反映されているか確認する。"Attribute not found" が返る場合、テキスト記述が必要。

## 2. 新規オブジェクトの presentation 設定

**ルール**: `add_max_object` でオブジェクトを追加した後、ロジック用オブジェクトは必ず `set_object_attribute` で `presentation 0` を設定する。

**理由**: `add_max_object` の `attributes` パラメータで `presentation: 0` を指定しても、Max が無視してデフォルト値（プレゼンテーションモードが有効なパッチでは `1`）に戻す。このため、追加時の指定に頼らず、追加後に明示的に `set_object_attribute` で設定する必要がある。

**パターン**:

```
1. add_max_object でオブジェクトを追加
2. set_object_attribute で presentation = 0 を設定
3. get_object_attribute で presentation を確認（任意）
```

**対象**: trigger, prepend, route, zl, gate, live.object, live.path, live.thisdevice, pattr, pack, scale, int, !=, *, sel 等のロジック用オブジェクト全般

## 3. number / flonum の内部表現

Max の内部表現では **flonum は独立したクラスではない**。整数・浮動小数点の number box は共に `maxclass: "number"` として扱われる。

### format 属性による識別

| 表示 | maxclass | format | 説明 |
|------|----------|--------|------|
| `[0]` (整数) | `number` | `0` | 整数 number box |
| `[0.]` (実数) | `number` | `6` | 浮動小数点 number box (flonum) |

### MCP での flonum 作成

```
add_max_object: obj_type = "number"
set_object_attribute: attribute = "format", value = 6
```

### 既存パッチでの識別

`get_objects_in_patch` は両方とも `maxclass: "number"` と報告する。区別するには:

```
get_object_attribute: attribute = "format"
  → 0 = 整数 number box
  → 6 = 浮動小数点 number box (flonum)
```

`maxclass: "number"` を見て整数と決めつけず、常に `format` 属性を確認すること。

## 4. get_object_value の対応状況

`get_object_value` は Max SDK の `object_getvalueof()` を内部で使用する。全てのオブジェクトがこのインターフェースを実装しているわけではない。

### 対応オブジェクト

| オブジェクト | 戻り値の型 | 備考 |
|-------------|-----------|------|
| `number` (整数) | number | 現在の整数値 |
| `number` (flonum, format=6) | number | 現在の実数値 |
| `toggle` | number | 0 (off) / 1 (on) |
| `led` | number | 0 (off) / 1 (on) |
| `textbutton` | number | 0 (off) / 1 (on) |
| `tab` | number | 選択中のタブインデックス (整数) |
| `slider` | number | 現在のスライダー位置 (整数) |
| `dial` | number | 現在のダイヤル値 |
| `gain~` | number | 現在のゲイン値 (整数) |
| `live.gain~` | number | 現在のゲイン (dB, 実数) |
| `multislider` | array | スライダー値の配列 (実数) |
| `live.text` | number | 0.0 (off) / 1.0 (on), 実数 |
| `live.tab` | number | 選択中のタブインデックス (実数) |
| `live.step` | array | シーケンサーデータ |
| `live.grid` | array | グリッド状態 |
| `textedit` | string | テキスト内容 |

### 非対応オブジェクト

| オブジェクト | 動作 |
|-------------|------|
| `message` | テキスト内容ではなくオブジェクト型情報を返す |
| `comment` | テキスト内容ではなくオブジェクト型情報を返す |

message/comment のテキスト内容は `get_objects_in_patch`（box text を含む）で取得するか、`replace_object_text` で変更する。
