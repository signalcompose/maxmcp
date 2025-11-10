{
	"patcher": {
		"fileversion": 1,
		"appversion": {
			"major": 9,
			"minor": 0,
			"revision": 0
		},
		"rect": [100, 100, 800, 850],
		"boxes": [
			{
				"box": {
					"maxclass": "comment",
					"text": "MaxMCP - Phase 2 Examples & Tests",
					"fontsize": 18,
					"fontface": 1,
					"patching_rect": [20, 20, 500, 27]
				}
			},
			{
				"box": {
					"maxclass": "comment",
					"text": "Click each button below to open the corresponding test patch.",
					"fontsize": 12,
					"patching_rect": [20, 55, 500, 20]
				}
			},
			{
				"box": {
					"maxclass": "comment",
					"text": "Setup: Install Bridge Dependencies (Required Once)",
					"fontsize": 14,
					"fontface": 1,
					"textcolor": [1.0, 0.7, 0.0, 1.0],
					"patching_rect": [20, 90, 450, 22]
				}
			},
			{
				"box": {
					"maxclass": "comment",
					"text": "Install Node.js dependencies for the WebSocket bridge (npm install).",
					"patching_rect": [20, 115, 500, 20]
				}
			},
			{
				"box": {
					"maxclass": "message",
					"text": "load 00-setup.maxpat",
					"patching_rect": [20, 140, 240, 22],
					"id": "obj-install"
				}
			},
			{
				"box": {
					"maxclass": "comment",
					"text": "Test 1: Claude Code MCP Connection (E2E)",
					"fontsize": 14,
					"fontface": 1,
					"patching_rect": [20, 180, 350, 22]
				}
			},
			{
				"box": {
					"maxclass": "comment",
					"text": "Complete end-to-end test with Claude Code MCP client.",
					"patching_rect": [20, 205, 450, 20]
				}
			},
			{
				"box": {
					"maxclass": "comment",
					"text": "Prerequisites: Test 5-7 patches must be open!",
					"textcolor": [1.0, 0.0, 0.0, 1.0],
					"patching_rect": [20, 225, 350, 20]
				}
			},
			{
				"box": {
					"maxclass": "message",
					"text": "load 01-claude-code-connection.maxpat",
					"patching_rect": [20, 250, 250, 22],
					"id": "obj-test1"
				}
			},
			{
				"box": {
					"maxclass": "comment",
					"text": "Test 2: Basic Client Registration",
					"fontsize": 14,
					"fontface": 1,
					"patching_rect": [20, 285, 300, 22]
				}
			},
			{
				"box": {
					"maxclass": "comment",
					"text": "Learn how maxmcp object auto-registers patches with default ID.",
					"patching_rect": [20, 310, 450, 20]
				}
			},
			{
				"box": {
					"maxclass": "message",
					"text": "load 02-basic-client.maxpat",
					"patching_rect": [20, 335, 180, 22],
					"id": "obj-test2"
				}
			},
			{
				"box": {
					"maxclass": "comment",
					"text": "Test 3: Custom Patch ID (@alias)",
					"fontsize": 14,
					"fontface": 1,
					"patching_rect": [20, 370, 300, 22]
				}
			},
			{
				"box": {
					"maxclass": "comment",
					"text": "Use @alias attribute to set custom patch identifiers.",
					"patching_rect": [20, 395, 450, 20]
				}
			},
			{
				"box": {
					"maxclass": "message",
					"text": "load 03-custom-alias.maxpat",
					"patching_rect": [20, 420, 180, 22],
					"id": "obj-test3"
				}
			},
			{
				"box": {
					"maxclass": "comment",
					"text": "Test 4: Group Assignment (@group)",
					"fontsize": 14,
					"fontface": 1,
					"patching_rect": [20, 455, 300, 22]
				}
			},
			{
				"box": {
					"maxclass": "comment",
					"text": "Assign patches to groups for organized multi-patch control.",
					"patching_rect": [20, 480, 450, 20]
				}
			},
			{
				"box": {
					"maxclass": "message",
					"text": "load 04-group-assignment.maxpat",
					"patching_rect": [20, 505, 210, 22],
					"id": "obj-test4"
				}
			},
			{
				"box": {
					"maxclass": "comment",
					"text": "Test 5-7: Multi-Patch Setup",
					"fontsize": 14,
					"fontface": 1,
					"patching_rect": [20, 540, 300, 22]
				}
			},
			{
				"box": {
					"maxclass": "comment",
					"text": "Open multiple patches with different groups (synths, effects).",
					"patching_rect": [20, 565, 450, 20]
				}
			},
			{
				"box": {
					"maxclass": "message",
					"text": "load 05-multi-patch-synth1.maxpat",
					"patching_rect": [20, 590, 220, 22],
					"id": "obj-test5a"
				}
			},
			{
				"box": {
					"maxclass": "message",
					"text": "load 06-multi-patch-synth2.maxpat",
					"patching_rect": [250, 590, 220, 22],
					"id": "obj-test5b"
				}
			},
			{
				"box": {
					"maxclass": "message",
					"text": "load 07-multi-patch-fx1.maxpat",
					"patching_rect": [480, 590, 200, 22],
					"id": "obj-test5c"
				}
			},
			{
				"box": {
					"maxclass": "comment",
					"text": "Test 8: Server & Group Filter",
					"fontsize": 14,
					"fontface": 1,
					"patching_rect": [20, 625, 300, 22]
				}
			},
			{
				"box": {
					"maxclass": "comment",
					"text": "Test maxmcp.server with group filtering functionality.",
					"patching_rect": [20, 650, 450, 20]
				}
			},
			{
				"box": {
					"maxclass": "comment",
					"text": "Prerequisites: Open Test 5-7 patches (synth1, synth2, fx1) first!",
					"textcolor": [1.0, 0.0, 0.0, 1.0],
					"patching_rect": [20, 670, 450, 20]
				}
			},
			{
				"box": {
					"maxclass": "message",
					"text": "load 08-server-groupfilter.maxpat",
					"patching_rect": [20, 695, 220, 22],
					"id": "obj-test8"
				}
			},
			{
				"box": {
					"maxclass": "comment",
					"text": "Test 9: Bridge Auto-Launch",
					"fontsize": 14,
					"fontface": 1,
					"patching_rect": [20, 730, 300, 22]
				}
			},
			{
				"box": {
					"maxclass": "comment",
					"text": "Test automatic WebSocket bridge launching from Max.",
					"patching_rect": [20, 755, 450, 20]
				}
			},
			{
				"box": {
					"maxclass": "message",
					"text": "load 09-bridge-launch.maxpat",
					"patching_rect": [20, 780, 180, 22],
					"id": "obj-test9"
				}
			},
			{
				"box": {
					"maxclass": "newobj",
					"text": "pcontrol",
					"patching_rect": [20, 820, 60, 22],
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
					"patching_rect": [10, 10, 770, 810]
				}
			}
		],
		"lines": [
			{
				"patchline": {
					"source": ["obj-install", 0],
					"destination": ["obj-pcontrol", 0]
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
					"source": ["obj-test5a", 0],
					"destination": ["obj-pcontrol", 0]
				}
			},
			{
				"patchline": {
					"source": ["obj-test5b", 0],
					"destination": ["obj-pcontrol", 0]
				}
			},
			{
				"patchline": {
					"source": ["obj-test5c", 0],
					"destination": ["obj-pcontrol", 0]
				}
			},
			{
				"patchline": {
					"source": ["obj-test8", 0],
					"destination": ["obj-pcontrol", 0]
				}
			},
			{
				"patchline": {
					"source": ["obj-test9", 0],
					"destination": ["obj-pcontrol", 0]
				}
			}
		]
	}
}
