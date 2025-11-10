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
					"text": "Phase 2 - Test 3: Group Assignment (@group)",
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
					"text": "1. \"maxmcp client initialized (ID: synth1)\"",
					"patching_rect": [20, 70, 350, 20]
				}
			},
			{
				"box": {
					"maxclass": "comment",
					"text": "2. \"Group: synths\"",
					"patching_rect": [20, 90, 350, 20]
				}
			},
			{
				"box": {
					"maxclass": "newobj",
					"text": "maxmcp @alias synth1 @group synths",
					"patching_rect": [20, 130, 230, 22],
					"numinlets": 0,
					"numoutlets": 0,
					"id": "obj-1"
				}
			},
			{
				"box": {
					"maxclass": "comment",
					"text": "↑ Both @alias and @group attributes",
					"patching_rect": [260, 130, 300, 20]
				}
			},
			{
				"box": {
					"maxclass": "comment",
					"text": "This patch belongs to the \"synths\" group",
					"patching_rect": [20, 160, 350, 20]
				}
			},
			{
				"box": {
					"maxclass": "panel",
					"bgcolor": [0.3, 0.4, 0.2, 1.0],
					"patching_rect": [10, 10, 570, 180]
				}
			}
		],
		"lines": []
	}
}
