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
					"text" : "MaxMCP - Synth Creation Example",
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
					"patching_rect" : [ 20.0, 70.0, 600.0, 34.0 ],
					"text" : "This example demonstrates using Claude Code to create a simple synthesizer.\nUse natural language to build the synth step by step.",
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
					"patching_rect" : [ 20.0, 120.0, 110.0, 22.0 ],
					"text" : "maxmcp synth-demo"
				}

			}
, 			{
				"box" : 				{
					"id" : "obj-5",
					"maxclass" : "comment",
					"numinlets" : 1,
					"numoutlets" : 0,
					"patching_rect" : [ 20.0, 160.0, 600.0, 20.0 ],
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
					"patching_rect" : [ 20.0, 190.0, 600.0, 224.0 ],
					"text" : "Step 1: Create oscillator\n  \"Create a cycle~ object at [100, 100] with argument 440, name it 'osc'\"\n\nStep 2: Create frequency control\n  \"Add a number box at [100, 50] named 'freq', set min to 20 and max to 2000\"\n\nStep 3: Create filter\n  \"Create a biquad~ object at [100, 200] named 'filter'\"\n\nStep 4: Create output\n  \"Add a dac~ object at [100, 300] named 'output'\"\n\nStep 5: Make connections\n  \"Connect freq to osc inlet 0\"\n  \"Connect osc outlet 0 to filter inlet 0\"\n  \"Connect filter outlet 0 to output inlet 0\"\n  \"Connect filter outlet 0 to output inlet 1\"",
					"linecount" : 16,
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
					"patching_rect" : [ 20.0, 430.0, 600.0, 34.0 ],
					"text" : "Result: A complete synthesizer built through natural language!\nTurn on audio (click speaker icon) and adjust the frequency number box to hear it.",
					"linecount" : 2,
					"textcolor" : [ 0.0, 0.0, 0.0, 1.0 ]
				}

			}
 ],
		"lines" : [  ]
	}

}
