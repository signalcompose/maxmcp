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

## 5. live.* オブジェクト生成テンプレート

### 🔴 必読: テンプレート全項目必須

以下のテンプレートは「参考」ではなく**生成時に必ず設定する全項目**。1 つでも欠けると Phase 4 構造検証では検出できない潜在バグの原因になる:

- `_parameter_shortname` 未設定 → Push 表示・パラメータ一覧で空白になる
- `_parameter_unitstyle` 未設定 → Float 値が Int 表示になり精度が見えない
- `appearance` 未設定（live.numbox） → Slider 用途で Numbox スタイルになる
- `_parameter_order` 未設定 → 復元順が不定になり、`_parameter_range` 設定前に値が復元されてクランプされる

**🚫 禁止**: 「動作には影響しなさそうだから後回し」。設定漏れは数日後にユーザーから指摘される形でしか発覚しない。`add_max_object` 直後に**全項目を `set_object_attribute` で連続設定**する習慣にする。

### ❌ Anti-pattern: テンプレート全項目を読み飛ばし、必要そうな項目だけ設定

```
add_max_object(obj_type="live.numbox", varname="range_min")
set_object_attribute(varname="range_min", attribute="_parameter_longname", value="range_min")
set_object_attribute(varname="range_min", attribute="_parameter_initial", value=0.0)
// ↑ ここで止める。_parameter_unitstyle / appearance / _parameter_shortname / _parameter_order を忘れる
```

### ✅ 正解: 全項目を一括設定（テンプレートを上から順にすべて適用）

```
add_max_object(obj_type="live.numbox", varname="range_min")
// 以下、テンプレートの全項目を漏れなく設定
set_object_attribute(... "_parameter_type" ... 0)
set_object_attribute(... "_parameter_unitstyle" ... 1)
set_object_attribute(... "_parameter_range" ... [-100000, 100000])
set_object_attribute(... "_parameter_shortname" ... "Min")
set_object_attribute(... "_parameter_longname" ... "range_min")
set_object_attribute(... "_parameter_order" ... <設計値>)
set_object_attribute(... "_parameter_initial_enable" ... 1)
set_object_attribute(... "_parameter_initial" ... 0.0)
set_object_attribute(... "appearance" ... 2)  // Slider モードが必要なら
```

---

オブジェクト生成時に設定すべきアトリビュートの一覧。`add_max_object` の `attributes` パラメータは反映されない場合があるため、生成後に `get_object_attribute` で確認し、未反映の場合は `set_object_attribute` で再設定する。

### live.dial

```
obj_type: live.dial
varname: control_dial
```

生成後に設定:

| アトリビュート | 設定方法 | 例 |
|---|---|---|
| `_parameter_type` | `set_object_attribute` | 0 (Float) |
| `_parameter_unitstyle` | `set_object_attribute` | 1 (Float表示) |
| `_parameter_range` | `set_object_attribute` | [0, 1] |
| `_parameter_shortname` | `set_object_attribute` | "Control" |
| `_parameter_longname` | `set_object_attribute` | "control_dial" |
| `_parameter_order` | `set_object_attribute` | (設計に基づく) |
| `_parameter_initial_enable` | `set_object_attribute` | 1 |
| `_parameter_initial` | `set_object_attribute` | 0 |
| `showname` | `set_object_attribute` | 0 |
| `shownumber` | `set_object_attribute` | 0 |
| `appearance` | `set_object_attribute` | 0=Vertical, 1=Tiny, 2=Panel, 3=Large |

### live.numbox

```
obj_type: live.numbox
varname: range_min
```

生成後に設定:

| アトリビュート | 設定方法 | 例 |
|---|---|---|
| `_parameter_type` | `set_object_attribute` | 0 (Float) |
| `_parameter_unitstyle` | `set_object_attribute` | 1 (Float表示) |
| `_parameter_range` | `set_object_attribute` | [-100000, 100000] |
| `_parameter_shortname` | `set_object_attribute` | "Min" |
| `_parameter_longname` | `set_object_attribute` | "range_min" |
| `_parameter_order` | `set_object_attribute` | (設計に基づく) |
| `_parameter_initial_enable` | `set_object_attribute` | 1 |
| `_parameter_initial` | `set_object_attribute` | 0 |
| `appearance` | `set_object_attribute` | 0=Numbox, 2=Slider |

### live.text

```
obj_type: live.text
varname: learn_toggle
```

生成後に設定:

| アトリビュート | 設定方法 | 例 |
|---|---|---|
| `mode` | `set_object_attribute` | 1 (Toggle) |
| `text` | `set_object_attribute` | "Learn" (OFF時テキスト) |
| `texton` | `set_object_attribute` | "Learn" (ON時テキスト) |
| `_parameter_shortname` | `set_object_attribute` | "Learn" |
| `_parameter_longname` | `set_object_attribute` | "learn_toggle" |
| `_parameter_type` | `set_object_attribute` | 2 (Int) |
| `_parameter_range` | `set_object_attribute` | [0, 1] |
| `_parameter_order` | `set_object_attribute` | (設計に基づく) |
| `_parameter_initial_enable` | `set_object_attribute` | 1 |
| `_parameter_initial` | `set_object_attribute` | 0 |

### pattr

```
obj_type: pattr
arguments: ["param_name"]
varname: param_name
```

生成後に設定:

| アトリビュート | 設定方法 | 例 |
|---|---|---|
| `parameter_enable` | `set_object_attribute` | 1 |
| `_parameter_invisible` | `set_object_attribute` | 1 (Stored Only) |
| `_parameter_order` | `set_object_attribute` | (設計に基づく) |
| `_parameter_range` | `set_object_attribute` | [-100000, 100000] (Float用) |
| `_parameter_type` | `set_object_attribute` | 0=Float, 3=blob(テキスト用) |
| `_parameter_modmode` | `set_object_attribute` | 0 (None) |
| `parameter_mappable` | `set_object_attribute` | 0 |
| `_parameter_initial_enable` | `set_object_attribute` | 1 |
| `_parameter_longname` | `set_object_attribute` | "param_name" |

**pattr outlet 構造**:
- outlet 0 (左): 値出力（autorestore、値変更時）
- outlet 1 (中央): bindto 接続（UI オブジェクトを接続すると自動バインド）
- outlet 2 (右): dumpout

### live.object

```
obj_type: live.object
varname: target_obj
```

生成後に設定:

| アトリビュート | 設定方法 | 例 |
|---|---|---|
| `_persistence` | `set_object_attribute` | 1 (Persistent Mapping) |

**API**:
- プロパティ取得: `get <property>` を左 inlet に送信
- プロパティ設定: `set <property> <value>` を左 inlet に送信（`<property> <value>` ではない）
- ターゲット設定: `id N` を右 inlet に送信

### textedit

```
obj_type: textedit
varname: param_name_display
```

生成後に設定:

| アトリビュート | 設定方法 | 例 |
|---|---|---|
| `lines` | `set_object_attribute` | 1 (1行制限) |
| `wordwrap` | `set_object_attribute` | 0 |
| `textjustification` | `set_object_attribute` | 0=左, 1=中央, 2=右 |
| `fontsize` | `set_object_attribute` | 12 |
