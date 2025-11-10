{
	"patcher": {
		"fileversion": 1,
		"appversion": {
			"major": 9,
			"minor": 0,
			"revision": 0
		},
		"rect": [250, 250, 800, 600],
		"boxes": [
			{
				"box": {
					"maxclass": "comment",
					"text": "Test 7: MCP Tools Test Patch",
					"fontsize": 14,
					"fontface": 1,
					"patching_rect": [20, 20, 400, 22]
				}
			},
			{
				"box": {
					"maxclass": "comment",
					"text": "This is a target patch for Claude Code to manipulate using MCP tools.",
					"patching_rect": [20, 50, 650, 20]
				}
			},
			{
				"box": {
					"maxclass": "comment",
					"text": "Test Scenarios:",
					"fontsize": 12,
					"fontface": 1,
					"patching_rect": [20, 85, 200, 20]
				}
			},
			{
				"box": {
					"maxclass": "comment",
					"text": "1. add_max_object: \"Add a button at position [100, 100]\"",
					"patching_rect": [20, 110, 550, 20]
				}
			},
			{
				"box": {
					"maxclass": "comment",
					"text": "2. set_object_attribute: \"Set the button color to red\"",
					"patching_rect": [20, 130, 550, 20]
				}
			},
			{
				"box": {
					"maxclass": "comment",
					"text": "3. connect_max_objects: \"Connect button outlet to number inlet\"",
					"patching_rect": [20, 150, 550, 20]
				}
			},
			{
				"box": {
					"maxclass": "comment",
					"text": "4. get_objects_in_patch: \"List all objects in this patch\"",
					"patching_rect": [20, 170, 550, 20]
				}
			},
			{
				"box": {
					"maxclass": "comment",
					"text": "5. get_avoid_rect_position: \"Find empty position for new object\"",
					"patching_rect": [20, 190, 550, 20]
				}
			},
			{
				"box": {
					"maxclass": "comment",
					"text": "6. disconnect_max_objects: \"Disconnect button from number\"",
					"patching_rect": [20, 210, 550, 20]
				}
			},
			{
				"box": {
					"maxclass": "comment",
					"text": "7. remove_max_object: \"Remove the button\"",
					"patching_rect": [20, 230, 550, 20]
				}
			},
			{
				"box": {
					"maxclass": "comment",
					"text": "Initial Objects:",
					"fontsize": 12,
					"fontface": 1,
					"patching_rect": [20, 265, 200, 20]
				}
			},
			{
				"box": {
					"maxclass": "comment",
					"text": "This patch starts with a simple number object for testing.",
					"patching_rect": [20, 290, 500, 20]
				}
			},
			{
				"box": {
					"maxclass": "number",
					"patching_rect": [20, 330, 50, 22],
					"numinlets": 1,
					"numoutlets": 2,
					"outlettype": ["", "bang"],
					"parameter_enable": 0,
					"id": "obj-number",
					"varname": "test_number"
				}
			},
			{
				"box": {
					"maxclass": "comment",
					"text": "← Test number object (varname: test_number)",
					"patching_rect": [80, 335, 350, 20]
				}
			},
			{
				"box": {
					"maxclass": "comment",
					"text": "Canvas Area:",
					"fontsize": 12,
					"fontface": 1,
					"patching_rect": [20, 375, 200, 20]
				}
			},
			{
				"box": {
					"maxclass": "comment",
					"text": "Claude Code will add objects to this empty area below.",
					"patching_rect": [20, 400, 500, 20]
				}
			},
			{
				"box": {
					"maxclass": "panel",
					"bgcolor": [0.15, 0.15, 0.15, 1.0],
					"patching_rect": [20, 430, 740, 130]
				}
			},
			{
				"box": {
					"maxclass": "comment",
					"text": "↑ Empty canvas for Claude Code to work",
					"textcolor": [0.7, 0.7, 0.7, 1.0],
					"patching_rect": [250, 520, 300, 20]
				}
			},
			{
				"box": {
					"maxclass": "newobj",
					"text": "maxmcp @mode patch @alias test-target @group test",
					"patching_rect": [500, 20, 350, 22],
					"numinlets": 0,
					"numoutlets": 0,
					"id": "obj-maxmcp"
				}
			},
			{
				"box": {
					"maxclass": "comment",
					"text": "← MaxMCP registration",
					"patching_rect": [860, 25, 150, 20]
				}
			},
			{
				"box": {
					"maxclass": "panel",
					"bgcolor": [0.25, 0.35, 0.45, 1.0],
					"patching_rect": [10, 10, 770, 560]
				}
			}
		],
		"lines": []
	}
}
