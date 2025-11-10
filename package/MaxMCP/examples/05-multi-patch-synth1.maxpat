{
	"patcher": {
		"fileversion": 1,
		"appversion": {
			"major": 9,
			"minor": 0,
			"revision": 0
		},
		"rect": [100, 100, 600, 350],
		"boxes": [
			{
				"box": {
					"maxclass": "comment",
					"text": "Phase 2 - Test 4: Multi-Patch (Synth 1)",
					"fontsize": 14,
					"patching_rect": [20, 20, 400, 22]
				}
			},
			{
				"box": {
					"maxclass": "comment",
					"text": "Open all three patches for Test 4:",
					"patching_rect": [20, 50, 300, 20]
				}
			},
			{
				"box": {
					"maxclass": "comment",
					"text": "1. phase2-test4-multi-patch-synth1.maxpat (this patch)",
					"patching_rect": [20, 70, 400, 20]
				}
			},
			{
				"box": {
					"maxclass": "comment",
					"text": "2. phase2-test4-multi-patch-synth2.maxpat",
					"patching_rect": [20, 90, 400, 20]
				}
			},
			{
				"box": {
					"maxclass": "comment",
					"text": "3. phase2-test4-multi-patch-fx1.maxpat",
					"patching_rect": [20, 110, 400, 20]
				}
			},
			{
				"box": {
					"maxclass": "newobj",
					"text": "maxmcp @alias synth1 @group synths",
					"patching_rect": [20, 150, 230, 22],
					"numinlets": 0,
					"numoutlets": 0,
					"id": "obj-1"
				}
			},
			{
				"box": {
					"maxclass": "comment",
					"text": "↑ Synth 1 - Group: synths",
					"patching_rect": [260, 150, 200, 20]
				}
			},
			{
				"box": {
					"maxclass": "panel",
					"bgcolor": [0.4, 0.3, 0.5, 1.0],
					"patching_rect": [10, 10, 570, 170]
				}
			}
		],
		"lines": []
	}
}
