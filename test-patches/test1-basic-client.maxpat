{
	"patcher": {
		"fileversion": 1,
		"appversion": {
			"major": 9,
			"minor": 0,
			"revision": 0
		},
		"boxes": [
			{
				"box": {
					"maxclass": "newobj",
					"text": "maxmcp",
					"patching_rect": [100, 100, 60, 22],
					"numinlets": 0,
					"numoutlets": 0
				}
			},
			{
				"box": {
					"maxclass": "comment",
					"text": "Test 1: Basic Client Registration",
					"patching_rect": [100, 50, 250, 20]
				}
			},
			{
				"box": {
					"maxclass": "comment",
					"text": "Expected: Console shows patch ID like 'test1-basic-client_xxxxxxxx'",
					"patching_rect": [100, 150, 400, 20]
				}
			}
		]
	}
}
