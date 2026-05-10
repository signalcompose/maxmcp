# MCP Integration Test Checklist

**Total Tools**: 26
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
| 18 | `get_patchlines`          | List all patchcords        | patchlines array with connection info      | [ ]  |
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

| #  | Tool                      | Test                    | Expected                                      | Pass |
|----|---------------------------|-------------------------|-----------------------------------------------|------|
| 25 | `get_console_log`         | Retrieve console log    | Array of log messages returned                | [ ]  |
| 26 | `get_avoid_rect_position` | Find empty position     | Returns coordinates avoiding existing objects | [ ]  |

---

## Recommended Test Flow

1. **Verify connection**: Check all 4 prerequisites
2. **Create objects**: Add number, cycle~, gain~, dac~ (#4)
3. **Object operations**: Verify attributes, values, IO, visibility (#6-15)
4. **Connections**: Connect and disconnect objects (#16-19)
5. **Patch state**: Check lock/dirty state (#20-22)
6. **Hierarchy**: Verify parent/subpatcher queries (#23-24)
7. **Utilities**: Test log and position tools (#25-26)
8. **Cleanup**: Remove test objects (#5)

---

## Notes

- `get_parent_patcher` returns an error on top-level patches — this is expected behavior
- `get_subpatchers` returns an empty array when no subpatchers exist — this is expected behavior
- Full hierarchy testing requires a patch containing subpatchers (p, poly~, bpatcher, etc.)
