{
	"patcher": {
		"fileversion": 1,
		"appversion": {
			"major": 9,
			"minor": 0,
			"revision": 0
		},
		"rect": [100, 100, 750, 650],
		"boxes": [
			{
				"box": {
					"maxclass": "comment",
					"text": "Phase 2 - Test 1: Claude Code MCP Connection (E2E)",
					"fontsize": 14,
					"fontface": 1,
					"patching_rect": [20, 20, 450, 22]
				}
			},
			{
				"box": {
					"maxclass": "comment",
					"text": "This test verifies end-to-end MCP connection from Claude Code to Max.",
					"patching_rect": [20, 50, 550, 20]
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
					"text": "1. Open Test 5-7 patches (synth1, synth2, fx1) for patch registry",
					"patching_rect": [20, 110, 450, 20]
				}
			},
			{
				"box": {
					"maxclass": "comment",
					"text": "2. Configure Claude Code MCP settings (see below)",
					"patching_rect": [20, 130, 450, 20]
				}
			},
			{
				"box": {
					"maxclass": "comment",
					"text": "STEP 1: Start Bridge & Server",
					"fontsize": 12,
					"fontface": 1,
					"patching_rect": [20, 165, 300, 20]
				}
			},
			{
				"box": {
					"maxclass": "button",
					"patching_rect": [20, 190, 30, 30],
					"id": "obj-loadbang"
				}
			},
			{
				"box": {
					"maxclass": "message",
					"text": "start_bridge 18081",
					"patching_rect": [20, 230, 120, 22],
					"id": "obj-start-bridge"
				}
			},
			{
				"box": {
					"maxclass": "comment",
					"text": "← Click to start bridge on port 18081",
					"patching_rect": [150, 190, 250, 20]
				}
			},
			{
				"box": {
					"maxclass": "newobj",
					"text": "maxmcp.server @bridge_port 18081",
					"patching_rect": [20, 270, 250, 22],
					"numinlets": 1,
					"numoutlets": 1,
					"outlettype": [""],
					"id": "obj-server"
				}
			},
			{
				"box": {
					"maxclass": "comment",
					"text": "STEP 2: Configure Claude Code",
					"fontsize": 12,
					"fontface": 1,
					"patching_rect": [20, 310, 300, 20]
				}
			},
			{
				"box": {
					"maxclass": "comment",
					"text": "Run this command in your terminal:",
					"patching_rect": [20, 335, 350, 20]
				}
			},
			{
				"box": {
					"maxclass": "textedit",
					"text": "claude mcp add maxmcp ws://localhost:18081",
					"textcolor": [0.0, 0.5, 1.0, 1.0],
					"bgcolor": [0.15, 0.15, 0.15, 1.0],
					"fontname": "Monaco",
					"fontsize": 11,
					"readonly": 1,
					"wordwrap": 0,
					"patching_rect": [35, 360, 380, 22],
					"id": "obj-command"
				}
			},
			{
				"box": {
					"maxclass": "comment",
					"text": "← Click to select, then Cmd+C to copy",
					"textcolor": [0.7, 0.7, 0.7, 1.0],
					"fontsize": 10,
					"patching_rect": [420, 362, 250, 18]
				}
			},
			{
				"box": {
					"maxclass": "comment",
					"text": "(This configures MaxMCP WebSocket server at port 18081)",
					"textcolor": [0.5, 0.5, 0.5, 1.0],
					"fontsize": 10,
					"patching_rect": [35, 387, 400, 18]
				}
			},
			{
				"box": {
					"maxclass": "comment",
					"text": "STEP 3: Test in Claude Code",
					"fontsize": 12,
					"fontface": 1,
					"patching_rect": [20, 422, 300, 20]
				}
			},
			{
				"box": {
					"maxclass": "comment",
					"text": "1. Open Claude Code (this session!)",
					"patching_rect": [20, 445, 250, 20]
				}
			},
			{
				"box": {
					"maxclass": "comment",
					"text": "2. Try: \"List all active Max patches\"",
					"patching_rect": [20, 465, 300, 20]
				}
			},
			{
				"box": {
					"maxclass": "comment",
					"text": "3. Expected: Claude lists synth1, synth2, fx1 patches",
					"patching_rect": [20, 485, 400, 20]
				}
			},
			{
				"box": {
					"maxclass": "comment",
					"text": "4. Try: \"List only synth patches\"",
					"patching_rect": [20, 505, 300, 20]
				}
			},
			{
				"box": {
					"maxclass": "comment",
					"text": "5. Expected: Claude lists only synth1 and synth2 (group filter)",
					"patching_rect": [20, 525, 450, 20]
				}
			},
			{
				"box": {
					"maxclass": "panel",
					"bgcolor": [0.2, 0.3, 0.4, 1.0],
					"patching_rect": [10, 10, 720, 630]
				}
			}
		],
		"lines": [
			{
				"patchline": {
					"source": ["obj-loadbang", 0],
					"destination": ["obj-start-bridge", 0]
				}
			},
			{
				"patchline": {
					"source": ["obj-start-bridge", 0],
					"destination": ["obj-server", 0]
				}
			}
		]
	}
}
