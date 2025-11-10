{
	"patcher": {
		"fileversion": 1,
		"appversion": {
			"major": 9,
			"minor": 0,
			"revision": 0
		},
		"rect": [100, 100, 750, 500],
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
					"text": "1. Run 00-setup.maxpat to install Node.js dependencies (first time only)",
					"patching_rect": [20, 110, 550, 20]
				}
			},
			{
				"box": {
					"maxclass": "comment",
					"text": "2. Open Test 5-7 patches (synth1, synth2, fx1) for patch registry",
					"patching_rect": [20, 130, 450, 20]
				}
			},
			{
				"box": {
					"maxclass": "comment",
					"text": "3. Configure Claude Code MCP settings (see below)",
					"patching_rect": [20, 150, 450, 20]
				}
			},
			{
				"box": {
					"maxclass": "comment",
					"text": "STEP 1: Start Server & Bridge",
					"fontsize": 12,
					"fontface": 1,
					"patching_rect": [20, 185, 300, 20]
				}
			},
			{
				"box": {
					"maxclass": "message",
					"text": "start",
					"patching_rect": [20, 210, 40, 22],
					"id": "obj-start-button",
					"numinlets": 2,
					"numoutlets": 1,
					"outlettype": [""]
				}
			},
			{
				"box": {
					"maxclass": "comment",
					"text": "← Click to start (agent → bridge auto-start)",
					"patching_rect": [70, 215, 280, 20]
				}
			},
			{
				"box": {
					"maxclass": "message",
					"text": "stop",
					"patching_rect": [20, 240, 40, 22],
					"id": "obj-stop-button",
					"numinlets": 2,
					"numoutlets": 1,
					"outlettype": [""]
				}
			},
			{
				"box": {
					"maxclass": "comment",
					"text": "← Click to stop",
					"patching_rect": [70, 245, 100, 20]
				}
			},
			{
				"box": {
					"maxclass": "newobj",
					"text": "maxmcp.agent",
					"patching_rect": [20, 280, 100, 22],
					"numinlets": 1,
					"numoutlets": 1,
					"outlettype": [""],
					"id": "obj-agent"
				}
			},
			{
				"box": {
					"maxclass": "newobj",
					"text": "maxmcp-bridge",
					"patching_rect": [20, 320, 100, 22],
					"numinlets": 1,
					"numoutlets": 1,
					"outlettype": [""],
					"id": "obj-bridge"
				}
			},
			{
				"box": {
					"maxclass": "comment",
					"text": "Output is shown in Max Console (Cmd+B)",
					"patching_rect": [20, 360, 300, 20]
				}
			},
			{
				"box": {
					"maxclass": "comment",
					"text": "STEP 2: Configure Claude Code",
					"fontsize": 12,
					"fontface": 1,
					"patching_rect": [20, 390, 300, 20]
				}
			},
			{
				"box": {
					"maxclass": "comment",
					"text": "Run this command in your terminal:",
					"patching_rect": [20, 415, 350, 20]
				}
			},
			{
				"box": {
					"maxclass": "textedit",
					"text": "claude mcp add maxmcp node ~/Documents/Max\\ 9/Packages/MaxMCP/support/bridge/websocket-mcp-bridge.js ws://localhost:7400",
					"textcolor": [0.0, 0.5, 1.0, 1.0],
					"bgcolor": [0.15, 0.15, 0.15, 1.0],
					"fontname": "Monaco",
					"fontsize": 10,
					"readonly": 1,
					"wordwrap": 0,
					"patching_rect": [35, 440, 680, 22],
					"id": "obj-command"
				}
			},
			{
				"box": {
					"maxclass": "comment",
					"text": "← Click to select, then Cmd+C to copy",
					"textcolor": [0.7, 0.7, 0.7, 1.0],
					"fontsize": 10,
					"patching_rect": [420, 442, 250, 18]
				}
			},
			{
				"box": {
					"maxclass": "comment",
					"text": "(Bridge uses stdio for Claude Code ↔ WebSocket for maxmcp.agent)",
					"textcolor": [0.5, 0.5, 0.5, 1.0],
					"fontsize": 10,
					"patching_rect": [35, 467, 500, 18]
				}
			},
			{
				"box": {
					"maxclass": "comment",
					"text": "STEP 3: Test in Claude Code",
					"fontsize": 12,
					"fontface": 1,
					"patching_rect": [20, 502, 300, 20]
				}
			},
			{
				"box": {
					"maxclass": "comment",
					"text": "1. Restart Claude Code to load new MCP server",
					"patching_rect": [20, 525, 350, 20]
				}
			},
			{
				"box": {
					"maxclass": "comment",
					"text": "2. Try: \"List all active Max patches\"",
					"patching_rect": [20, 545, 300, 20]
				}
			},
			{
				"box": {
					"maxclass": "comment",
					"text": "3. Expected: Claude lists synth1, synth2, fx1 patches",
					"patching_rect": [20, 565, 400, 20]
				}
			},
			{
				"box": {
					"maxclass": "comment",
					"text": "4. Try: \"List only synth patches\"",
					"patching_rect": [20, 585, 300, 20]
				}
			},
			{
				"box": {
					"maxclass": "comment",
					"text": "5. Expected: Claude lists only synth1 and synth2 (group filter)",
					"patching_rect": [20, 605, 450, 20]
				}
			},
			{
				"box": {
					"maxclass": "panel",
					"bgcolor": [0.2, 0.3, 0.4, 1.0],
					"patching_rect": [10, 10, 720, 530]
				}
			}
		],
		"lines": [
			{
				"patchline": {
					"source": ["obj-agent", 0],
					"destination": ["obj-bridge", 0]
				}
			},
			{
				"patchline": {
					"source": ["obj-stop-button", 0],
					"destination": ["obj-bridge", 0]
				}
			}
		]
	}
}
