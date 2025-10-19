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
		"rect" : [ 100.0, 100.0, 640.0, 520.0 ],
		"bgcolor" : [ 1.0, 1.0, 1.0, 1.0 ],
		"gridsize" : [ 15.0, 15.0 ],
		"boxes" : [ 			{
				"box" : 				{
					"id" : "obj-1",
					"maxclass" : "comment",
					"numinlets" : 1,
					"numoutlets" : 0,
					"patching_rect" : [ 20.0, 20.0, 600.0, 20.0 ],
					"text" : "MaxMCP - Effect Chain Example",
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
					"bgcolor" : [ 0.9, 1.0, 0.9, 1.0 ],
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
					"text" : "This example demonstrates building an audio effect chain using Claude Code.\nYou'll create a signal flow: input → delay → reverb → filter → output",
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
					"patching_rect" : [ 20.0, 140.0, 100.0, 22.0 ],
					"text" : "maxmcp fx-chain"
				}

			}
, 			{
				"box" : 				{
					"id" : "obj-5",
					"maxclass" : "comment",
					"numinlets" : 1,
					"numoutlets" : 0,
					"patching_rect" : [ 20.0, 180.0, 600.0, 20.0 ],
					"text" : "Example Commands:",
					"fontface" : 1,
					"fontsize" : 12.0,
					"textcolor" : [ 0.0, 0.0, 0.0, 1.0 ]
				}

			}
, 			{
				"box" : 				{
					"id" : "obj-6",
					"maxclass" : "comment",
					"numinlets" : 1,
					"numoutlets" : 0,
					"patching_rect" : [ 20.0, 210.0, 600.0, 266.0 ],
					"text" : "Build the effect chain:\n\nStep 1: Create input\n  \"Create an adc~ object at [50, 100] named 'input'\"\n\nStep 2: Add delay effect\n  \"Create a tapin~ object at [50, 200] with argument 2000, named 'delay_in'\"\n  \"Create a tapout~ object at [50, 250] with argument 500, named 'delay_out'\"\n\nStep 3: Add reverb\n  \"Create a reverb~ object at [50, 300] named 'reverb'\"\n\nStep 4: Add filter\n  \"Create a biquad~ object at [50, 400] named 'filter'\"\n\nStep 5: Create output\n  \"Create a dac~ object at [50, 500] named 'output'\"\n\nStep 6: Connect the chain\n  \"Connect input outlet 0 to delay_in inlet 0\"\n  \"Connect delay_out outlet 0 to reverb inlet 0\"\n  \"Connect reverb outlet 0 to filter inlet 0\"\n  \"Connect filter outlet 0 to output inlet 0\"\n  \"Connect filter outlet 0 to output inlet 1\"",
					"linecount" : 19,
					"fontname" : "Courier",
					"fontsize" : 10.0,
					"textcolor" : [ 0.0, 0.0, 0.0, 1.0 ]
				}

			}
, 			{
				"box" : 				{
					"id" : "obj-7",
					"maxclass" : "comment",
					"numinlets" : 1,
					"numoutlets" : 0,
					"patching_rect" : [ 20.0, 490.0, 600.0, 20.0 ],
					"text" : "Advanced: Modify attributes",
					"fontface" : 1,
					"fontsize" : 12.0,
					"textcolor" : [ 0.0, 0.0, 0.0, 1.0 ]
				}

			}
, 			{
				"box" : 				{
					"id" : "obj-8",
					"maxclass" : "comment",
					"numinlets" : 1,
					"numoutlets" : 0,
					"patching_rect" : [ 20.0, 520.0, 600.0, 28.0 ],
					"text" : "  \"Set the cutoff frequency of filter to 1000\"\n  \"Set the Q factor of filter to 2.0\"",
					"linecount" : 2,
					"fontname" : "Courier",
					"fontsize" : 10.0,
					"textcolor" : [ 0.0, 0.0, 0.0, 1.0 ]
				}

			}
 ],
		"lines" : [  ]
	}

}
