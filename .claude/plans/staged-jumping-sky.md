# MaxMCPパッチ作成ガイドライン スキル実装計画

## 概要

MaxMCPでパッチを作成する際のガイドラインを、Claude Codeプラグインの**スキル**として実装する。

## 使用するツール

**plugin-dev プラグインのスキルを使用**:
- `/plugin-dev:plugin-structure` - プラグイン構造の作成
- `/plugin-dev:skill-development` - スキルの実装

### なぜスキルか？

| 比較項目 | スキル | エージェント |
|----------|--------|-------------|
| 用途 | 知識・ガイドライン提供 | 自動タスク実行 |
| トリガー | キーワードで自動読み込み | 条件マッチで自動実行 |
| 適合性 | **◎ 今回の要件に最適** | △ オーバースペック |

## ディレクトリ構造

```
package/MaxMCP/support/claude-plugin/
├── .claude-plugin/
│   └── plugin.json
├── skills/
│   └── maxmcp-patch-guidelines/
│       ├── SKILL.md                    # メインガイドライン（~1,500語）
│       └── reference/
│           ├── layout-rules.md         # レイアウト詳細規則
│           ├── naming-conventions.md   # 命名規則
│           └── javascript-guide.md     # JavaScript使用ガイド
└── README.md
```

## 実装ファイル一覧

### 1. plugin.json
- パス: `package/MaxMCP/support/claude-plugin/.claude-plugin/plugin.json`
- 内容: プラグインメタデータ（name, description, author）

### 2. SKILL.md（メイン）
- パス: `package/MaxMCP/support/claude-plugin/skills/maxmcp-patch-guidelines/SKILL.md`
- 内容:
  - YAML frontmatter（name, description）
  - 作成前チェックリスト
  - 信号フローの基本原則
  - レイアウト規則（マージン、配置）
  - オブジェクト作成のベストプラクティス
  - reference/ へのリンク

### 3. layout-rules.md
- パス: `package/MaxMCP/support/claude-plugin/skills/maxmcp-patch-guidelines/reference/layout-rules.md`
- 内容: 詳細なレイアウト規則、マージン値、配置パターン、複数回路の配置

### 4. naming-conventions.md
- パス: `package/MaxMCP/support/claude-plugin/skills/maxmcp-patch-guidelines/reference/naming-conventions.md`
- 内容: varname命名規則、入力/出力/演算子/制御の命名

### 5. javascript-guide.md
- パス: `package/MaxMCP/support/claude-plugin/skills/maxmcp-patch-guidelines/reference/javascript-guide.md`
- 内容: v8/v8ui推奨理由、コード例、移行ガイド

### 6. README.md
- パス: `package/MaxMCP/support/claude-plugin/README.md`
- 内容: インストール手順、使用方法

## トリガーキーワード設計

descriptionに含める自動トリガーフレーズ:
- "MaxMCP" + パッチ操作
- "Max patch" + create/build/add
- "パッチ作成" / "パッチレイアウト"
- MCPツール名（add_max_object, connect_max_objects等）

## 実装手順

### Phase 1: プラグイン構造の作成
1. `/plugin-dev:plugin-structure` スキルを使用
2. プラグイン名: `maxmcp-guidelines`
3. 配置先: `package/MaxMCP/support/claude-plugin/`

### Phase 2: スキルの実装
4. `/plugin-dev:skill-development` スキルを使用
5. スキル名: `maxmcp-patch-guidelines`
6. ユーザー提供のガイドライン内容をSKILL.mdに反映

### Phase 3: リファレンス作成
7. `reference/layout-rules.md` 作成
8. `reference/naming-conventions.md` 作成
9. `reference/javascript-guide.md` 作成

### Phase 4: ドキュメント
10. `README.md` 作成
11. `docs/INDEX.md` 更新

## 検証方法

1. **プラグイン認識テスト**
   ```bash
   # Claude Codeでプラグインが読み込まれるか確認
   /plugins
   ```

2. **スキルトリガーテスト**
   - 「MaxMCPでパッチを作成して」と入力
   - スキルが自動読み込みされるか確認

3. **実際のパッチ作成テスト**
   - MCPツールでパッチ作成を実行
   - ガイドラインに従った配置になるか確認

## CLAUDE.mdへの追記（最小限）

```markdown
## MaxMCP Patch Guidelines Plugin

パッチ作成ガイドラインはClaude Codeプラグインとして提供:
- 場所: `package/MaxMCP/support/claude-plugin/`
- 自動トリガー: MaxMCPツール使用時にスキルが読み込まれる
```

## 補足: エージェント追加の検討

現時点ではエージェントは不要。理由:
- パッチレビューはMax上での動作確認が必須
- `get_objects_in_patch`で構造確認は可能
- 将来的にパッチ解析エージェントを検討可能
