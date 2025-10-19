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
					"text" : "MaxMCP - Multi-Patch Example (Patch A)",
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
					"bgcolor" : [ 1.0, 0.9, 0.9, 1.0 ],
					"rounded" : 8
				}

			}
, 			{
				"box" : 				{
					"id" : "obj-3",
					"maxclass" : "comment",
					"numinlets" : 1,
					"numoutlets" : 0,
					"patching_rect" : [ 20.0, 70.0, 600.0, 62.0 ],
					"text" : "This example demonstrates working with multiple patches simultaneously.\n\nInstructions:\n1. Open this patch (Patch A)\n2. Create a new patch and add [maxmcp patch-b]\n3. Use Claude Code to target specific patches by name",
					"linecount" : 5,
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
					"patching_rect" : [ 20.0, 150.0, 80.0, 22.0 ],
					"text" : "maxmcp patch-a"
				}

			}
, 			{
				"box" : 				{
					"id" : "obj-5",
					"maxclass" : "comment",
					"numinlets" : 1,
					"numoutlets" : 0,
					"patching_rect" : [ 110.0, 150.0, 510.0, 20.0 ],
					"text" : "← This patch is registered as \"patch-a_{uuid}\"",
					"textcolor" : [ 0.0, 0.0, 0.0, 1.0 ]
				}

			}
, 			{
				"box" : 				{
					"id" : "obj-6",
					"maxclass" : "comment",
					"numinlets" : 1,
					"numoutlets" : 0,
					"patching_rect" : [ 20.0, 200.0, 600.0, 20.0 ],
					"text" : "Example Commands:",
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
					"patching_rect" : [ 20.0, 230.0, 600.0, 182.0 ],
					"text" : "List all patches:\n  \"List active patches\"\n  → Shows: patch-a_{uuid}, patch-b_{uuid}, etc.\n\nTarget specific patch:\n  \"In patch-a, create a toggle at [100, 100] named 'switch'\"\n  → Adds toggle only to patch-a\n\n  \"In patch-b, create a message box at [100, 100] with text 'hello'\"\n  → Adds message only to patch-b\n\nGet patch info:\n  \"Get info about patch-a\"\n  → Returns patch-a metadata",
					"linecount" : 13,
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
					"patching_rect" : [ 20.0, 430.0, 600.0, 34.0 ],
					"text" : "Each patch has a unique ID, allowing Claude Code to precisely target patches.\nThis enables complex multi-patch workflows.",
					"linecount" : 2,
					"textcolor" : [ 0.0, 0.0, 0.0, 1.0 ]
				}

			}
 ],
		"lines" : [  ]
	}

}
