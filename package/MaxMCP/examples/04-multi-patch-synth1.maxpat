{
	"patcher": {
		"fileversion": 1,
		"appversion": {
			"major": 9,
			"minor": 0,
			"revision": 0
		},
		"rect": [100, 100, 650, 400],
		"boxes": [
			{
				"box": {
					"maxclass": "comment",
					"text": "Test 4: Multi-Patch (Synth 1)",
					"fontsize": 14,
					"fontface": 1,
					"patching_rect": [20, 20, 400, 22]
				}
			},
			{
				"box": {
					"maxclass": "comment",
					"text": "Part of multi-patch test. Open all three patches:",
					"patching_rect": [20, 50, 450, 20]
				}
			},
			{
				"box": {
					"maxclass": "comment",
					"text": "1. 04-multi-patch-synth1.maxpat (this patch) → synth1 / synths",
					"patching_rect": [20, 75, 550, 20]
				}
			},
			{
				"box": {
					"maxclass": "comment",
					"text": "2. 05-multi-patch-synth2.maxpat → synth2 / synths",
					"patching_rect": [20, 95, 550, 20]
				}
			},
			{
				"box": {
					"maxclass": "comment",
					"text": "3. 06-multi-patch-fx1.maxpat → fx1 / effects",
					"patching_rect": [20, 115, 550, 20]
				}
			},
			{
				"box": {
					"maxclass": "comment",
					"text": "Expected Behavior:",
					"fontsize": 12,
					"fontface": 1,
					"patching_rect": [20, 150, 200, 20]
				}
			},
			{
				"box": {
					"maxclass": "comment",
					"text": "• List all patches → 3 patches (synth1, synth2, fx1)",
					"patching_rect": [20, 175, 500, 20]
				}
			},
			{
				"box": {
					"maxclass": "comment",
					"text": "• List synths group → 2 patches (synth1, synth2)",
					"patching_rect": [20, 195, 500, 20]
				}
			},
			{
				"box": {
					"maxclass": "comment",
					"text": "• List effects group → 1 patch (fx1)",
					"patching_rect": [20, 215, 500, 20]
				}
			},
			{
				"box": {
					"maxclass": "newobj",
					"text": "maxmcp @mode patch @alias synth1 @group synths",
					"patching_rect": [20, 260, 340, 22],
					"numinlets": 0,
					"numoutlets": 0,
					"id": "obj-maxmcp"
				}
			},
			{
				"box": {
					"maxclass": "comment",
					"text": "← Synth 1 - Group: synths",
					"patching_rect": [370, 265, 250, 20]
				}
			},
			{
				"box": {
					"maxclass": "comment",
					"text": "NOTE: Same group as synth2, different from fx1",
					"textcolor": [0.5, 0.5, 0.5, 1.0],
					"fontsize": 10,
					"patching_rect": [20, 305, 450, 18]
				}
			},
			{
				"box": {
					"maxclass": "panel",
					"bgcolor": [0.4, 0.3, 0.5, 1.0],
					"patching_rect": [10, 10, 620, 330]
				}
			}
		],
		"lines": []
	}
}
