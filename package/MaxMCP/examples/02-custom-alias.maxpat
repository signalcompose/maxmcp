{
	"patcher": {
		"fileversion": 1,
		"appversion": {
			"major": 9,
			"minor": 0,
			"revision": 0
		},
		"rect": [150, 150, 700, 450],
		"boxes": [
			{
				"box": {
					"maxclass": "comment",
					"text": "Test 2: Custom Patch ID (@alias)",
					"fontsize": 14,
					"fontface": 1,
					"patching_rect": [20, 20, 400, 22]
				}
			},
			{
				"box": {
					"maxclass": "comment",
					"text": "This test demonstrates custom patch ID using @alias attribute.",
					"patching_rect": [20, 50, 550, 20]
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
					"text": "1. Patch auto-registers with custom ID \"my_synth\"",
					"patching_rect": [20, 110, 450, 20]
				}
			},
			{
				"box": {
					"maxclass": "comment",
					"text": "2. No UUID suffix (exact ID = my_synth)",
					"patching_rect": [20, 130, 400, 20]
				}
			},
			{
				"box": {
					"maxclass": "comment",
					"text": "3. No group assignment",
					"patching_rect": [20, 150, 400, 20]
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
					"text": "\"maxmcp client initialized (ID: my_synth)\"",
					"textcolor": [0.0, 0.7, 0.0, 1.0],
					"fontname": "Monaco",
					"fontsize": 10,
					"patching_rect": [20, 210, 600, 18]
				}
			},
			{
				"box": {
					"maxclass": "comment",
					"text": "\"Using custom patch ID: my_synth\"",
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
					"text": "\"List all active patches\" → Should show patch_id: \"my_synth\" (exact match)",
					"patching_rect": [20, 290, 650, 20]
				}
			},
			{
				"box": {
					"maxclass": "newobj",
					"text": "maxmcp @mode patch @alias my_synth",
					"patching_rect": [20, 340, 230, 22],
					"numinlets": 0,
					"numoutlets": 0,
					"id": "obj-maxmcp"
				}
			},
			{
				"box": {
					"maxclass": "comment",
					"text": "← MaxMCP client with custom @alias",
					"patching_rect": [260, 345, 300, 20]
				}
			},
			{
				"box": {
					"maxclass": "comment",
					"text": "NOTE: @alias overrides default patch ID generation.",
					"textcolor": [0.5, 0.5, 0.5, 1.0],
					"fontsize": 10,
					"patching_rect": [20, 385, 450, 18]
				}
			},
			{
				"box": {
					"maxclass": "panel",
					"bgcolor": [0.2, 0.4, 0.3, 1.0],
					"patching_rect": [10, 10, 670, 400]
				}
			}
		],
		"lines": []
	}
}
