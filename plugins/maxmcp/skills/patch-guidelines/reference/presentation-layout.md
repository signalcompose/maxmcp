# Presentation Mode Layout

Guidelines for arranging UI objects in presentation mode, where the goal shifts from patching clarity to user operability.

## 🔴 必読: アンチパターン

以下は presentation_rect 設計時に頻発する誤実装。**presentation 設定を行う前に必ず確認**。

### ❌ Anti-pattern 1: presentation_rect を patching_rect とほぼ同じ寸法にする

```
patching_rect:      [60, 700, 50, 18]   // 接続用に最小幅
presentation_rect:  [10, 80, 50, 18]    // ← 同じ幅 50px のまま放置
```

**症状**: ユーザーが numbox を操作する時にドラッグできる幅が狭くて使いにくい。Live のデバイス UI として未完成に見える。

**正解**: UI 要素は presentation 用に**幅を拡張**する:

```
patching_rect:      [60, 700, 50, 18]    // パッチング用は 50px
presentation_rect:  [40, 80, 130, 18]    // ← UI 用は 130px（2-3 倍に拡張）
```

ドラッグ操作する live.numbox / live.slider は特に幅を広げる。クリックする live.text / live.button は元の幅で良い。

### ❌ Anti-pattern 2: 処理ロジックを presentation に含める

```
set_object_attribute(varname="trigger_capture", attribute="presentation", value=1)
// ↑ trigger / pak / route / prepend は presentation に含めない
```

**症状**: ユーザーから見える UI に内部処理オブジェクトが表示されてしまう。デバイスが未完成・デバッグ中の見た目になる。

**正解**: presentation に**含めるべき**は以下のみ:
- ユーザー操作: `live.dial`, `live.numbox`, `live.slider`, `live.text`, `live.button`
- 値表示: 読み取り専用 `live.numbox`, `live.meter~`, `multislider`, `textedit`
- レイアウト: `panel`, `live.comment`（ラベル）
- ブランディング: `fpic`（ロゴ）, `textbutton`（情報ボタン）

**含めない**: `trigger`, `t`, `pak`, `pack`, `prepend`, `route`, `zl.*`, `send`, `receive`, `live.path`, `live.object`, `live.observer`, `live.thisdevice`, `pattr`, `comment`（開発者用ノート）

典型的なデバイスでは presentation に含まれるオブジェクト数は**全体の 10-15%**。それ以上含まれている場合、内部処理が混入していないか確認。

### ❌ Anti-pattern 3: presentation_rect の y 座標が patching_rect と連動

`presentation_rect` は `patching_rect` から**完全に独立**。patching でレイアウトを変更しても presentation は影響を受けない（逆も同様）。**この独立性を活かして、patching は信号フロー優先、presentation はユーザー操作優先で別々に設計する**。

---

## Dual-Mode Design Principle

Every UI object in a Max patch exists in two layouts simultaneously:

- **Patching mode (patching_rect)**: Optimized for signal flow visibility and connection access
- **Presentation mode (presentation_rect)**: Optimized for end-user interaction

The same object serves different purposes in each mode, and its size and position should reflect that.

### What Changes Between Modes

| Aspect | Patching Mode | Presentation Mode |
|--------|---------------|-------------------|
| **Position** | Near connected objects (minimize patchcord length) | Grouped by user interaction logic |
| **Width** | Minimum needed for connections | Expanded for easy mouse/touch interaction |
| **Height** | Usually unchanged | Usually unchanged (Max enforces minimums) |
| **Grouping** | By signal flow (inputs → processing → output) | By user task (navigation, display, actions) |

### Example: live.numbox

```
Patching:      [48 x 17]     ← narrow, near connected objects
Presentation:  [129 x 17]    ← 2.7x wider, fills available panel width
```

Width expands for easier interaction; height stays at 17px (the object's minimum display height).

### Example: Directional Buttons

```
Patching:                           Presentation:
  left [32x32]   right [32x32]           [up]
  (x=520)        (x=590)            [left] [right]
                                          [down]
  up [32x32]     down [32x32]
  (x=830)        (x=902)            Cross/D-pad layout
                                    matching directional intent
  Grouped by function               Grouped by spatial meaning
  (Track pair / Scene pair)         (directional cross)
```

In patching mode, left/right buttons sit near the Track processing chain, and up/down buttons sit near the Scene processing chain. In presentation mode, all four form a directional cross that matches their spatial meaning.

## Layout Strategies

### Expand Interactive Elements

Controls that users interact with frequently should be sized for comfortable use:

| Element | Patching Width | Presentation Width | Rationale |
|---------|---------------|-------------------|-----------|
| `live.numbox` | 48px (default) | Full panel width | Easier drag interaction |
| `live.text` | 60px | 50-60px | Button-sized, adequate for click |
| `live.button` | 32x32 | 23x23 | Can shrink to fit a compact layout |

**Rule**: Prioritize width for elements that require dragging (numbox, slider). Click targets (buttons) can be smaller if the layout demands it.

### Shrink Decorative Elements

Branding and informational elements that are prominent in patching mode should shrink in presentation:

| Element | Patching Size | Presentation Size | Rationale |
|---------|--------------|-------------------|-----------|
| Logo image (`fpic`) | 117x100 | 18.5x15.8 | Present but unobtrusive |
| Info button (`textbutton`) | 100x20 | 19.3x19.6 | Small square, corner placement |

**Rule**: Decorative elements should not compete with interactive elements for space.

### Reorganize by User Intent

Patching mode groups objects by signal flow. Presentation mode groups them by user task:

```
Patching layout:                    Presentation layout:

  Track section:                      Navigation area:
    left, right buttons                 D-pad (up/down/left/right)
    Track numbox                        Launch button
    Track processing chain
                                      Display area:
  Scene section:                        Track numbox (wide)
    up, down buttons                    Scene numbox (wide)
    Scene numbox
    Scene processing chain            Branding (corner):
                                        Info button, logo
  Launch section:
    live.text, fire trigger
```

## Selective Inclusion

Presentation mode is **opt-in per object**. Only objects that the end user needs to see or interact with should be included. All internal processing objects remain hidden.

### What to Include

| Include | Examples | Reason |
|---------|----------|--------|
| User controls | `live.button`, `live.numbox`, `live.slider`, `live.text`, `live.dial` | Direct user interaction |
| Value displays | `live.numbox` (read-only), `live.meter~`, `multislider` | Visual feedback |
| Background panels | `panel` | Layout structure and visual framing |
| Branding/info | `fpic` (logo), `textbutton` (info link) | Device identity |

### What to Exclude

| Exclude | Examples | Reason |
|---------|----------|--------|
| Processing logic | `trigger`, `change`, `int`, `+`, `clip`, `pak` | No user interaction |
| LOM infrastructure | `live.path`, `live.object`, `live.observer` | Internal plumbing |
| Message routing | `prepend`, `pack`, `zl.*`, `send`, `receive` | No visual purpose |
| Comments | `comment` | Developer notes, not for end users |

### Typical Ratio

In a production device, only a small fraction of objects appear in presentation mode. A typical ratio is 10-15% — for example, a device with ~80 total objects might have only ~12 in the presentation view. The remaining 85-90% are internal processing that the user never sees.

## Enabling Presentation Mode

To include an object in the presentation view:

```
set_object_attribute: attribute = "presentation", value = 1
```

To set the presentation position and size (independent of patching_rect):

```
set_object_attribute: attribute = "presentation_rect", value = [x, y, width, height]
```

**Important**: `presentation_rect` is completely independent of `patching_rect`. Changing one does not affect the other. This is what enables the dual-mode design.

