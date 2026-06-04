# MCP Integration Test Checklist

**Total Tools**: 29
**Purpose**: Manual verification checklist for all MCP tools. Use before PR merges and releases.

---

## Prerequisites

- [ ] Max/MSP is running
- [ ] A patch containing `maxmcp @mode patch` is open
- [ ] Claude Code is connected to the MaxMCP MCP server
- [ ] `list_active_patches` detects the patch

---

## Patch Management (3)

| # | Tool                  | Test                          | Expected                                     | Pass |
|---|-----------------------|-------------------------------|----------------------------------------------|------|
| 1 | `list_active_patches` | List all patches              | At least 1 patch returned                    | [ ]  |
| 2 | `get_patch_info`      | Query by patch_id             | Returns display_name, patcher_name, etc.     | [ ]  |
| 3 | `get_frontmost_patch` | Get frontmost patch           | Returns currently focused patch info         | [ ]  |

## Object Operations (12)

| #  | Tool                   | Test                              | Expected                                  | Pass |
|----|------------------------|-----------------------------------|-------------------------------------------|------|
| 4  | `add_max_object`       | Add `cycle~ 440`                  | status: success, varname returned         | [ ]  |
| 5  | `remove_max_object`    | Remove added object               | status: success                           | [ ]  |
| 6  | `get_objects_in_patch` | List objects (no `mode`)          | Full metadata: index/varname/maxclass/text/position/size | [ ]  |
| 6a | `get_objects_in_patch` | `mode: "layout"`                  | Only varname/position/size returned       | [ ]  |
| 6b | `get_objects_in_patch` | `mode: "identity"`                | Only index/varname/maxclass/text returned | [ ]  |
| 7  | `set_object_attribute` | Change bgcolor of number box      | status: success, color changes in patch   | [ ]  |
| 8  | `get_object_attribute` | Get patching_rect                 | Returns [x, y, width, height] array       | [ ]  |
| 9  | `get_object_value`     | Get number box value              | Returns number (default: 0)               | [ ]  |
| 10 | `get_object_io_info`   | Get IO info for cycle~            | inlet_count: 2, outlet_count: 1           | [ ]  |
| 11 | `get_object_hidden`    | Get hidden state                  | hidden: false (default)                   | [ ]  |
| 12 | `set_object_hidden`    | Hide then unhide                  | hidden: true then hidden: false           | [ ]  |
| 13 | `redraw_object`        | Force redraw                      | success: true                             | [ ]  |
| 14 | `replace_object_text`  | Change `cycle~ 440` to `cycle~ 880` | old_text/new_text returned correctly   | [ ]  |
| 15 | `assign_varnames`      | Assign varname to unnamed object  | assigned: 1, varname set                  | [ ]  |

## Connection Operations (4)

| #  | Tool                      | Test                       | Expected                                   | Pass |
|----|---------------------------|----------------------------|--------------------------------------------|------|
| 16 | `connect_max_objects`     | Connect two objects        | status: success, patchcord visible         | [ ]  |
| 17 | `disconnect_max_objects`  | Disconnect objects         | status: success, patchcord removed         | [ ]  |
| 18  | `get_patchlines`         | List patchcords (no `mode`)  | Full metadata: topology + start/end + midpoints + hidden + color | [ ]  |
| 18a | `get_patchlines`         | `mode: "geometry"`           | Topology + start/end + midpoints (no hidden/color)               | [ ]  |
| 18b | `get_patchlines`         | `mode: "connections"`        | Topology only (src_varname/outlet/dst_varname/inlet)             | [ ]  |
| 19 | `set_patchline_midpoints` | Set then clear midpoints   | Midpoints set / cleared with empty array   | [ ]  |

## Patch State (3)

| #  | Tool                   | Test                    | Expected                                         | Pass |
|----|------------------------|-------------------------|--------------------------------------------------|------|
| 20 | `get_patch_lock_state` | Get lock state          | locked: true/false returned                      | [ ]  |
| 21 | `set_patch_lock_state` | Toggle lock/unlock      | Patch switches between edit/presentation mode    | [ ]  |
| 22 | `get_patch_dirty`      | Get dirty state         | dirty: true/false returned                       | [ ]  |

## Hierarchy (2)

| #  | Tool                 | Test                        | Expected                              | Pass |
|----|----------------------|-----------------------------|---------------------------------------|------|
| 23 | `get_parent_patcher` | Run on top-level patch      | Error: "No parent patcher" (expected) | [ ]  |
| 24 | `get_subpatchers`    | Run on patch with no subs   | count: 0, subpatchers: []             | [ ]  |

## Utilities (2)

| #   | Tool                      | Test                                  | Expected                                                              | Pass |
|-----|---------------------------|---------------------------------------|----------------------------------------------------------------------|------|
| 25  | `get_console_log`         | Retrieve console log                  | Array of log messages returned                                       | [ ]  |
| 26  | `get_avoid_rect_position` | No `near_x`/`near_y`                   | Placed to the right of existing objects; rationale mentions "right"  | [ ]  |
| 26a | `get_avoid_rect_position` | `near_x`/`near_y` at a free spot      | Returns that exact point; rationale mentions "near"                  | [ ]  |
| 26b | `get_avoid_rect_position` | `near_x`/`near_y` on top of an object | Returns the nearest non-overlapping spot; rationale mentions "nearest" | [ ]  |
| 26c | `get_avoid_rect_position` | `width`/`height` of a large object    | Returned spot clears all objects by the gap for that size; rationale notes the size | [ ]  |
| 26d | `get_avoid_rect_position` | Negative `near_x`/`near_y`            | Result clamped to non-negative coordinates (x ≥ 0, y ≥ 0)           | [ ]  |

## Layout Validation (3)

`validate_layout` is read-only. For each case, set up the patch state described, then run the tool and confirm the finding (or `clean`).

| #   | Tool              | Test                                                                 | Expected                                                                                  | Pass |
|-----|-------------------|----------------------------------------------------------------------|-------------------------------------------------------------------------------------------|------|
| 27  | `validate_layout` | Tidy patch (no overlaps/crossings)                                   | `clean: true`, empty `findings`, all `summary` counters 0                                  | [ ]  |
| 27a | `validate_layout` | Two objects overlapping by more than `epsilon`                       | One `overlap` finding (severity error); `detail` reports area + overlap region bounds      | [ ]  |
| 27b | `validate_layout` | Straight cord routed upward (`start.y > end.y`, no midpoints)        | One `upward` finding, severity **error**                                                    | [ ]  |
| 27c | `validate_layout` | Folded cord (midpoints) with an upward intermediate segment          | `upward` finding, severity **warning**                                                      | [ ]  |
| 27d | `validate_layout` | Cord passing through an unrelated object's rect                      | One `cord_object` finding; `object` names the obstacle; `detail` gives the crossing point  | [ ]  |
| 27e | `validate_layout` | Cord whose endpoint object lies on its path                          | That endpoint object is **not** reported (src/dst excluded)                                 | [ ]  |
| 27f | `validate_layout` | Two cords collinear on the same row/column with overlapping spans    | One `cord_cord` finding (severity warning)                                                  | [ ]  |
| 27g | `validate_layout` | `scope_varnames` limited to one section                             | Only objects/cords within that section are checked                                          | [ ]  |
| 27h | `validate_layout` | `mode: "presentation"` with overlapping `presentation_rect`         | `presentation_overlap` finding; cord checks skipped                                         | [ ]  |
| 27i | `validate_layout` | `checks` subset (e.g. `["overlap"]`)                                | Only the requested check runs; others stay 0                                               | [ ]  |
| 27j | `validate_layout` | Fix a reported finding, then re-run                                  | The finding disappears; repeat until `clean: true` (validate → fix → re-validate loop)     | [ ]  |

`get_io_position` is read-only. The ground-truth cross-check is a connected cord's real endpoint (`get_patchlines mode:"geometry"` `start_point` for an outlet, `end_point` for an inlet).

| #   | Tool              | Test                                                                 | Expected                                                                                   | Pass |
|-----|-------------------|----------------------------------------------------------------------|--------------------------------------------------------------------------------------------|------|
| 28  | `get_io_position` | `side:"inlet"` on a 2+ inlet object (e.g. `cycle~`)                  | `count` matches inlet count; `positions[i].y` = object top; x values match cord endpoints  | [ ]  |
| 28a | `get_io_position` | `side:"outlet"` on a multi-outlet object (e.g. `number`, `live.gain~`) | `positions[i].y` = object bottom; x equally spaced, matches cord `start_point`s            | [ ]  |
| 28b | `get_io_position` | Cross-check: connect a cord to inlet `i`, compare to `end_point`      | `positions[i]` agrees with the cord endpoint within ~1px                                    | [ ]  |
| 28c | `get_io_position` | Tall object (`gain~`, `slider`)                                       | inlet y = top, outlet y = bottom (height handled); x uses the calibrated inset             | [ ]  |
| 28d | `get_io_position` | Unknown `varname`                                                    | Error: "Object not found: <varname>"                                                        | [ ]  |
| 28e | `get_io_position` | Invalid `side` (not inlet/outlet)                                    | Error: side must be "inlet" or "outlet"                                                      | [ ]  |

`suggest_alignment` is read-only (returns a recommended rect; apply it with `set_object_attribute`). Verify by applying the rect, then `get_io_position` on both nubs to confirm they share x.

| #   | Tool                | Test                                                                        | Expected                                                                                       | Pass |
|-----|---------------------|-----------------------------------------------------------------------------|------------------------------------------------------------------------------------------------|------|
| 29  | `suggest_alignment` | `adjust:"width"`: size a multi-outlet object so an outlet lands on an inlet  | `recommended_patching_rect` returned; after applying, `get_io_position` outlet x == anchor x   | [ ]  |
| 29a | `suggest_alignment` | `adjust:"left"`: move a target so its inlet lands under an anchor outlet     | left changes, width unchanged; after applying, the two nubs share x                             | [ ]  |
| 29b | `suggest_alignment` | `adjust:"width"` targeting the leftmost nub (index 0)                        | Error suggesting `adjust:"left"` (leftmost nub is width-independent)                            | [ ]  |
| 29c | `suggest_alignment` | `adjust:"width"` on a single-nub side                                       | Error suggesting `adjust:"left"`                                                                | [ ]  |
| 29d | `suggest_alignment` | Unknown `varname` (anchor or target), or bad `side`/`adjust`                | Appropriate error; patch is never modified                                                       | [ ]  |

---

## Recommended Test Flow

1. **Verify connection**: Check all 4 prerequisites
2. **Create objects**: Add number, cycle~, gain~, dac~ (#4)
3. **Object operations**: Verify attributes, values, IO, visibility (#6-15)
4. **Connections**: Connect and disconnect objects (#16-19)
5. **Patch state**: Check lock/dirty state (#20-22)
6. **Hierarchy**: Verify parent/subpatcher queries (#23-24)
7. **Utilities**: Test log and position tools (#25-26)
8. **Layout validation**: Deliberately break the layout (overlap, upward cord, crossing), confirm `validate_layout` reports each, fix and re-validate to `clean` (#27)
9. **Cleanup**: Remove test objects (#5)

---

## Notes

- `get_parent_patcher` returns an error on top-level patches — this is expected behavior
- `get_subpatchers` returns an empty array when no subpatchers exist — this is expected behavior
- Full hierarchy testing requires a patch containing subpatchers (p, poly~, bpatcher, etc.)
- `validate_layout` is read-only (never moves objects or cords); it only reports findings. The intended workflow is validate → fix → re-validate until `clean: true`
- `validate_layout` cord endpoints use the patchline's real start/end points (`jpatchline_get_startpoint/endpoint`); per-inlet/outlet pixel positions (`get_io_position`) are a separate future tool (see `docs/layout-validation-tools-spec.md`)
