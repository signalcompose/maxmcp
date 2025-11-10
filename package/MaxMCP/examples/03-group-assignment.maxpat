{
	"patcher": {
		"fileversion": 1,
		"appversion": {
			"major": 9,
			"minor": 0,
			"revision": 0
		},
		"rect": [200, 200, 700, 500],
		"boxes": [
			{
				"box": {
					"maxclass": "comment",
					"text": "Test 3: Group Assignment (@alias + @group)",
					"fontsize": 14,
					"fontface": 1,
					"patching_rect": [20, 20, 450, 22]
				}
			},
			{
				"box": {
					"maxclass": "comment",
					"text": "This test demonstrates group assignment for multi-patch organization.",
					"patching_rect": [20, 50, 600, 20]
				}
			},
			{
				"box": {
					"maxclass": "comment",
					"text": "Expected Behavior:",
					"fontsize": 12,
					"fontface": 1,
					"patching_rect": [20, 85, 200, 20]
				}
			},
			{
				"box": {
					"maxclass": "comment",
					"text": "1. Patch auto-registers with ID \"group-test\"",
					"patching_rect": [20, 110, 450, 20]
				}
			},
			{
				"box": {
					"maxclass": "comment",
					"text": "2. Assigned to group \"synths\"",
					"patching_rect": [20, 130, 400, 20]
				}
			},
			{
				"box": {
					"maxclass": "comment",
					"text": "3. Can be filtered by group in list_active_patches tool",
					"patching_rect": [20, 150, 500, 20]
				}
			},
			{
				"box": {
					"maxclass": "comment",
					"text": "Check Max Console (Cmd+B):",
					"fontsize": 12,
					"fontface": 1,
					"patching_rect": [20, 185, 300, 20]
				}
			},
			{
				"box": {
					"maxclass": "comment",
					"text": "\"maxmcp client initialized (ID: group-test)\"",
					"textcolor": [0.0, 0.7, 0.0, 1.0],
					"fontname": "Monaco",
					"fontsize": 10,
					"patching_rect": [20, 210, 600, 18]
				}
			},
			{
				"box": {
					"maxclass": "comment",
					"text": "\"Group: synths\"",
					"textcolor": [0.0, 0.7, 0.0, 1.0],
					"fontname": "Monaco",
					"fontsize": 10,
					"patching_rect": [20, 230, 600, 18]
				}
			},
			{
				"box": {
					"maxclass": "comment",
					"text": "Test from Claude Code:",
					"fontsize": 12,
					"fontface": 1,
					"patching_rect": [20, 265, 300, 20]
				}
			},
			{
				"box": {
					"maxclass": "comment",
					"text": "\"List all active patches\" → Should show patch_id: \"group-test\", group: \"synths\"",
					"patching_rect": [20, 290, 650, 20]
				}
			},
			{
				"box": {
					"maxclass": "comment",
					"text": "\"List patches in synths group\" → Should include this patch",
					"patching_rect": [20, 310, 650, 20]
				}
			},
			{
				"box": {
					"maxclass": "newobj",
					"text": "maxmcp @mode patch @alias group-test @group synths",
					"patching_rect": [20, 360, 340, 22],
					"numinlets": 0,
					"numoutlets": 0,
					"id": "obj-maxmcp"
				}
			},
			{
				"box": {
					"maxclass": "comment",
					"text": "← MaxMCP client with @alias and @group",
					"patching_rect": [370, 365, 300, 20]
				}
			},
			{
				"box": {
					"maxclass": "comment",
					"text": "NOTE: Groups enable organized multi-patch control.",
					"textcolor": [0.5, 0.5, 0.5, 1.0],
					"fontsize": 10,
					"patching_rect": [20, 405, 450, 18]
				}
			},
			{
				"box": {
					"maxclass": "comment",
					"text": "Use Case: \"Add a 440Hz oscillator to all synth patches\" → targets synths group",
					"textcolor": [0.5, 0.5, 0.5, 1.0],
					"fontsize": 10,
					"patching_rect": [20, 425, 650, 18]
				}
			},
			{
				"box": {
					"maxclass": "panel",
					"bgcolor": [0.3, 0.4, 0.2, 1.0],
					"patching_rect": [10, 10, 670, 450]
				}
			}
		],
		"lines": []
	}
}
