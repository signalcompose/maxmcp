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
					"text": "Phase 2 - Test 2: Custom Patch ID (@alias)",
					"fontsize": 14,
					"patching_rect": [20, 20, 400, 22]
				}
			},
			{
				"box": {
					"maxclass": "comment",
					"text": "Expected Console Output:",
					"patching_rect": [20, 50, 200, 20]
				}
			},
			{
				"box": {
					"maxclass": "comment",
					"text": "1. \"maxmcp client initialized (ID: my_synth)\"",
					"patching_rect": [20, 70, 350, 20]
				}
			},
			{
				"box": {
					"maxclass": "comment",
					"text": "2. \"Using custom patch ID: my_synth\"",
					"patching_rect": [20, 90, 350, 20]
				}
			},
			{
				"box": {
					"maxclass": "newobj",
					"text": "maxmcp @alias my_synth",
					"patching_rect": [20, 130, 150, 22],
					"numinlets": 0,
					"numoutlets": 0,
					"id": "obj-1"
				}
			},
			{
				"box": {
					"maxclass": "comment",
					"text": "↑ @alias attribute overrides default patch ID",
					"patching_rect": [180, 130, 350, 20]
				}
			},
			{
				"box": {
					"maxclass": "comment",
					"text": "Patch ID should be exactly \"my_synth\" (no UUID suffix)",
					"patching_rect": [20, 160, 400, 20]
				}
			},
			{
				"box": {
					"maxclass": "panel",
					"bgcolor": [0.2, 0.4, 0.3, 1.0],
					"patching_rect": [10, 10, 570, 180]
				}
			}
		],
		"lines": []
	}
}
