# リサーチ: Max External の dylib バンドリングとコード署名

## リサーチ実施日
2025-11-09

## リサーチ目的
MaxMCP の maxmcp.server external が「system security policy」エラーで読み込めない問題を解決するため、Max External における dylib の正しいバンドリング方法とコード署名のベストプラクティスを調査。

## リサーチ方法
- Gemini CLI による Web 検索
- `otool -L` による依存関係の分析
- `install_name_tool` によるパス修正の実験
- `codesign` による署名検証

## 調査結果サマリー

Max MSP は External Object のセキュリティポリシーとして、**外部の絶対パスを参照する dylib を含む External の読み込みを拒否**する。これは macOS の Gatekeeper とは別の、Max 独自のセキュリティチェック。

解決策は以下の2点:
1. **dylib のバンドリング**: 全ての依存 dylib を `Contents/Frameworks/` にコピー
2. **相対パス化**: `install_name_tool` で全てのパスを `@loader_path` ベースの相対パスに変更

## 詳細調査結果

### 問題の特定

#### 初期症状
```
maxmcp.server: cannot be loaded due to system security policy
```

このエラーメッセージは以下の点で誤解を招く:
- **"system security policy"** と表示されるが、実際は **Max のセキュリティポリシー**
- macOS の Gatekeeper や SIP とは無関係
- macOS Console.app にログが出ないため、Max 内部のチェックであることが判明

#### 根本原因の発見

`otool -L` による依存関係の確認:
```bash
otool -L maxmcp.server.mxo/Contents/MacOS/maxmcp.server
```

出力:
```
/opt/homebrew/opt/libwebsockets/lib/libwebsockets.20.dylib
/opt/homebrew/opt/openssl@3/lib/libssl.3.dylib
/opt/homebrew/opt/openssl@3/lib/libcrypto.3.dylib
```

**問題**: Homebrew でインストールした dylib への絶対パス参照が、Max のセキュリティポリシーに違反。

### 解決策の実装

#### ステップ1: dylib のバンドリング

CMake で自動化:
```cmake
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory
        "$<TARGET_BUNDLE_DIR:${PROJECT_NAME}>/Contents/Frameworks"
    COMMAND ${CMAKE_COMMAND} -E copy
        /opt/homebrew/opt/libwebsockets/lib/libwebsockets.20.dylib
        "$<TARGET_BUNDLE_DIR:${PROJECT_NAME}>/Contents/Frameworks/"
    # ... 他の dylib も同様
)
```

#### ステップ2: メイン実行ファイルのパス修正

```cmake
# Fix install names in main executable
COMMAND install_name_tool -change
    /opt/homebrew/opt/libwebsockets/lib/libwebsockets.20.dylib
    @loader_path/../Frameworks/libwebsockets.20.dylib
    "$<TARGET_BUNDLE_DIR:${PROJECT_NAME}>/Contents/MacOS/maxmcp.server"
```

#### ステップ3: dylib 内部のパス修正（重要！）

**落とし穴**: バンドルした dylib 自身も絶対パスで他の dylib を参照している。

例: `libwebsockets.20.dylib` の依存関係:
```bash
otool -L libwebsockets.20.dylib
# 出力:
/opt/homebrew/opt/openssl@3/lib/libssl.3.dylib
/opt/homebrew/opt/openssl@3/lib/libcrypto.3.dylib
```

**解決**: dylib の ID と内部参照も修正:
```cmake
# Fix install name in libwebsockets.20.dylib itself
COMMAND install_name_tool -id
    @loader_path/../Frameworks/libwebsockets.20.dylib
    "$<TARGET_BUNDLE_DIR:${PROJECT_NAME}>/Contents/Frameworks/libwebsockets.20.dylib"
COMMAND install_name_tool -change
    /opt/homebrew/opt/openssl@3/lib/libssl.3.dylib
    @loader_path/libssl.3.dylib
    "$<TARGET_BUNDLE_DIR:${PROJECT_NAME}>/Contents/Frameworks/libwebsockets.20.dylib"
```

#### ステップ4: コード署名の再適用

`install_name_tool` はバイナリを変更するため、既存の署名が無効化される。

**エラー例**:
```
code has no resources but signature indicates they must be present
```

**解決**: 全ての変更完了後に署名:
```cmake
# CODE SIGNING (macOS only)
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND find "$<TARGET_BUNDLE_DIR:${PROJECT_NAME}>" -name "*.backup" -delete
    COMMAND codesign --force --deep --sign - "$<TARGET_BUNDLE_DIR:${PROJECT_NAME}>"
    COMMENT "Code signing ${OUTPUT_NAME} with ad-hoc signature"
)
```

**検証**:
```bash
codesign --verify --deep --strict maxmcp.server.mxo
# 出力: (エラーなし)

codesign -dv --verbose=4 maxmcp.server.mxo
# Signature=adhoc (ad-hoc 署名が適用されている)
```

### @loader_path の詳細

#### @loader_path とは
- **ローダーの場所を基準とした相対パス**
- External の場合: `Contents/MacOS/maxmcp.server` がローダー

#### パス計算例

メイン実行ファイルから:
```
@loader_path/../Frameworks/libwebsockets.20.dylib
↓
Contents/MacOS/../Frameworks/libwebsockets.20.dylib
= Contents/Frameworks/libwebsockets.20.dylib
```

dylib から dylib への参照:
```
@loader_path/libssl.3.dylib
↓
Contents/Frameworks/libssl.3.dylib
```

**注意**: dylib から dylib の場合、`../Frameworks/` は不要（同じディレクトリ内）。

### 試行錯誤の履歴

#### 失敗した試み
1. **Ad-hoc 署名のみ**: dylib が外部パスのままでは無効
2. **Quarantine 属性の削除**: 問題は macOS ではなく Max 側
3. **fork/exec の無効化**: ブリッジ起動機能は無関係

#### 成功の鍵
- **`otool -L` による徹底的な依存関係調査**
- **dylib 内部の参照パスも全て修正**
- **署名は最後に実施**

## 参考情報

### Max の Library Validation Entitlement

Max.app 自体は以下の Entitlement を持つ:
```xml
<key>com.apple.security.cs.disable-library-validation</key>
<true/>
```

しかし、これは「署名されていない dylib を許可」という意味ではなく、「バンドル内の dylib なら検証を緩和」という意味。外部の絶対パス参照は依然として拒否される。

### Cycling '74 公式ドキュメント

参考: [Using Unsigned Max Externals on Mac OS 10.15 Catalina](https://cycling74.com/articles/using-unsigned-max-externals-on-mac-os-10-15-catalina)

このドキュメントは Gatekeeper の回避方法を説明しているが、今回の問題は **Max 内部のセキュリティポリシー** であり、Gatekeeper とは別の問題。

## 結論

Max External で外部 dylib を使用する場合、以下の手順を厳守する必要がある:

### ビルド時の必須手順

1. **Frameworks ディレクトリの作成**
   ```bash
   mkdir -p YourExternal.mxo/Contents/Frameworks
   ```

2. **全ての依存 dylib をコピー**
   ```bash
   cp /path/to/libfoo.dylib YourExternal.mxo/Contents/Frameworks/
   ```

3. **メイン実行ファイルの参照を修正**
   ```bash
   install_name_tool -change /absolute/path/to/libfoo.dylib \
       @loader_path/../Frameworks/libfoo.dylib \
       YourExternal.mxo/Contents/MacOS/YourExternal
   ```

4. **各 dylib の ID と内部参照を修正**
   ```bash
   # dylib の ID を修正
   install_name_tool -id @loader_path/../Frameworks/libfoo.dylib \
       YourExternal.mxo/Contents/Frameworks/libfoo.dylib

   # dylib が参照する他の dylib のパスを修正
   install_name_tool -change /absolute/path/to/libbar.dylib \
       @loader_path/libbar.dylib \
       YourExternal.mxo/Contents/Frameworks/libfoo.dylib
   ```

5. **全ての変更完了後に署名**
   ```bash
   # .backup ファイルの削除 (install_name_tool が生成)
   find YourExternal.mxo -name "*.backup" -delete

   # Ad-hoc 署名
   codesign --force --deep --sign - YourExternal.mxo
   ```

6. **検証**
   ```bash
   # 署名の検証
   codesign --verify --deep --strict YourExternal.mxo

   # 依存関係の確認
   otool -L YourExternal.mxo/Contents/MacOS/YourExternal
   otool -L YourExternal.mxo/Contents/Frameworks/*.dylib
   ```

### CMake での自動化

完全な実装例: [CMakeLists.txt:121-195](../CMakeLists.txt)

```cmake
if(APPLE AND BUILD_MODE STREQUAL "server")
    # 1. Create Frameworks directory
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory
            "$<TARGET_BUNDLE_DIR:${PROJECT_NAME}>/Contents/Frameworks"
    )

    # 2. Copy dylibs and fix paths
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        # Copy all dylibs
        COMMAND ${CMAKE_COMMAND} -E copy ...

        # Fix main executable references
        COMMAND install_name_tool -change ...

        # Fix dylib IDs and internal references
        COMMAND install_name_tool -id ...
        COMMAND install_name_tool -change ...
    )
endif()

# 3. Code signing (after all modifications)
if(APPLE)
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND find "$<TARGET_BUNDLE_DIR:${PROJECT_NAME}>" -name "*.backup" -delete
        COMMAND codesign --force --deep --sign - "$<TARGET_BUNDLE_DIR:${PROJECT_NAME}>"
    )
endif()
```

### 推奨事項

- **CMake で完全に自動化**: 手動での修正は忘れやすく、エラーの原因になる
- **otool で徹底検証**: ビルド後に全ての dylib の依存関係を確認
- **リリース時は正式な署名**: Ad-hoc 署名は開発用。配布時は Developer ID で署名

### 注意事項

- **Universal Binary の制約**: Homebrew の dylib は arm64 のみの場合が多い。Universal Binary が必要な場合は、x86_64 版も別途ビルドして lipo でマージする必要がある。
- **署名の順序**: `install_name_tool` の後に署名。順序を間違えると署名が無効化される。
- **`.backup` ファイルの削除**: `install_name_tool` は `.backup` ファイルを生成するため、署名前に削除が必要。

## 今後の課題

- [ ] リリースビルド時の正式な署名手順の確立（Developer ID 証明書）
- [ ] Universal Binary 対応（x86_64 + arm64）
- [ ] CI/CD での自動署名の実装
- [ ] 他の Max External 開発者向けのドキュメント整備
