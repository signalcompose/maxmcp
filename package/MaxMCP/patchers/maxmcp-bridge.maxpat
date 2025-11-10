{
	"patcher": {
		"fileversion": 1,
		"appversion": {
			"major": 9,
			"minor": 0,
			"revision": 0
		},
		"rect": [0, 0, 640, 480],
		"boxes": [
			{
				"box": {
					"maxclass": "inlet",
					"patching_rect": [20, 20, 30, 30],
					"numinlets": 0,
					"numoutlets": 1,
					"outlettype": [""],
					"id": "obj-inlet",
					"comment": "'start' to start, 'stop' to stop"
				}
			},
			{
				"box": {
					"maxclass": "newobj",
					"text": "route start stop bang",
					"patching_rect": [20, 70, 140, 22],
					"numinlets": 1,
					"numoutlets": 4,
					"outlettype": ["", "", "", ""],
					"id": "obj-route"
				}
			},
			{
				"box": {
					"maxclass": "message",
					"text": "start",
					"patching_rect": [20, 110, 40, 22],
					"numinlets": 2,
					"numoutlets": 1,
					"outlettype": [""],
					"id": "obj-start-msg"
				}
			},
			{
				"box": {
					"maxclass": "message",
					"text": "stop",
					"patching_rect": [80, 110, 40, 22],
					"numinlets": 2,
					"numoutlets": 1,
					"outlettype": [""],
					"id": "obj-stop-msg"
				}
			},
			{
				"box": {
					"maxclass": "newobj",
					"text": "node.script ~/Documents/Max\\ 9/Packages/MaxMCP/support/bridge/bridge-launcher.js @autostart 1 @watch 0",
					"patching_rect": [20, 150, 700, 22],
					"numinlets": 1,
					"numoutlets": 2,
					"outlettype": ["", ""],
					"id": "obj-node-script"
				}
			}
		],
		"lines": [
			{
				"patchline": {
					"source": ["obj-inlet", 0],
					"destination": ["obj-route", 0]
				}
			},
			{
				"patchline": {
					"source": ["obj-route", 0],
					"destination": ["obj-start-msg", 0]
				}
			},
			{
				"patchline": {
					"source": ["obj-route", 1],
					"destination": ["obj-stop-msg", 0]
				}
			},
			{
				"patchline": {
					"source": ["obj-route", 2],
					"destination": ["obj-start-msg", 0]
				}
			},
			{
				"patchline": {
					"source": ["obj-start-msg", 0],
					"destination": ["obj-node-script", 0]
				}
			},
			{
				"patchline": {
					"source": ["obj-stop-msg", 0],
					"destination": ["obj-node-script", 0]
				}
			}
		]
	}
}
