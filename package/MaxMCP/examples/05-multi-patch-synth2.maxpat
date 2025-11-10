{
	"patcher": {
		"fileversion": 1,
		"appversion": {
			"major": 9,
			"minor": 0,
			"revision": 0
		},
		"rect": [150, 150, 650, 300],
		"boxes": [
			{
				"box": {
					"maxclass": "comment",
					"text": "Test 5: Multi-Patch (Synth 2)",
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
					"text": "This patch belongs to the same \"synths\" group as synth1.",
					"patching_rect": [20, 75, 500, 20]
				}
			},
			{
				"box": {
					"maxclass": "newobj",
					"text": "maxmcp @mode patch @alias synth2 @group synths",
					"patching_rect": [20, 120, 340, 22],
					"numinlets": 0,
					"numoutlets": 0,
					"id": "obj-maxmcp"
				}
			},
			{
				"box": {
					"maxclass": "comment",
					"text": "← Synth 2 - Group: synths (same as synth1)",
					"patching_rect": [370, 125, 250, 20]
				}
			},
			{
				"box": {
					"maxclass": "comment",
					"text": "NOTE: Group filter test requires all 3 patches open",
					"textcolor": [0.5, 0.5, 0.5, 1.0],
					"fontsize": 10,
					"patching_rect": [20, 165, 450, 18]
				}
			},
			{
				"box": {
					"maxclass": "panel",
					"bgcolor": [0.4, 0.3, 0.5, 1.0],
					"patching_rect": [10, 10, 620, 190]
				}
			}
		],
		"lines": []
	}
}
