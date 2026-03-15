# bpatcher Techniques

`bpatcher` を使った再利用可能コンポーネントのテクニック集。基本仕様（embedding, arguments）は Max refpage を参照。

## Why bpatcher Instead of p/patcher

`p`/`patcher`（埋め込みサブパッチ）は `#N` 引数置換をサポートしない。引数を使ってインスタンスごとに動作を変えたい場合は `bpatcher` を使う。

| オブジェクト | `#N` 引数置換 | 引数の渡し方 |
|---|---|---|
| `p` / `patcher` | 不可 | （名前のみ） |
| `bpatcher` | 可能 | `@args value1 value2` |
| `poly~` / `mc.poly~` | 可能 | `@args value1 value2` |
| アブストラクション（外部 .maxpat） | 可能 | オブジェクト名の後に引数 |

## Dynamic send/receive Names

引数を使ってインスタンスごとにユニークな通信チャネルを作成:

```
// Inside bpatcher, with argument = "synth1"
send #1_volume       → becomes "synth1_volume"
receive #1_volume    → matches the same channel
```

`#0` の問題（同じファイルをロードした全インスタンスが同じ値を共有）を引数ベースの命名で回避。

## Argument Inheritance

ネストした bpatcher で引数を転送:

```
Parent bpatcher (arg: "main")
  └── Child bpatcher (arg: "#1_sub")    → becomes "main_sub"
       └── send #1_data                 → becomes "main_sub_data"
```

深い階層でも名前衝突なしにネーミングが可能。`poly~` / `mc.poly~` での `@args` による選択的転送・結合については [Argument Forwarding with Transformation](poly-techniques.md#argument-forwarding-with-transformation) を参照。

## Presentation Mode

サブパッチャーの Inspector で "Open in Presentation" を有効にすると、親のプレゼンテーションモード内にサブパッチャーの presentation view が表示される。一度UIを設計すれば複数の親パッチで再利用可能。

## Combining bpatcher + poly~ for Voice UI

bpatcher インスタンスを poly~ ボイスの UI コントローラーとして使うパターン。ボイス番号を引数として渡す:

```
bpatcher @args 1    → controls poly~ voice 1
bpatcher @args 2    → controls poly~ voice 2
bpatcher @args 3    → controls poly~ voice 3
```

bpatcher 内:
```
slider → send #1_volume    → "1_volume", "2_volume", "3_volume"
```

poly~ ボイス内:
```
receive #1_volume           → #1 matches the voice number via @args
```

**Namespace convention**: プロジェクト固有プレフィックスで衝突回避:
```
#0_PROJECT_#1_paramname     → e.g., "1234_SYNTH_1_volume"
```

**Benefit**: bpatcher テンプレートを1回修正すれば全ボイス UI が同時更新。

## Sources

- https://leico.github.io/TechnicalNote/Max/bpatcher-tips
- https://leico.github.io/TechnicalNote/Max/bpatcher-poly-interface
