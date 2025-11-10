{
	"patcher": {
		"fileversion": 1,
		"appversion": {
			"major": 9,
			"minor": 0,
			"revision": 0
		},
		"rect": [100, 100, 950, 800],
		"boxes": [
			{
				"box": {
					"maxclass": "comment",
					"text": "E2E Test: Claude Code MCP Connection",
					"fontsize": 14,
					"fontface": 1,
					"patching_rect": [20, 20, 450, 22]
				}
			},
			{
				"box": {
					"maxclass": "comment",
					"text": "This patch verifies complete end-to-end MCP connection from Claude Code to Max.",
					"patching_rect": [20, 50, 650, 20]
				}
			},
			{
				"box": {
					"maxclass": "comment",
					"text": "PREREQUISITES",
					"fontsize": 12,
					"fontface": 1,
					"textcolor": [1.0, 0.5, 0.0, 1.0],
					"patching_rect": [20, 85, 200, 20]
				}
			},
			{
				"box": {
					"maxclass": "comment",
					"text": "1. Open 00-index.maxpat and start Agent + Bridge",
					"patching_rect": [20, 110, 550, 20]
				}
			},
			{
				"box": {
					"maxclass": "comment",
					"text": "2. Configure Claude Code MCP (see 00-index for command)",
					"patching_rect": [20, 130, 550, 20]
				}
			},
			{
				"box": {
					"maxclass": "comment",
					"text": "3. Restart Claude Code",
					"patching_rect": [20, 150, 550, 20]
				}
			},
			{
				"box": {
					"maxclass": "comment",
					"text": "4. Open test patches: 04-06 (synth1, synth2, fx1)",
					"patching_rect": [20, 170, 550, 20]
				}
			},
			{
				"box": {
					"maxclass": "comment",
					"text": "TEST SCENARIOS",
					"fontsize": 12,
					"fontface": 1,
					"patching_rect": [20, 210, 200, 20]
				}
			},
			{
				"box": {
					"maxclass": "comment",
					"text": "Test 1: List All Patches",
					"fontsize": 11,
					"fontface": 1,
					"patching_rect": [20, 240, 300, 19]
				}
			},
			{
				"box": {
					"maxclass": "comment",
					"text": "Ask Claude: \"List all active Max patches\"",
					"patching_rect": [20, 260, 550, 20]
				}
			},
			{
				"box": {
					"maxclass": "comment",
					"text": "Expected: Claude lists synth1, synth2, fx1, e2e-demo (this patch)",
					"textcolor": [0.0, 0.7, 0.0, 1.0],
					"patching_rect": [20, 280, 550, 20]
				}
			},
			{
				"box": {
					"maxclass": "comment",
					"text": "Test 2: Group Filtering",
					"fontsize": 11,
					"fontface": 1,
					"patching_rect": [20, 315, 300, 19]
				}
			},
			{
				"box": {
					"maxclass": "comment",
					"text": "Ask Claude: \"List only synth patches\"",
					"patching_rect": [20, 335, 550, 20]
				}
			},
			{
				"box": {
					"maxclass": "comment",
					"text": "Expected: Claude lists synth1 and synth2 (group: synths)",
					"textcolor": [0.0, 0.7, 0.0, 1.0],
					"patching_rect": [20, 355, 550, 20]
				}
			},
			{
				"box": {
					"maxclass": "comment",
					"text": "Test 3: Get Console Log",
					"fontsize": 11,
					"fontface": 1,
					"patching_rect": [20, 390, 300, 19]
				}
			},
			{
				"box": {
					"maxclass": "comment",
					"text": "Ask Claude: \"Show me the Max Console log\"",
					"patching_rect": [20, 410, 550, 20]
				}
			},
			{
				"box": {
					"maxclass": "comment",
					"text": "Expected: Claude retrieves recent Max Console messages",
					"textcolor": [0.0, 0.7, 0.0, 1.0],
					"patching_rect": [20, 430, 550, 20]
				}
			},
			{
				"box": {
					"maxclass": "comment",
					"text": "Test 4: Add Max Object",
					"fontsize": 11,
					"fontface": 1,
					"patching_rect": [20, 465, 300, 19]
				}
			},
			{
				"box": {
					"maxclass": "comment",
					"text": "Ask Claude: \"Add a button to this patch at position [150, 150]\"",
					"patching_rect": [20, 485, 550, 20]
				}
			},
			{
				"box": {
					"maxclass": "comment",
					"text": "Expected: Button appears at specified position",
					"textcolor": [0.0, 0.7, 0.0, 1.0],
					"patching_rect": [20, 505, 550, 20]
				}
			},
			{
				"box": {
					"maxclass": "comment",
					"text": "Test 5: Get Objects in Patch",
					"fontsize": 11,
					"fontface": 1,
					"patching_rect": [20, 540, 300, 19]
				}
			},
			{
				"box": {
					"maxclass": "comment",
					"text": "Ask Claude: \"List all objects in this patch\"",
					"patching_rect": [20, 560, 550, 20]
				}
			},
			{
				"box": {
					"maxclass": "comment",
					"text": "Expected: Claude lists maxmcp object and any objects you added",
					"textcolor": [0.0, 0.7, 0.0, 1.0],
					"patching_rect": [20, 580, 550, 20]
				}
			},
			{
				"box": {
					"maxclass": "comment",
					"text": "Test 6: Remove Max Object",
					"fontsize": 11,
					"fontface": 1,
					"patching_rect": [20, 615, 300, 19]
				}
			},
			{
				"box": {
					"maxclass": "comment",
					"text": "Ask Claude: \"Remove the button you just added\"",
					"patching_rect": [20, 635, 550, 20]
				}
			},
			{
				"box": {
					"maxclass": "comment",
					"text": "Expected: Button disappears from patch",
					"textcolor": [0.0, 0.7, 0.0, 1.0],
					"patching_rect": [20, 655, 550, 20]
				}
			},
			{
				"box": {
					"maxclass": "comment",
					"text": "VERIFICATION",
					"fontsize": 12,
					"fontface": 1,
					"patching_rect": [20, 695, 200, 20]
				}
			},
			{
				"box": {
					"maxclass": "comment",
					"text": "✓ All MCP tools working correctly",
					"patching_rect": [20, 720, 300, 20]
				}
			},
			{
				"box": {
					"maxclass": "comment",
					"text": "✓ Bridge communication stable",
					"patching_rect": [20, 740, 300, 20]
				}
			},
			{
				"box": {
					"maxclass": "comment",
					"text": "✓ Patch registry functional",
					"patching_rect": [20, 760, 300, 20]
				}
			},
			{
				"box": {
					"maxclass": "newobj",
					"text": "maxmcp @mode patch @alias e2e-demo @group test",
					"patching_rect": [600, 20, 330, 22],
					"numinlets": 0,
					"numoutlets": 0,
					"id": "obj-maxmcp"
				}
			},
			{
				"box": {
					"maxclass": "comment",
					"text": "← This patch registration",
					"patching_rect": [750, 50, 180, 20]
				}
			},
			{
				"box": {
					"maxclass": "panel",
					"bgcolor": [0.2, 0.3, 0.4, 1.0],
					"patching_rect": [10, 10, 920, 780]
				}
			}
		],
		"lines": []
	}
}
