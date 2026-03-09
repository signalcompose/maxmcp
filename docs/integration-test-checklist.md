# MCP Integration Test Checklist

**Total Tools**: 26
**Purpose**: 実機テスト用チェックリスト。PR作成時やリリース前に全MCPツールの動作を確認する。

---

## Prerequisites

- [ ] Max/MSP が起動している
- [ ] `maxmcp @mode patch` を含むパッチが開かれている
- [ ] Claude Code が MaxMCP MCP サーバーに接続されている
- [ ] `list_active_patches` でパッチが検出される

---

## Patch Management (3)

| # | Tool                  | Test                        | Expected                            | Pass |
|---|-----------------------|-----------------------------|-------------------------------------|------|
| 1 | `list_active_patches` | パッチ一覧を取得            | パッチが1件以上表示される           | [ ]  |
| 2 | `get_patch_info`      | patch_id を指定して情報取得 | display_name, patcher_name 等が返る | [ ]  |
| 3 | `get_frontmost_patch` | 最前面パッチを取得          | 現在フォーカス中のパッチ情報が返る  | [ ]  |

## Object Operations (12)

| #  | Tool                   | Test                           | Expected                              | Pass |
|----|------------------------|--------------------------------|---------------------------------------|------|
| 4  | `add_max_object`       | `cycle~ 440` を追加            | status: success, varname が返る       | [ ]  |
| 5  | `remove_max_object`    | 追加したオブジェクトを削除     | status: success                       | [ ]  |
| 6  | `get_objects_in_patch` | パッチ内オブジェクト一覧取得   | オブジェクトの配列が返る              | [ ]  |
| 7  | `set_object_attribute` | number box の bgcolor を変更   | status: success, パッチ上で色が変わる | [ ]  |
| 8  | `get_object_attribute` | patching_rect を取得           | [x, y, width, height] の配列が返る    | [ ]  |
| 9  | `get_object_value`     | number box の値を取得          | 数値が返る (デフォルト: 0)            | [ ]  |
| 10 | `get_object_io_info`   | cycle~ の IO 情報取得          | inlet_count: 2, outlet_count: 1       | [ ]  |
| 11 | `get_object_hidden`    | 非表示状態を取得               | hidden: false (デフォルト)            | [ ]  |
| 12 | `set_object_hidden`    | 非表示に設定→元に戻す          | hidden: true → hidden: false          | [ ]  |
| 13 | `redraw_object`        | オブジェクトを再描画           | success: true                         | [ ]  |
| 14 | `replace_object_text`  | `cycle~ 440` → `cycle~ 880`    | old_text/new_text が正しく返る        | [ ]  |
| 15 | `assign_varnames`      | varname なしオブジェクトに付与 | assigned: 1, varname が設定される     | [ ]  |

## Connection Operations (4)

| #  | Tool                      | Test                    | Expected                                  | Pass |
|----|---------------------------|-------------------------|-------------------------------------------|------|
| 16 | `connect_max_objects`     | 2つのオブジェクトを接続 | status: success, パッチコードが表示される | [ ]  |
| 17 | `disconnect_max_objects`  | 接続を切断              | status: success, パッチコードが消える     | [ ]  |
| 18 | `get_patchlines`          | パッチコード一覧取得    | patchlines 配列に接続情報が含まれる       | [ ]  |
| 19 | `set_patchline_midpoints` | 中間点を設定→削除       | midpoints 設定/空配列で解除               | [ ]  |

## Patch State (3)

| #  | Tool                   | Test                  | Expected                                    | Pass |
|----|------------------------|-----------------------|---------------------------------------------|------|
| 20 | `get_patch_lock_state` | ロック状態を取得      | locked: true/false が返る                   | [ ]  |
| 21 | `set_patch_lock_state` | ロック→アンロック切替 | パッチの編集/プレゼンテーションが切り替わる | [ ]  |
| 22 | `get_patch_dirty`      | ダーティ状態を取得    | dirty: true/false が返る                    | [ ]  |

## Hierarchy (2)

| #  | Tool                 | Test                     | Expected                           | Pass |
|----|----------------------|--------------------------|------------------------------------|------|
| 23 | `get_parent_patcher` | トップレベルパッチで実行 | エラー: "No parent patcher" (正常) | [ ]  |
| 24 | `get_subpatchers`    | サブパッチャーなしで実行 | count: 0, subpatchers: []          | [ ]  |

## Utilities (2)

| # | Tool | Test | Expected | Pass |
|---|------|------|----------|------|
| 25 | `get_console_log` | コンソールログを取得 | ログメッセージの配列が返る | [ ] |
| 26 | `get_avoid_rect_position` | 空き位置を取得 | 既存オブジェクトと重ならない座標が返る | [ ] |

---

## Test Flow (推奨手順)

効率的にテストするための推奨手順:

1. **接続確認**: Prerequisites の4項目を確認
2. **オブジェクト作成**: number, cycle~, gain~, dac~ を作成 (#4)
3. **オブジェクト操作**: 属性/値/IO/表示を確認 (#6-15)
4. **接続操作**: オブジェクトを接続→切断 (#16-19)
5. **パッチ状態**: ロック/ダーティ確認 (#20-22)
6. **階層**: 親パッチャー/サブパッチャー確認 (#23-24)
7. **ユーティリティ**: ログ/位置取得 (#25-26)
8. **クリーンアップ**: テスト用オブジェクトを削除 (#5)

---

## Notes

- `get_parent_patcher` はトップレベルパッチに対してエラーを返すのが正常動作
- `get_subpatchers` はサブパッチャーがなければ空配列を返すのが正常動作
- Hierarchy の完全なテストにはサブパッチャー（p, poly~, bpatcher 等）を含むパッチが必要
