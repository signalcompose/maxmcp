{
	"patcher": {
		"fileversion": 1,
		"appversion": {
			"major": 9,
			"minor": 0,
			"revision": 0
		},
		"rect": [200, 200, 600, 350],
		"boxes": [
			{
				"box": {
					"maxclass": "comment",
					"text": "Phase 2 - Test 4: Multi-Patch (FX 1)",
					"fontsize": 14,
					"patching_rect": [20, 20, 400, 22]
				}
			},
			{
				"box": {
					"maxclass": "newobj",
					"text": "maxmcp @alias fx1 @group effects",
					"patching_rect": [20, 70, 210, 22],
					"numinlets": 0,
					"numoutlets": 0,
					"id": "obj-1"
				}
			},
			{
				"box": {
					"maxclass": "comment",
					"text": "↑ FX 1 - Group: effects (different group)",
					"patching_rect": [240, 70, 300, 20]
				}
			},
			{
				"box": {
					"maxclass": "comment",
					"text": "Expected: 3 patches registered with 2 different groups",
					"patching_rect": [20, 110, 400, 20]
				}
			},
			{
				"box": {
					"maxclass": "panel",
					"bgcolor": [0.5, 0.4, 0.3, 1.0],
					"patching_rect": [10, 10, 570, 130]
				}
			}
		],
		"lines": []
	}
}
