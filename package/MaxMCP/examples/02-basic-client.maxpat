{
	"patcher": {
		"fileversion": 1,
		"appversion": {
			"major": 9,
			"minor": 0,
			"revision": 0
		},
		"rect": [100, 100, 600, 400],
		"boxes": [
			{
				"box": {
					"maxclass": "comment",
					"text": "Phase 2 - Test 1: Basic Client Registration",
					"fontsize": 14,
					"patching_rect": [20, 20, 400, 22]
				}
			},
			{
				"box": {
					"maxclass": "comment",
					"text": "Expected: Max Console shows \"maxmcp client initialized (ID: phase2-test1-basic-client_XXXXXXXX)\"",
					"patching_rect": [20, 50, 550, 20]
				}
			},
			{
				"box": {
					"maxclass": "comment",
					"text": "Patch ID format: {patchname}_{8-char-uuid}",
					"patching_rect": [20, 75, 350, 20]
				}
			},
			{
				"box": {
					"maxclass": "newobj",
					"text": "maxmcp",
					"patching_rect": [20, 120, 60, 22],
					"numinlets": 0,
					"numoutlets": 0,
					"id": "obj-1"
				}
			},
			{
				"box": {
					"maxclass": "comment",
					"text": "↑ This maxmcp object should auto-register with default patch ID",
					"patching_rect": [90, 120, 400, 20]
				}
			},
			{
				"box": {
					"maxclass": "panel",
					"bgcolor": [0.2, 0.3, 0.4, 1.0],
					"patching_rect": [10, 10, 570, 140]
				}
			}
		],
		"lines": []
	}
}
