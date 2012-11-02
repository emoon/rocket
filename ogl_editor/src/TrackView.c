#include "trackview.h"
#include <Emgui.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "TrackData.h"
#include "rlog.h"
#include "../../sync/sync.h"
#include "../../sync/data.h"
#include "../../sync/track.h"

const int font_size = 8;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void TrackView_init()
{
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static void printRowNumbers(int x, int y, int rowCount, int rowOffset, int rowSpacing, int maskBpm, int endY)
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
		memset(rowText, 0, sizeof(rowText));
		sprintf(rowText, "%04d", rowOffset);

		if (rowOffset % maskBpm)
			Emgui_drawText(rowText, x, y, Emgui_color32(0xa0, 0xa0, 0xa0, 0xff));
		else
			Emgui_drawText(rowText, x, y, Emgui_color32(0xf0, 0xf0, 0xf0, 0xff));

		y += rowSpacing;
		rowOffset++;

		if (y > endY)
			break;
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static void renderChannel(struct sync_track* track, int startX, int startY, int startPos, int endPos, int endSizeY)
{
	uint y;

	uint32_t color = Emgui_color32(40, 40, 40, 255);
	Emgui_drawBorder(color, color, startX, startY - font_size * 2, 160, endSizeY);

	if (track)
		Emgui_drawText(track->name, startX + 4, startY - font_size, Emgui_color32(0xff, 0xff, 0xff, 0xff));

	int y_offset = startY;

	if (startPos < 0)
	{
		y_offset = startY + (font_size * -startPos);
		startPos = 0;
	}

	y_offset += font_size / 2;

	for (y = startPos; y < endPos; ++y)
	{
		int offset = startX + 6;
		int idx = -1;

		float value = 0.0f;

		if (track)
			idx = sync_find_key(track, y);

		if (idx >= 0)
		{
			char temp[256];
			value = track->keys[idx].value;
			snprintf(temp, 256, "% .2f", value);

			Emgui_drawText(temp, offset, y_offset - font_size / 2, Emgui_color32(255, 255, 255, 255));
		}
		else
		{
			uint32_t color = 0; 

			if (y & 7)
				color = Emgui_color32(0x4f, 0x4f, 0x4f, 0xff); 
			else
				color = Emgui_color32(0x7f, 0x7f, 0x7f, 0xff); 

			Emgui_drawText("---", offset, y_offset - font_size / 2, color); 
		}

		y_offset += font_size;

		if (y_offset > (endSizeY + font_size/2))
			break;
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

int doMax(int a, int b)
{
	if (b >= a)
		return b;
	else
		return a;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static inline int min(int a, int b)
{
	if (a < b)
		return a;
	else
		return b;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// foo 

void TrackView_render(const TrackViewInfo* viewInfo, TrackData* trackData)
{
	uint i = 0;
	struct sync_data* syncData = &trackData->syncData;
	const int sel_track = trackData->activeTrack;

	// Calc to position the selection in the ~middle of the screen 

	const int adjust_top_size = 3 * font_size;
	const int mid_screen_y = (viewInfo->windowSizeY / 2) & ~(font_size - 1);
	const int y_pos_row = viewInfo->rowPos - (mid_screen_y / font_size);

	// TODO: Calculate how many channels we can draw given the width

	int end_row = viewInfo->windowSizeY / font_size;
	int y_end_border = viewInfo->windowSizeY - 32; // adjust to have some space at the end of the screen

	printRowNumbers(2, adjust_top_size, end_row, y_pos_row, font_size, 8, y_end_border);

	if (syncData->num_tracks == 0)
	{
		renderChannel(0, 40 + (i * 64), adjust_top_size, y_pos_row, y_pos_row + end_row, y_end_border);
		uint32_t color = Emgui_color32(127, 127, 127, 56);
		Emgui_fill(color, 0, mid_screen_y + adjust_top_size, viewInfo->windowSizeX, font_size + 2);
		return;
	}

	int num_tracks = syncData->num_tracks;

	int max_render_tracks = viewInfo->windowSizeX / 128;

	if (num_tracks > max_render_tracks)
		num_tracks = max_render_tracks;

	int start_track = 0;

	if (sel_track > 3)
		start_track = sel_track - 3;

	int x_pos = 40;

	const int end_track = min(start_track + num_tracks, syncData->num_tracks);

	for (i = start_track; i < end_track; ++i)
	{
		renderChannel(syncData->tracks[i], x_pos, adjust_top_size, y_pos_row, y_pos_row + end_row, y_end_border);

		if (sel_track == i)
		{
			Emgui_fill(Emgui_color32(0xff, 0xff, 0x00, 0x80), x_pos, mid_screen_y + adjust_top_size, 160, font_size + 1);

			if (trackData->editText)
				Emgui_drawText(trackData->editText, x_pos, mid_screen_y + adjust_top_size, Emgui_color32(255, 255, 255, 255));
		}

		x_pos += 160;
	}	

	uint32_t color = Emgui_color32(127, 127, 127, 56);
	Emgui_fill(color, 0, mid_screen_y + adjust_top_size, viewInfo->windowSizeX, font_size + 1);
}

