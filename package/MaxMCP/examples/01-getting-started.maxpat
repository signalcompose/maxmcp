{
	"patcher" : 	{
		"fileversion" : 1,
		"appversion" : 		{
			"major" : 9,
			"minor" : 0,
			"revision" : 0,
			"architecture" : "x64",
			"modernui" : 1
		}
,
		"classnamespace" : "box",
		"rect" : [ 100.0, 100.0, 640.0, 480.0 ],
		"bgcolor" : [ 1.0, 1.0, 1.0, 1.0 ],
		"gridsize" : [ 15.0, 15.0 ],
		"boxes" : [ 			{
				"box" : 				{
					"id" : "obj-1",
					"maxclass" : "comment",
					"numinlets" : 1,
					"numoutlets" : 0,
					"patching_rect" : [ 20.0, 20.0, 600.0, 20.0 ],
					"text" : "MaxMCP - Getting Started Example",
					"fontsize" : 14.0,
					"fontface" : 1,
					"textcolor" : [ 0.0, 0.0, 0.0, 1.0 ]
				}

			}
, 			{
				"box" : 				{
					"id" : "obj-2",
					"maxclass" : "panel",
					"numinlets" : 1,
					"numoutlets" : 0,
					"patching_rect" : [ 10.0, 10.0, 620.0, 40.0 ],
					"bgcolor" : [ 0.9, 0.9, 1.0, 1.0 ],
					"rounded" : 8
				}

			}
, 			{
				"box" : 				{
					"id" : "obj-3",
					"maxclass" : "comment",
					"numinlets" : 1,
					"numoutlets" : 0,
					"patching_rect" : [ 20.0, 70.0, 600.0, 48.0 ],
					"text" : "This is a simple introduction to MaxMCP. The [maxmcp] object below registers this patch with the MaxMCP server, enabling Claude Code to control it using natural language.",
					"linecount" : 2,
					"textcolor" : [ 0.0, 0.0, 0.0, 1.0 ]
				}

			}
, 			{
				"box" : 				{
					"id" : "obj-4",
					"maxclass" : "newobj",
					"numinlets" : 1,
					"numoutlets" : 1,
					"outlettype" : [ "" ],
					"patching_rect" : [ 20.0, 140.0, 120.0, 22.0 ],
					"text" : "maxmcp getting-started"
				}

			}
, 			{
				"box" : 				{
					"id" : "obj-5",
					"maxclass" : "comment",
					"numinlets" : 1,
					"numoutlets" : 0,
					"patching_rect" : [ 150.0, 140.0, 470.0, 20.0 ],
					"text" : "← This registers the patch as \"getting-started_{uuid}\"",
					"textcolor" : [ 0.0, 0.0, 0.0, 1.0 ]
				}

			}
, 			{
				"box" : 				{
					"id" : "obj-6",
					"maxclass" : "comment",
					"numinlets" : 1,
					"numoutlets" : 0,
					"patching_rect" : [ 20.0, 190.0, 600.0, 20.0 ],
					"text" : "What you can do with Claude Code:",
					"fontface" : 1,
					"fontsize" : 12.0,
					"textcolor" : [ 0.0, 0.0, 0.0, 1.0 ]
				}

			}
, 			{
				"box" : 				{
					"id" : "obj-7",
					"maxclass" : "comment",
					"numinlets" : 1,
					"numoutlets" : 0,
					"patching_rect" : [ 20.0, 220.0, 600.0, 140.0 ],
					"text" : "1. \"List active patches\"\n   → Shows this patch as \"getting-started_{uuid}\"\n\n2. \"Create a number box at position [200, 100] named 'freq'\"\n   → Adds a number box to this patch\n\n3. \"Create a button at [200, 150] named 'trigger'\"\n   → Adds a button object\n\n4. \"Connect freq to trigger's inlet\"\n   → Creates a patchcord between the two objects",
					"linecount" : 10,
					"fontname" : "Courier",
					"fontsize" : 10.0,
					"textcolor" : [ 0.0, 0.0, 0.0, 1.0 ]
				}

			}
, 			{
				"box" : 				{
					"id" : "obj-8",
					"maxclass" : "comment",
					"numinlets" : 1,
					"numoutlets" : 0,
					"patching_rect" : [ 20.0, 380.0, 600.0, 48.0 ],
					"text" : "Try these commands in Claude Code and watch as objects appear in this patch!\nSee the help patch (Command+B on maxmcp object) for more information.",
					"linecount" : 2,
					"textcolor" : [ 0.0, 0.0, 0.0, 1.0 ]
				}

			}
 ],
		"lines" : [  ]
	}

}
