{
	"patcher": {
		"fileversion": 1,
		"appversion": {
			"major": 9,
			"minor": 0,
			"revision": 0
		},
		"rect": [100, 100, 700, 600],
		"boxes": [
			{
				"box": {
					"maxclass": "comment",
					"text": "Phase 2 - Test 5-9: Server & Group Filter",
					"fontsize": 14,
					"patching_rect": [20, 20, 400, 22]
				}
			},
			{
				"box": {
					"maxclass": "comment",
					"text": "Prerequisites: Open Test 4 patches (synth1, synth2, fx1) first!",
					"patching_rect": [20, 50, 450, 20],
					"textcolor": [1.0, 0.0, 0.0, 1.0]
				}
			},
			{
				"box": {
					"maxclass": "newobj",
					"text": "maxmcp.server @transport stdio",
					"patching_rect": [20, 90, 200, 22],
					"numinlets": 0,
					"numoutlets": 0,
					"id": "obj-server"
				}
			},
			{
				"box": {
					"maxclass": "comment",
					"text": "↑ MCP Server (stdio mode for testing)",
					"patching_rect": [230, 90, 300, 20]
				}
			},
			{
				"box": {
					"maxclass": "comment",
					"text": "Test 6: List all patches (no filter)",
					"fontsize": 12,
					"patching_rect": [20, 140, 300, 20]
				}
			},
			{
				"box": {
					"maxclass": "message",
					"text": "{\"jsonrpc\":\"2.0\",\"id\":1,\"method\":\"tools/call\",\"params\":{\"name\":\"list_active_patches\",\"arguments\":{}}}",
					"patching_rect": [20, 165, 650, 22],
					"id": "obj-test6"
				}
			},
			{
				"box": {
					"maxclass": "comment",
					"text": "Expected: All 3 patches (synth1, synth2, fx1) with group fields",
					"patching_rect": [20, 190, 450, 20]
				}
			},
			{
				"box": {
					"maxclass": "comment",
					"text": "Test 7: Filter by \"synths\" group",
					"fontsize": 12,
					"patching_rect": [20, 230, 300, 20]
				}
			},
			{
				"box": {
					"maxclass": "message",
					"text": "{\"jsonrpc\":\"2.0\",\"id\":2,\"method\":\"tools/call\",\"params\":{\"name\":\"list_active_patches\",\"arguments\":{\"group\":\"synths\"}}}",
					"patching_rect": [20, 255, 650, 22],
					"id": "obj-test7"
				}
			},
			{
				"box": {
					"maxclass": "comment",
					"text": "Expected: Only synth1 and synth2, with filter: {group: \"synths\"}",
					"patching_rect": [20, 280, 450, 20]
				}
			},
			{
				"box": {
					"maxclass": "comment",
					"text": "Test 8: Filter by \"effects\" group",
					"fontsize": 12,
					"patching_rect": [20, 320, 300, 20]
				}
			},
			{
				"box": {
					"maxclass": "message",
					"text": "{\"jsonrpc\":\"2.0\",\"id\":3,\"method\":\"tools/call\",\"params\":{\"name\":\"list_active_patches\",\"arguments\":{\"group\":\"effects\"}}}",
					"patching_rect": [20, 345, 650, 22],
					"id": "obj-test8"
				}
			},
			{
				"box": {
					"maxclass": "comment",
					"text": "Expected: Only fx1, with filter: {group: \"effects\"}",
					"patching_rect": [20, 370, 450, 20]
				}
			},
			{
				"box": {
					"maxclass": "comment",
					"text": "Test 9: Filter by non-existent group",
					"fontsize": 12,
					"patching_rect": [20, 410, 300, 20]
				}
			},
			{
				"box": {
					"maxclass": "message",
					"text": "{\"jsonrpc\":\"2.0\",\"id\":4,\"method\":\"tools/call\",\"params\":{\"name\":\"list_active_patches\",\"arguments\":{\"group\":\"nonexistent\"}}}",
					"patching_rect": [20, 435, 650, 22],
					"id": "obj-test9"
				}
			},
			{
				"box": {
					"maxclass": "comment",
					"text": "Expected: Empty array, count: 0, filter: {group: \"nonexistent\"}",
					"patching_rect": [20, 460, 450, 20]
				}
			},
			{
				"box": {
					"maxclass": "comment",
					"text": "NOTE: Click each message box to send test request to server",
					"patching_rect": [20, 500, 450, 20],
					"textcolor": [0.0, 0.0, 1.0, 1.0]
				}
			},
			{
				"box": {
					"maxclass": "comment",
					"text": "Check Max Console for responses",
					"patching_rect": [20, 520, 300, 20],
					"textcolor": [0.0, 0.0, 1.0, 1.0]
				}
			},
			{
				"box": {
					"maxclass": "panel",
					"bgcolor": [0.3, 0.3, 0.4, 1.0],
					"patching_rect": [10, 10, 670, 540]
				}
			}
		],
		"lines": []
	}
}
