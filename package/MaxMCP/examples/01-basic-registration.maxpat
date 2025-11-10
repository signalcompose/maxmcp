{
	"patcher": {
		"fileversion": 1,
		"appversion": {
			"major": 9,
			"minor": 0,
			"revision": 0
		},
		"rect": [100, 100, 700, 450],
		"boxes": [
			{
				"box": {
					"maxclass": "comment",
					"text": "Test 1: Basic Client Registration",
					"fontsize": 14,
					"fontface": 1,
					"patching_rect": [20, 20, 400, 22]
				}
			},
			{
				"box": {
					"maxclass": "comment",
					"text": "This test demonstrates the simplest MaxMCP client setup.",
					"patching_rect": [20, 50, 500, 20]
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
					"text": "1. Patch auto-registers when opened",
					"patching_rect": [20, 110, 400, 20]
				}
			},
			{
				"box": {
					"maxclass": "comment",
					"text": "2. Patch ID = {filename}_{8-char-uuid}",
					"patching_rect": [20, 130, 400, 20]
				}
			},
			{
				"box": {
					"maxclass": "comment",
					"text": "3. No custom alias, no group assignment",
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
					"text": "\"maxmcp client initialized (ID: 01-basic-registration_XXXXXXXX)\"",
					"textcolor": [0.0, 0.7, 0.0, 1.0],
					"fontname": "Monaco",
					"fontsize": 10,
					"patching_rect": [20, 210, 600, 18]
				}
			},
			{
				"box": {
					"maxclass": "comment",
					"text": "Test from Claude Code:",
					"fontsize": 12,
					"fontface": 1,
					"patching_rect": [20, 245, 300, 20]
				}
			},
			{
				"box": {
					"maxclass": "comment",
					"text": "\"List all active patches\" → Should show this patch with auto-generated ID",
					"patching_rect": [20, 270, 600, 20]
				}
			},
			{
				"box": {
					"maxclass": "newobj",
					"text": "maxmcp @mode patch",
					"patching_rect": [20, 320, 130, 22],
					"numinlets": 0,
					"numoutlets": 0,
					"id": "obj-maxmcp"
				}
			},
			{
				"box": {
					"maxclass": "comment",
					"text": "← MaxMCP client (no attributes = auto-generated ID)",
					"patching_rect": [160, 325, 400, 20]
				}
			},
			{
				"box": {
					"maxclass": "comment",
					"text": "NOTE: This is a simple registration test. No actual Max objects here.",
					"textcolor": [0.5, 0.5, 0.5, 1.0],
					"fontsize": 10,
					"patching_rect": [20, 365, 550, 18]
				}
			},
			{
				"box": {
					"maxclass": "panel",
					"bgcolor": [0.2, 0.3, 0.4, 1.0],
					"patching_rect": [10, 10, 670, 380]
				}
			}
		],
		"lines": []
	}
}
