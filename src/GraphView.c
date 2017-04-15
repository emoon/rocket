#include "GraphView.h"
#include <emgui/Emgui.h>
#include <sync.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void GraphView_render(const GraphView* graphView, const GraphSettings* settings, const Rect* rect)
{
	float temp_vals[4096];
	Vec2 line_vals[4096];

	Emgui_drawBorder(settings->borderColor, settings->borderColor, 
					 rect->x, rect->y, rect->width, rect->height);

	if (!graphView->activeTrack) {
		return;
	}

	printf("graphView->activeTrack %p\n", (void*)graphView->activeTrack);

	// Eval the data

	int count = graphView->endRow - graphView->startRow; 

	if (count > sizeof_array(temp_vals)) {
		count = sizeof_array(temp_vals);
	}

	for (int i = 0; i < count; ++i) {
		temp_vals[i] = sync_get_val(graphView->activeTrack, graphView->startRow + i);
	}

	float range = -10000.0f;

	// find scale

	for (int i = 0; i < count; ++i) {
		int av = fabs(temp_vals[i]);

		if (av > range) {
			range = av;
		}
	}

	float x_scale = ((float)rect->width) / (float)count;
	float x = rect->x; 

	for (int i = 0; i < count; ++i) {
		line_vals[i].x = x;
		line_vals[i].y = rect->y + (rect->height / 2) + (temp_vals[i] / range) * (rect->height / 2);
		x += x_scale;
	}
	
	Emgui_drawLines(line_vals, count, EMGUI_COLOR32(0xff, 0x96, 0x40, 0x80));

	/*

	Vec2 coords[2];

	coords[0].x = rect->x;
	coords[0].y = rect->y;

	coords[1].x = rect->x + rect->width;
	coords[1].y = rect->y + rect->height;

	Emgui_drawLines(coords, sizeof_array(coords), EMGUI_COLOR32(0xff, 0x96, 0x40, 0x80));
	*/
}




