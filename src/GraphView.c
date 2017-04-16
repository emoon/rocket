#include "GraphView.h"
#include <emgui/Emgui.h>
#include <sync.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <float.h>

//static uint32_t grid_square_color = EMGUI_COLOR32(0x95, 0x96, 0x95, 0xff);
//static uint32_t grid_line_color = EMGUI_COLOR32(0x12, 0x12, 0x12, 0xff);

// Precise method, which guarantees v = v1 when t = 1.
float lerp(float v0, float v1, float t) {
	return (1.0 - t) * v0 + t * v1;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static void drawGrid(const Rect* rect, const float startValue, const float endValue, int fontY)
{
	char text[256];
	int count = rect->height / (fontY + 4);

	float count_step = 1.0f / (float)(count - 1); 
	float t = 0.0f;
	int y = rect->y + 2;
	int x = rect->x + 10;

	for (int i = 0; i < count; ++i) {
		float v = lerp(startValue, endValue, t);
		snprintf(text, sizeof(text), "% .2f", v);

		Emgui_drawText(text, x, y, Emgui_color32(255, 255, 255, 255));
		y += fontY + 4;
		t += count_step;
	}


	//float range_step = (


	// figure out how many rows we can render

}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void GraphView_render(const GraphView* graphView, const GraphSettings* settings, const Rect* rect)
{
	float temp_vals[4096];
	Vec2 line_vals[4096];

	Emgui_setFont(settings->fontId);

	// TODO: Precalc

	const uint32_t fontYSize = Emgui_getTextSize("0123456789") >> 16;

	Emgui_drawBorder(settings->borderColor, settings->borderColor, 
					 rect->x, rect->y, rect->width, rect->height);


	if (!graphView->activeTrack) {
		return;
	}

	//static void drawGrid(const Rect* rect, const float startValue, const float endValue, int fontY)

	drawGrid(rect, 100, -100, fontYSize);

	// Eval the data

	int count = graphView->endRow - graphView->startRow; 

	if (count > sizeof_array(temp_vals)) {
		count = sizeof_array(temp_vals);
	}

	for (int i = 0; i < count; ++i) {
		temp_vals[i] = sync_get_val(graphView->activeTrack, graphView->startRow + i);
	}

	float min_range = FLT_MAX;
	float max_range = -FLT_MAX;

	// find scale

	for (int i = 0; i < count; ++i) {
		int r = temp_vals[i];

		if (r < min_range) {
			min_range = r;
		}

		if (r > max_range) {
			max_range = r;
		}
	}

	float inv_range = 1.0f / (fabs(min_range) + fabs(max_range));

	float x_scale = ((float)rect->width) / (float)count;
	float x = rect->x; 

	for (int i = 0; i < count; ++i) {
		line_vals[i].x = x;
		line_vals[i].y = rect->y + (rect->height / 2) - ((temp_vals[i] * inv_range) * (rect->height / 2));
		x += x_scale;
	}
	
	Emgui_drawLines(line_vals, count, EMGUI_COLOR32(0xff, 0x96, 0x40, 0xff), 2, 1);

	/*

	Vec2 coords[2];

	coords[0].x = rect->x;
	coords[0].y = rect->y;

	coords[1].x = rect->x + rect->width;
	coords[1].y = rect->y + rect->height;

	Emgui_drawLines(coords, sizeof_array(coords), EMGUI_COLOR32(0xff, 0x96, 0x40, 0x80));
	*/
}




