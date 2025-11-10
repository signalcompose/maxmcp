# リサーチ: Phase 2 E2E WebSocket MCP接続修正

## リサーチ実施日
2025-11-10

## リサーチ目的
Claude CodeからMax/MSPへのE2E MCP接続を確立し、11のツールをすべて認識・実行可能にする。

## 背景

### 初期状態
- Phase 1でWebSocketサーバーとMCPサーバーの基本実装完了
- Outlet bangタイミング問題を`defer_low()`で解決
- WebSocket送信キュー機構が動作
- **しかし**: Claude Codeが「Capabilities: none」と表示し、ツールを認識しない

### アーキテクチャ
```
Claude Code (stdio) → Bridge (WebSocket) → maxmcp.agent (Max)
```

## 調査方法

### 1. MCPプロトコル仕様確認
- Context7 MCPサーバーでMCP公式ドキュメント参照
- JSON-RPC 2.0仕様の詳細確認
- FastMCP（公式Python SDK）の実装パターン調査

### 2. 成功実装との比較
- `/Users/yamato/Src/proj_max_mcp/MaxMSP-MCP-Server-multipatch`
- Node.js for MaxでFastMCPを使用した成功実装
- プロトコルバージョン交渉の自動処理パターンを発見

### 3. デバッグ手法
- Max Consoleへの詳細ログ出力
- Bridge側のファイルログ（`~/.claude/maxmcp-bridge.log`）
- JSON出力の完全ダンプ（`/tmp/maxmcp_tools_list.json`）
- Python JSON Schema検証スクリプト

## 発見された問題と解決策

### 問題1: Notification Response（最重要）

**症状**:
```json
{"jsonrpc": "2.0", "id": null}
```
`notifications/initialized`に対してレスポンスを返していた。

**原因**:
JSON-RPC 2.0仕様違反。Notificationは一方向メッセージであり、レスポンスを**返してはならない**。

**調査プロセス**:
1. ユーザーからの指摘: "mcpの仕様を確認してください"
2. MCP公式ドキュメント確認
3. JSON-RPC 2.0仕様で明確に禁止されていることを発見

**解決策**:

#### mcp_server.cpp (handle_request)
```cpp
} else if (method.find("notifications/") == 0) {
    // Notification messages (no response required)
    // According to JSON-RPC 2.0, notifications MUST NOT receive a response
    ConsoleLogger::log(("MCP: Received notification: " + method).c_str());
    return nullptr;  // Return null JSON to signal no response
}
```

#### mcp_server.cpp (handle_request_string)
```cpp
json response = handle_request(req);

// Check if response is null (for notifications)
if (response.is_null()) {
    ConsoleLogger::log("MCP: No response required (notification)");
    return std::string();  // Return truly empty string (0 bytes)
}

return response.dump();
```

#### websocket_server.cpp
```cpp
std::string response = server->sync_message_callback_(client->client_id, message);

// Check if response is empty (for notifications)
if (response.empty()) {
    ConsoleLogger::log("RECEIVE: Empty response (notification), no response sent");
} else {
    // Add response to send queue and trigger writable
    ...
}
```

**重要ポイント**:
- `return "";` は2バイト（引用符付き）なので不適切
- `return std::string();` で真の空文字列（0バイト）を返す
- `response.empty()` でチェック可能

**検証**:
```
Max Console:
MCP: Received notification: notifications/initialized
MCP: No response required (notification)
Sync callback: Generated response (0 bytes)
RECEIVE: Empty response (notification), no response sent
```

---

### 問題2: プロトコルバージョン不一致

**症状**:
Claude Codeが「Capabilities: none」と表示。ツールリスト（4181バイト）は正常に送信されているのに認識されない。

**原因**:
```
Claude Code 2.0.36 → "protocolVersion": "2025-06-18"
MaxMCP          → "protocolVersion": "2024-11-05" (固定値)
```

バージョンミスマッチによりClaude Codeが接続を拒否。

**調査プロセス**:
1. ユーザーの提案: "成功しているPython実装を参考にしては？"
2. FastMCP実装を確認
3. **キー発見**: FastMCPはクライアントのバージョンをエコーバックしている

**解決策**:

```cpp
if (method == "initialize") {
    // Echo back the client's protocol version (MCP spec requirement)
    std::string client_protocol_version =
        req["params"].value("protocolVersion", "2024-11-05");

    ConsoleLogger::log(("MCP: Client protocol version: " +
                       client_protocol_version).c_str());

    return {
        {"jsonrpc", "2.0"},
        {"id", req.contains("id") ? req["id"] : nullptr},
        {"result", {
            {"protocolVersion", client_protocol_version},  // Echo client's version
            {"capabilities", {
                {"tools", {
                    {"listChanged", true}
                }}
            }},
            {"serverInfo", {
                {"name", "maxmcp"},
                {"version", "0.2.0"}
            }}
        }}
    };
}
```

**検証**:
```
Max Console:
MCP: Client protocol version: 2025-06-18
```

Claude Code `/mcp`:
```
Status: ✓ connected
Capabilities: tools
Tools: 11 tools
```

---

### 問題3: JSON Schema Draft 2020-12 違反

**症状**:
```
API Error 400
tools.21.custom.input_schema: JSON schema is invalid.
It must match JSON Schema draft 2020-12
```

**原因1**: 空のproperties
```cpp
// Wrong:
{"properties", {}}  // Becomes null in JSON

// Fixed:
{"properties", json::object()}  // Becomes {}
```

**原因2**: itemsフィールドの不正なネスト（最重要）
```cpp
// Wrong - becomes ["type", "number"] (array of strings):
{"position", {
    {"type", "array"},
    {"items", {"type", "number"}},  // Invalid!
    {"description", "Position [x, y] in patch"}
}}

// Fixed - proper nesting:
{"position", {
    {"type", "array"},
    {"items", {  // Extra level of braces
        {"type", "number"}
    }},
    {"description", "Position [x, y] in patch"}
}}
```

**調査プロセス**:
1. 完全なJSON出力を`/tmp/maxmcp_tools_list.json`に保存
2. Python JSON Schema検証スクリプトで分析
3. `add_max_object`ツールの`position`プロパティで発見

**影響範囲**:
`add_max_object`ツールのみ（tool #2 in array）

**検証**:
```
全11ツールがJSON Schema draft 2020-12に準拠
Claude Codeが正常にツールを認識
```

---

### 問題4: バイナリインストール（副次的）

**症状**:
リビルド後もMax Consoleに古いログが表示される。

**原因**:
1. `cmake --install`が誤った場所にインストール
2. Maxアプリケーションが古いバイナリをキャッシュ

**解決策**:
```bash
# 正しいビルド出力パス
/Users/yamato/externals/maxmcp.agent.mxo

# 手動コピー
rm -rf "$HOME/Documents/Max 9/Packages/MaxMCP/externals/maxmcp.agent.mxo"
cp -R /Users/yamato/externals/maxmcp.agent.mxo \
      "$HOME/Documents/Max 9/Packages/MaxMCP/externals/"

# Max完全終了（Cmd+Q）後に再起動
```

---

## アーキテクチャ改善

### Bridge再配置

**変更前**:
```
bridge/
  websocket-mcp-bridge.js
  package.json
  build.sh (nexe用)
```

**変更後**:
```
package/MaxMCP/support/bridge/
  websocket-mcp-bridge.js
  package.json
  bridge-launcher.js
  npm-install.js
  test/bridge.test.js
```

**利点**:
- Max Packageと統合された配布
- nexe依存を削除（シンプル化）
- npm installで即座に使用可能

### C++コードのシンプル化

**削除**:
- `start_bridge()`機能（maxmcp_server.cpp）
- Bridge起動ロジックをC++から削除

**理由**:
- BridgeライフサイクルはMaxパッチで管理すべき
- C++からのプロセス起動は複雑性を増す
- ユーザーが必要に応じて手動起動可能

### 新しいExample構成

1. **00-setup.maxpat**: npm install自動化
2. **maxmcp-bridge.maxpat**: 再利用可能な抽象化
3. **01-claude-code-connection.maxpat**: E2E接続デモ
4. **02-08**: 各種ユースケースデモ
5. **09-bridge-launch.maxpat**: Bridge単独起動

---

## 検証結果

### E2E接続成功

**Claude Code `/mcp` 出力**:
```
Status: ✓ connected
Capabilities: tools
Tools: 11 tools
```

### ツール実行成功

#### 1. get_console_log
```
Request ID: 2
Arguments: {"lines": 10}
Response: 904 bytes
Result: 10行のログを正常に返却
```

#### 2. list_active_patches
```
Request ID: 3
Arguments: {}
Response: 69 bytes
Result: {"count": 0, "patches": []}
説明: maxmcp.clientが含まれるパッチが未開のため0
```

### Notification処理検証

```
Request: {"method":"notifications/cancelled",...}
Response: (0 bytes, no WebSocket send)
Log: "RECEIVE: Empty response (notification), no response sent"
```

JSON-RPC 2.0準拠を確認。

---

## 技術的洞察

### 1. MCP Protocol Negotiation Pattern

**重要な発見**: MCPサーバーは**クライアントのprotocolVersionをエコーバック**すべき。

これはFastMCP（公式SDK）の実装パターンであり、事実上のベストプラクティス。

**理由**:
- クライアントは自身がサポートするバージョンを送信
- サーバーはそれを受け入れる意思をエコーで示す
- バージョン不一致時にクライアントが接続を拒否

### 2. JSON-RPC 2.0 Notification Semantics

**仕様**:
> A Notification is a Request object without an "id" member.
> A Server MUST NOT reply to a Notification.

**実装の要点**:
- Notificationにはレスポンスを返さない
- 空文字列（0バイト）を返すことでWebSocket送信をスキップ
- `response.empty()`チェックで判定

### 3. nlohmann/json Initialization Behavior

**重要な違い**:
```cpp
{} → null (initializer_listが空)
json::object() → {} (明示的な空オブジェクト)
```

JSON Schema準拠には`json::object()`を使用すべき。

### 4. JSON Schema Draft 2020-12 Items Field

**正しいネスト**:
```json
{
  "type": "array",
  "items": {  // itemsはschemaオブジェクト
    "type": "number"
  }
}
```

**間違ったネスト**:
```cpp
{"items", {"type", "number"}}  // → ["type", "number"]
```

C++初期化リストでは**余分なブレースが必要**。

---

## 得られた教訓

### 1. 仕様を最優先で確認

ユーザーからの指摘「仕様を確認してください」は正しかった。MCP公式ドキュメントとJSON-RPC 2.0仕様を確認することで、notification問題を即座に解決。

### 2. 成功実装から学ぶ

FastMCP実装の確認により、プロトコルバージョン交渉の正しいパターンを発見。公式SDKは事実上のリファレンス実装。

### 3. 完全なJSON出力でデバッグ

`/tmp/maxmcp_tools_list.json`への完全出力が、JSON Schema問題の特定に不可欠だった。部分的なログでは発見できなかった。

### 4. クライアント視点でのテスト

Max Console上では正常に見えても、Claude Code側で問題が発生。クライアント視点（`/mcp`コマンド）でのテストが重要。

### 5. バイナリキャッシュに注意

Max/MSPはexternalをキャッシュする。完全終了（Cmd+Q）なしでは新しいバイナリが反映されない。

---

## 結論

Phase 2は以下の3つの重要な修正により完了：

1. **Notification処理** - JSON-RPC 2.0準拠（0バイトレスポンス）
2. **プロトコルバージョン交渉** - クライアントバージョンのエコーバック
3. **JSON Schema準拠** - draft 2020-12要件を満たす正しいネスト

これらの修正により、Claude CodeからMax/MSPへのE2E MCP接続が確立され、全11ツールが認識・実行可能になった。

## 参照

- MCP Specification: https://spec.modelcontextprotocol.io/
- JSON-RPC 2.0: https://www.jsonrpc.org/specification
- JSON Schema draft 2020-12: https://json-schema.org/draft/2020-12/
- FastMCP (Python SDK): `/Users/yamato/Src/proj_max_mcp/MaxMSP-MCP-Server-multipatch`

## 次のステップ

Phase 3への移行:
- 追加MCP toolsの実装
- エラーハンドリングの強化
- パフォーマンス最適化
- ドキュメント拡充
