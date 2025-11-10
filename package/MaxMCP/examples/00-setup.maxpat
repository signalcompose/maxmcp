{
	"patcher": {
		"fileversion": 1,
		"appversion": {
			"major": 9,
			"minor": 0,
			"revision": 0
		},
		"rect": [100, 100, 600, 300],
		"boxes": [
			{
				"box": {
					"maxclass": "comment",
					"text": "MaxMCP Setup - Install Node.js Dependencies",
					"fontsize": 14,
					"fontface": 1,
					"patching_rect": [20, 20, 400, 22]
				}
			},
			{
				"box": {
					"maxclass": "comment",
					"text": "This patch installs the required Node.js dependencies for the MaxMCP bridge.",
					"patching_rect": [20, 50, 600, 20]
				}
			},
			{
				"box": {
					"maxclass": "comment",
					"text": "STEP 1: Install Dependencies",
					"fontsize": 12,
					"fontface": 1,
					"patching_rect": [20, 90, 300, 20]
				}
			},
			{
				"box": {
					"maxclass": "button",
					"patching_rect": [20, 120, 40, 40],
					"id": "obj-install-button"
				}
			},
			{
				"box": {
					"maxclass": "comment",
					"text": "← Click to install (first time only)",
					"patching_rect": [70, 130, 250, 20]
				}
			},
			{
				"box": {
					"maxclass": "newobj",
					"text": "node.script ~/Documents/Max\\ 9/Packages/MaxMCP/support/bridge/npm-install.js @autostart 1 @watch 0",
					"patching_rect": [20, 170, 650, 22],
					"numinlets": 1,
					"numoutlets": 2,
					"outlettype": ["", ""],
					"id": "obj-node-script"
				}
			},
			{
				"box": {
					"maxclass": "comment",
					"text": "Output is shown in Max Console (Cmd+B)",
					"patching_rect": [20, 210, 300, 20]
				}
			},
			{
				"box": {
					"maxclass": "comment",
					"text": "STEP 2: After Installation",
					"fontsize": 12,
					"fontface": 1,
					"textcolor": [1.0, 0.7, 0.0, 1.0],
					"patching_rect": [20, 240, 300, 20]
				}
			},
			{
				"box": {
					"maxclass": "comment",
					"text": "Once installation completes successfully:",
					"patching_rect": [20, 265, 300, 20]
				}
			},
			{
				"box": {
					"maxclass": "comment",
					"text": "1. Use [maxmcp-bridge] abstraction in your patches",
					"patching_rect": [20, 285, 400, 20]
				}
			},
			{
				"box": {
					"maxclass": "comment",
					"text": "2. See 01-claude-code-connection.maxpat for usage examples",
					"patching_rect": [20, 305, 450, 20]
				}
			},
			{
				"box": {
					"maxclass": "panel",
					"bgcolor": [0.2, 0.3, 0.4, 1.0],
					"patching_rect": [10, 10, 570, 270]
				}
			}
		],
		"lines": [
			{
				"patchline": {
					"source": ["obj-install-button", 0],
					"destination": ["obj-node-script", 0]
				}
			}
		]
	}
}
