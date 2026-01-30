# PR作成とレビュワー向けテスト手順

## 概要

`feature/plugin-marketplace` ブランチのPRを作成し、レビュワーがmainマージ前にプラグインをテストできる方法を説明する。

## 現在の状態

- **ブランチ**: `feature/plugin-marketplace`
- **コミット**: 2件
  - `feat(plugin): add Claude Code plugin marketplace for patch guidelines`
  - `docs(readme): add Claude Code plugin section`
- **追加ファイル**: 9ファイル（約1000行）

## レビュワー向けテスト方法

### 方法1: ローカルクローンからテスト（推奨）

レビュワーがfeatureブランチをクローンしてローカルでテスト:

```bash
# 1. リポジトリをクローン
git clone https://github.com/signalcompose/maxmcp.git
cd maxmcp
git checkout feature/plugin-marketplace

# 2. ローカルディレクトリからマーケットプレイス追加
/plugin marketplace add /path/to/maxmcp

# 3. プラグインインストール
/plugin install maxmcp@maxmcp

# 4. スキル呼び出しテスト
/maxmcp:patch-guidelines
```

### 方法2: GitHub URLから直接テスト

Claude Codeがブランチ指定をサポートしている場合:

```bash
# featureブランチを直接指定（サポート状況要確認）
/plugin marketplace add signalcompose/maxmcp@feature/plugin-marketplace
/plugin install maxmcp@maxmcp
/maxmcp:patch-guidelines
```

**注意**: ブランチ指定がサポートされていない可能性あり。方法1を推奨。

### テスト項目チェックリスト

- [ ] マーケットプレイス追加が成功する
- [ ] プラグインインストールが成功する
- [ ] `/plugins` でmaxmcpが表示される
- [ ] `/maxmcp:patch-guidelines` でスキルが読み込まれる
- [ ] SKILL.mdの内容が適切に表示される
- [ ] リファレンスファイルへのリンクが機能する

## PR内容

### タイトル
```
feat(plugin): add Claude Code plugin marketplace for patch guidelines
```

### 本文構成

1. **Summary**: 変更概要
2. **Changes**: 追加ファイル一覧
3. **Testing Instructions**: レビュワー向けテスト手順
4. **Verification Checklist**: テスト項目

## 実行手順

1. `gh pr create` でPR作成
2. テスト手順をPR本文に含める
3. PR URLを返す

## 検証方法

PR作成後、以下を確認:
- CIが通過する
- PR本文にテスト手順が含まれている
- レビュワーがテスト可能な状態
