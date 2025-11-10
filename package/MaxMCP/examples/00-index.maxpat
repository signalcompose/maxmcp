{
	"patcher": {
		"fileversion": 1,
		"appversion": {
			"major": 9,
			"minor": 0,
			"revision": 0
		},
		"rect": [100, 100, 900, 1000],
		"boxes": [
			{
				"box": {
					"maxclass": "comment",
					"text": "MaxMCP - Phase 2 Examples & E2E Tests",
					"fontsize": 18,
					"fontface": 1,
					"patching_rect": [20, 20, 500, 27]
				}
			},
			{
				"box": {
					"maxclass": "comment",
					"text": "This patch demonstrates MaxMCP Agent setup and provides links to test patches.",
					"fontsize": 12,
					"patching_rect": [20, 55, 600, 20]
				}
			},
			{
				"box": {
					"maxclass": "comment",
					"text": "STEP 1: Install Node.js Dependencies (First Time Only)",
					"fontsize": 14,
					"fontface": 1,
					"textcolor": [1.0, 0.7, 0.0, 1.0],
					"patching_rect": [20, 95, 500, 22]
				}
			},
			{
				"box": {
					"maxclass": "comment",
					"text": "Click the button below to install bridge dependencies (npm install).",
					"patching_rect": [20, 125, 500, 20]
				}
			},
			{
				"box": {
					"maxclass": "button",
					"patching_rect": [20, 150, 40, 40],
					"id": "obj-install-button"
				}
			},
			{
				"box": {
					"maxclass": "comment",
					"text": "← Click to install",
					"patching_rect": [70, 160, 150, 20]
				}
			},
			{
				"box": {
					"maxclass": "newobj",
					"text": "node.script ~/Documents/Max\\ 9/Packages/MaxMCP/support/bridge/npm-install.js @autostart 1 @watch 0",
					"patching_rect": [20, 200, 650, 22],
					"numinlets": 1,
					"numoutlets": 2,
					"outlettype": ["", ""],
					"id": "obj-npm-install"
				}
			},
			{
				"box": {
					"maxclass": "comment",
					"text": "STEP 2: Start MaxMCP Agent",
					"fontsize": 14,
					"fontface": 1,
					"patching_rect": [20, 245, 400, 22]
				}
			},
			{
				"box": {
					"maxclass": "comment",
					"text": "Agent: WebSocket MCP server that manages patch registry and tools",
					"patching_rect": [20, 275, 550, 20]
				}
			},
			{
				"box": {
					"maxclass": "comment",
					"text": "Bridge runs on Claude Code side (websocket-mcp-bridge.js)",
					"patching_rect": [20, 295, 500, 20]
				}
			},
			{
				"box": {
					"maxclass": "message",
					"text": "start",
					"patching_rect": [20, 325, 40, 22],
					"id": "obj-start-button"
				}
			},
			{
				"box": {
					"maxclass": "comment",
					"text": "← Click to start Agent",
					"patching_rect": [70, 330, 200, 20]
				}
			},
			{
				"box": {
					"maxclass": "message",
					"text": "stop",
					"patching_rect": [20, 355, 40, 22],
					"id": "obj-stop-button"
				}
			},
			{
				"box": {
					"maxclass": "comment",
					"text": "← Click to stop",
					"patching_rect": [70, 360, 150, 20]
				}
			},
			{
				"box": {
					"maxclass": "newobj",
					"text": "maxmcp @mode agent",
					"patching_rect": [20, 395, 130, 22],
					"numinlets": 1,
					"numoutlets": 1,
					"outlettype": [""],
					"id": "obj-agent"
				}
			},
			{
				"box": {
					"maxclass": "comment",
					"text": "← MaxMCP Agent (manages registry + MCP tools)",
					"patching_rect": [160, 400, 350, 20]
				}
			},
			{
				"box": {
					"maxclass": "comment",
					"text": "STEP 3: Configure Claude Code MCP",
					"fontsize": 14,
					"fontface": 1,
					"patching_rect": [20, 475, 400, 22]
				}
			},
			{
				"box": {
					"maxclass": "comment",
					"text": "Run this command in your terminal:",
					"patching_rect": [20, 505, 350, 20]
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
					"patching_rect": [35, 530, 800, 22],
					"id": "obj-mcp-command"
				}
			},
			{
				"box": {
					"maxclass": "comment",
					"text": "← Click to select, then Cmd+C to copy",
					"textcolor": [0.7, 0.7, 0.7, 1.0],
					"fontsize": 10,
					"patching_rect": [440, 532, 250, 18]
				}
			},
			{
				"box": {
					"maxclass": "comment",
					"text": "After adding MCP server, restart Claude Code.",
					"patching_rect": [35, 560, 350, 20]
				}
			},
			{
				"box": {
					"maxclass": "comment",
					"text": "STEP 4: Open Test Patches",
					"fontsize": 14,
					"fontface": 1,
					"patching_rect": [20, 600, 400, 22]
				}
			},
			{
				"box": {
					"maxclass": "comment",
					"text": "Click buttons below to open test patches. Each patch will auto-register with the Agent.",
					"patching_rect": [20, 630, 650, 20]
				}
			},
			{
				"box": {
					"maxclass": "comment",
					"text": "Test 1: Basic Registration (no @alias, no @group)",
					"fontsize": 12,
					"fontface": 1,
					"patching_rect": [20, 665, 400, 20]
				}
			},
			{
				"box": {
					"maxclass": "comment",
					"text": "Auto-generated patch ID: {patchname}_{8-char-uuid}",
					"patching_rect": [20, 685, 400, 20]
				}
			},
			{
				"box": {
					"maxclass": "message",
					"text": "load 01-basic-registration.maxpat",
					"patching_rect": [20, 710, 220, 22],
					"id": "obj-test1"
				}
			},
			{
				"box": {
					"maxclass": "comment",
					"text": "Test 2: Custom Alias (@alias only)",
					"fontsize": 12,
					"fontface": 1,
					"patching_rect": [20, 745, 400, 20]
				}
			},
			{
				"box": {
					"maxclass": "comment",
					"text": "Custom patch ID: my_synth (no UUID suffix)",
					"patching_rect": [20, 765, 400, 20]
				}
			},
			{
				"box": {
					"maxclass": "message",
					"text": "load 02-custom-alias.maxpat",
					"patching_rect": [20, 790, 200, 22],
					"id": "obj-test2"
				}
			},
			{
				"box": {
					"maxclass": "comment",
					"text": "Test 3: Group Assignment (@alias + @group)",
					"fontsize": 12,
					"fontface": 1,
					"patching_rect": [20, 825, 400, 20]
				}
			},
			{
				"box": {
					"maxclass": "comment",
					"text": "Patch ID: synth1, Group: synths",
					"patching_rect": [20, 845, 400, 20]
				}
			},
			{
				"box": {
					"maxclass": "message",
					"text": "load 03-group-assignment.maxpat",
					"patching_rect": [20, 870, 220, 22],
					"id": "obj-test3"
				}
			},
			{
				"box": {
					"maxclass": "comment",
					"text": "Test 4-6: Multi-Patch Registration",
					"fontsize": 12,
					"fontface": 1,
					"patching_rect": [20, 905, 400, 20]
				}
			},
			{
				"box": {
					"maxclass": "comment",
					"text": "Open all three patches to test multi-patch registry and group filtering.",
					"patching_rect": [20, 925, 550, 20]
				}
			},
			{
				"box": {
					"maxclass": "message",
					"text": "load 04-multi-patch-synth1.maxpat",
					"patching_rect": [20, 950, 220, 22],
					"id": "obj-test4"
				}
			},
			{
				"box": {
					"maxclass": "message",
					"text": "load 05-multi-patch-synth2.maxpat",
					"patching_rect": [250, 950, 220, 22],
					"id": "obj-test5"
				}
			},
			{
				"box": {
					"maxclass": "message",
					"text": "load 06-multi-patch-fx1.maxpat",
					"patching_rect": [480, 950, 200, 22],
					"id": "obj-test6"
				}
			},
			{
				"box": {
					"maxclass": "comment",
					"text": "Test 7: MCP Tools Test",
					"fontsize": 12,
					"fontface": 1,
					"patching_rect": [20, 985, 400, 20]
				}
			},
			{
				"box": {
					"maxclass": "comment",
					"text": "Target patch for Claude Code to manipulate using MCP tools.",
					"patching_rect": [20, 1005, 500, 20]
				}
			},
			{
				"box": {
					"maxclass": "message",
					"text": "load 07-mcp-tools-test.maxpat",
					"patching_rect": [20, 1030, 200, 22],
					"id": "obj-test7"
				}
			},
			{
				"box": {
					"maxclass": "comment",
					"text": "E2E Test: Complete Claude Code Integration",
					"fontsize": 12,
					"fontface": 1,
					"patching_rect": [20, 1065, 400, 20]
				}
			},
			{
				"box": {
					"maxclass": "comment",
					"text": "Full end-to-end test with Claude Code MCP client.",
					"patching_rect": [20, 1085, 500, 20]
				}
			},
			{
				"box": {
					"maxclass": "message",
					"text": "load 01-claude-code-connection.maxpat",
					"patching_rect": [20, 1110, 250, 22],
					"id": "obj-e2e"
				}
			},
			{
				"box": {
					"maxclass": "newobj",
					"text": "pcontrol",
					"patching_rect": [20, 1150, 60, 22],
					"numinlets": 1,
					"numoutlets": 1,
					"outlettype": [""],
					"id": "obj-pcontrol"
				}
			},
			{
				"box": {
					"maxclass": "panel",
					"bgcolor": [0.2, 0.2, 0.3, 1.0],
					"patching_rect": [10, 10, 870, 1170]
				}
			}
		],
		"lines": [
			{
				"patchline": {
					"source": ["obj-install-button", 0],
					"destination": ["obj-npm-install", 0]
				}
			},
			{
				"patchline": {
					"source": ["obj-start-button", 0],
					"destination": ["obj-agent", 0]
				}
			},
			{
				"patchline": {
					"source": ["obj-stop-button", 0],
					"destination": ["obj-agent", 0]
				}
			},
			{
				"patchline": {
					"source": ["obj-test1", 0],
					"destination": ["obj-pcontrol", 0]
				}
			},
			{
				"patchline": {
					"source": ["obj-test2", 0],
					"destination": ["obj-pcontrol", 0]
				}
			},
			{
				"patchline": {
					"source": ["obj-test3", 0],
					"destination": ["obj-pcontrol", 0]
				}
			},
			{
				"patchline": {
					"source": ["obj-test4", 0],
					"destination": ["obj-pcontrol", 0]
				}
			},
			{
				"patchline": {
					"source": ["obj-test5", 0],
					"destination": ["obj-pcontrol", 0]
				}
			},
			{
				"patchline": {
					"source": ["obj-test6", 0],
					"destination": ["obj-pcontrol", 0]
				}
			},
			{
				"patchline": {
					"source": ["obj-test7", 0],
					"destination": ["obj-pcontrol", 0]
				}
			},
			{
				"patchline": {
					"source": ["obj-e2e", 0],
					"destination": ["obj-pcontrol", 0]
				}
			}
		]
	}
}
