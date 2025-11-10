{
	"patcher": {
		"fileversion": 1,
		"appversion": {
			"major": 9,
			"minor": 0,
			"revision": 0
		},
		"rect": [200, 200, 650, 300],
		"boxes": [
			{
				"box": {
					"maxclass": "comment",
					"text": "Test 6: Multi-Patch (FX 1)",
					"fontsize": 14,
					"fontface": 1,
					"patching_rect": [20, 20, 400, 22]
				}
			},
			{
				"box": {
					"maxclass": "comment",
					"text": "Part of multi-patch test. See 04-multi-patch-synth1 for details.",
					"patching_rect": [20, 50, 550, 20]
				}
			},
			{
				"box": {
					"maxclass": "comment",
					"text": "This patch belongs to \"effects\" group (different from synths).",
					"patching_rect": [20, 75, 500, 20]
				}
			},
			{
				"box": {
					"maxclass": "newobj",
					"text": "maxmcp @mode patch @alias fx1 @group effects",
					"patching_rect": [20, 120, 320, 22],
					"numinlets": 0,
					"numoutlets": 0,
					"id": "obj-maxmcp"
				}
			},
			{
				"box": {
					"maxclass": "comment",
					"text": "← FX 1 - Group: effects (different group)",
					"patching_rect": [350, 125, 270, 20]
				}
			},
			{
				"box": {
					"maxclass": "comment",
					"text": "NOTE: Total 3 patches with 2 groups (synths: 2, effects: 1)",
					"textcolor": [0.5, 0.5, 0.5, 1.0],
					"fontsize": 10,
					"patching_rect": [20, 165, 500, 18]
				}
			},
			{
				"box": {
					"maxclass": "panel",
					"bgcolor": [0.5, 0.4, 0.3, 1.0],
					"patching_rect": [10, 10, 620, 190]
				}
			}
		],
		"lines": []
	}
}
