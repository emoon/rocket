#include "trackview.h"
#include <Emgui.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

const int font_size = 12;
static int start_pos = -19;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void TrackView_init()
{
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static void printRowNumbers(int x, int y, int rowCount, int rowOffset, int rowSpacing, int maskBpm)
{
	int i;

	if (rowOffset < 0)
	{
		y += rowSpacing * -rowOffset;
		rowOffset  = 0;
	}

	for (i = 0; i < rowCount; ++i)
	{
		char rowText[16];
		sprintf(rowText, "%04d\n", rowOffset);

		if (rowOffset % maskBpm)
			Emgui_drawText(rowText, x, y, Emgui_color32(0xa0, 0xa0, 0xa0, 0xff));
		else
			Emgui_drawText(rowText, x, y, Emgui_color32(0xf0, 0xf0, 0xf0, 0xff));

		y += rowSpacing;
		rowOffset++;
	}
}


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/*
static void renderChannel(Channel* channel, int startX, int startY, int startPos, int endPos)
{
	uint x, y;

	uint32_t color = Emgui_color32(40, 40, 40, 255);
	Emgui_drawBorder(color, color, startX, startY, 64, 600);
	int y_offset = (startY + font_size) + font_size/2;

	if (startPos < 0)
	{
		y_offset = ((startY + font_size * -startPos) + font_size) + font_size/2;
		startPos = 0;
		endPos = 40;
	}

	//int count = 0;

	//float values[40];

	for (y = startPos; y < endPos; ++y)
	{
		int offset = startX + 6;

		float value = 0.0f;
		bool set = false;

		if (y < channel->maxRange)
		{
			set = channel->set[y];
			value = channel->values[y];
		}

		if (set)
		{
			char valueText[16];

			if (value < 0.0f)
				sprintf(valueText, "%0.8f", value);
			else
				sprintf(valueText, "%8.4f", value);

			Emgui_drawText(valueText, offset, y_offset + (font_size / 2), Emgui_color32(255, 255, 255, 255));
		}
		else
		{
			int points[64];
			int* points_ptr = (int*)&points[0];

			for (x = 0; x < 6; ++x)
			{
				points_ptr[0] = offset + 0;
				points_ptr[1] = y_offset;
				points_ptr[2] = offset + 2;
				points_ptr[3] = y_offset;
				points_ptr[4] = offset + 4;
				points_ptr[5] = y_offset;

				points_ptr += 6;
				offset += 10;
			}

			if (y & 7)
				Emgui_drawDots(0x004f4f4f, (int*)&points[0], 18 * 2);
			else
				Emgui_drawDots(0x007f7f7f, (int*)&points[0], 18 * 2);
		}

		y_offset += font_size;
	}
}
*/


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void TrackView_render()
{
	//uint i = 0; //, channel_count = 10; 

	printRowNumbers(2, 42, 40, start_pos, font_size, 8);

	//i = 0;

	//for (i = 0; i < channel_count; ++i)
	//		renderChannel(&s_testChannel, 40 + (i * 64), 20, start_pos, start_pos + 40);

	uint32_t color = Emgui_color32(127, 127, 127, 56);

	Emgui_fill(color, 0, 257, 800, font_size + 3);
}

