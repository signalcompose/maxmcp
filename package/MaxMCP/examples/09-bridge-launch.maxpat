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
					"text": "Phase 2 - Test 10: Bridge Auto-Launch",
					"fontsize": 14,
					"fontface": 1,
					"patching_rect": [20, 20, 400, 22]
				}
			},
			{
				"box": {
					"maxclass": "comment",
					"text": "This test demonstrates automatic WebSocket bridge launching.",
					"patching_rect": [20, 50, 500, 20]
				}
			},
			{
				"box": {
					"maxclass": "comment",
					"text": "Step 1: Click loadbang to start bridge",
					"fontsize": 12,
					"fontface": 1,
					"patching_rect": [20, 90, 300, 20]
				}
			},
			{
				"box": {
					"maxclass": "button",
					"patching_rect": [20, 115, 24, 24],
					"id": "obj-loadbang"
				}
			},
			{
				"box": {
					"maxclass": "message",
					"text": "start_bridge 18081",
					"patching_rect": [20, 145, 120, 22],
					"id": "obj-start"
				}
			},
			{
				"box": {
					"maxclass": "comment",
					"text": "Step 2: Server will auto-start bridge on port 18081",
					"patching_rect": [150, 115, 350, 20]
				}
			},
			{
				"box": {
					"maxclass": "comment",
					"text": "Step 3: Check Max Console for confirmation",
					"fontsize": 12,
					"fontface": 1,
					"patching_rect": [20, 185, 350, 20]
				}
			},
			{
				"box": {
					"maxclass": "comment",
					"text": "Expected Console Output:",
					"patching_rect": [20, 210, 200, 20]
				}
			},
			{
				"box": {
					"maxclass": "comment",
					"text": "• \"Bridge started on port 18081 (PID: XXXX)\"",
					"patching_rect": [20, 230, 400, 20]
				}
			},
			{
				"box": {
					"maxclass": "comment",
					"text": "• \"maxmcp-bridge started on port 18081 (PID: XXXX)\"",
					"patching_rect": [20, 250, 400, 20]
				}
			},
			{
				"box": {
					"maxclass": "comment",
					"text": "Step 4: Verify bridge process is running",
					"fontsize": 12,
					"fontface": 1,
					"patching_rect": [20, 290, 350, 20]
				}
			},
			{
				"box": {
					"maxclass": "comment",
					"text": "Run in terminal: lsof -i :18081",
					"textcolor": [0.0, 0.5, 1.0, 1.0],
					"patching_rect": [20, 315, 300, 20]
				}
			},
			{
				"box": {
					"maxclass": "comment",
					"text": "You should see 'maxmcp-br' process listening on port 18081",
					"patching_rect": [20, 335, 450, 20]
				}
			},
			{
				"box": {
					"maxclass": "comment",
					"text": "Step 5: Test manual stop (optional)",
					"fontsize": 12,
					"fontface": 1,
					"patching_rect": [20, 375, 350, 20]
				}
			},
			{
				"box": {
					"maxclass": "message",
					"text": "stop_bridge",
					"patching_rect": [20, 400, 80, 22],
					"id": "obj-stop"
				}
			},
			{
				"box": {
					"maxclass": "comment",
					"text": "Stops the bridge process gracefully",
					"patching_rect": [110, 400, 250, 20]
				}
			},
			{
				"box": {
					"maxclass": "newobj",
					"text": "maxmcp.server @transport stdio @bridge_port 18081",
					"patching_rect": [20, 450, 320, 22],
					"numinlets": 1,
					"numoutlets": 1,
					"outlettype": [""],
					"id": "obj-server"
				}
			},
			{
				"box": {
					"maxclass": "comment",
					"text": "↑ Server with bridge port 18081 (default)",
					"patching_rect": [350, 450, 300, 20]
				}
			},
			{
				"box": {
					"maxclass": "comment",
					"text": "Note: Bridge will auto-stop when patch is closed",
					"textcolor": [0.0, 0.7, 0.0, 1.0],
					"patching_rect": [20, 490, 400, 20]
				}
			},
			{
				"box": {
					"maxclass": "comment",
					"text": "Claude Code Configuration:",
					"fontsize": 12,
					"fontface": 1,
					"patching_rect": [20, 530, 250, 20]
				}
			},
			{
				"box": {
					"maxclass": "comment",
					"text": "Add to ~/.config/claude-code/mcp_settings.json:",
					"patching_rect": [20, 555, 400, 20]
				}
			},
			{
				"box": {
					"maxclass": "comment",
					"text": "{\"mcpServers\": {\"maxmcp\": {\"transport\": \"websocket\", \"url\": \"ws://localhost:18081\"}}}",
					"textcolor": [0.3, 0.3, 0.3, 1.0],
					"fontname": "Monaco",
					"fontsize": 10,
					"patching_rect": [20, 575, 650, 18]
				}
			},
			{
				"box": {
					"maxclass": "panel",
					"bgcolor": [0.25, 0.35, 0.45, 1.0],
					"patching_rect": [10, 10, 670, 590]
				}
			}
		],
		"lines": [
			{
				"patchline": {
					"source": ["obj-loadbang", 0],
					"destination": ["obj-start", 0]
				}
			},
			{
				"patchline": {
					"source": ["obj-start", 0],
					"destination": ["obj-server", 0]
				}
			},
			{
				"patchline": {
					"source": ["obj-stop", 0],
					"destination": ["obj-server", 0]
				}
			}
		]
	}
}
