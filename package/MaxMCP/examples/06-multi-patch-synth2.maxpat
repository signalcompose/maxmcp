{
	"patcher": {
		"fileversion": 1,
		"appversion": {
			"major": 9,
			"minor": 0,
			"revision": 0
		},
		"rect": [150, 150, 600, 350],
		"boxes": [
			{
				"box": {
					"maxclass": "comment",
					"text": "Phase 2 - Test 4: Multi-Patch (Synth 2)",
					"fontsize": 14,
					"patching_rect": [20, 20, 400, 22]
				}
			},
			{
				"box": {
					"maxclass": "newobj",
					"text": "maxmcp @alias synth2 @group synths",
					"patching_rect": [20, 70, 230, 22],
					"numinlets": 0,
					"numoutlets": 0,
					"id": "obj-1"
				}
			},
			{
				"box": {
					"maxclass": "comment",
					"text": "↑ Synth 2 - Group: synths (same group as synth1)",
					"patching_rect": [260, 70, 300, 20]
				}
			},
			{
				"box": {
					"maxclass": "panel",
					"bgcolor": [0.4, 0.3, 0.5, 1.0],
					"patching_rect": [10, 10, 570, 100]
				}
			}
		],
		"lines": []
	}
}
