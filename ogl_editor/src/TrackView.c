#include "trackview.h"
#include <Emgui.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "TrackData.h"
#include "../../sync/sync.h"
#include "../../sync/data.h"
#include "../../sync/track.h"

const int font_size = 8;
static int start_pos = -27;

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
		memset(rowText, 0, sizeof(rowText));
		sprintf(rowText, "%04d", rowOffset);

		if (rowOffset % maskBpm)
			Emgui_drawText(rowText, x, y, Emgui_color32(0xa0, 0xa0, 0xa0, 0xff));
		else
			Emgui_drawText(rowText, x, y, Emgui_color32(0xf0, 0xf0, 0xf0, 0xff));

		y += rowSpacing;
		rowOffset++;
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static void renderChannel(struct sync_track* track, int startX, int startY, int startPos, int endPos)
{
	uint x, y;

	uint32_t color = Emgui_color32(40, 40, 40, 255);
	Emgui_drawBorder(color, color, startX, startY - 20, 160, 600);

	if (track)
		Emgui_drawText(track->name, startX + 2, startY + 2, Emgui_color32(0xff, 0xff, 0xff, 0xff));

	int y_offset = startY;// + font_size / 2;

	if (startPos < 0)
	{
		y_offset = startY + (font_size * -startPos);
		startPos = 0;
		endPos = 40;
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

void TrackView_render(const TrackViewInfo* viewInfo, TrackData* trackData)
{
	struct sync_data* syncData = &trackData->syncData;
	const int sel_track = trackData->activeTrack;

	// TODO: Calculate how many channels we can draw given the width

	uint i = 0; //, channel_count = 10; 

	printRowNumbers(2, 42, 40, start_pos + viewInfo->rowPos, font_size, 8);

	if (syncData->num_tracks == 0)
	{
		renderChannel(0, 40 + (i * 64), 42, 
				(start_pos + viewInfo->rowPos), 
				(start_pos + viewInfo->rowPos + 40));
		uint32_t color = Emgui_color32(127, 127, 127, 56);
		Emgui_fill(color, 0, 257, 800, font_size + 2);
		return;
	}

	int num_tracks = syncData->num_tracks;

	if (num_tracks > 5)
		num_tracks = 5;

	int start_track = 0;

	if (sel_track > 3)
		start_track = sel_track - 3;

	int x_pos = 40;

	for (i = start_track; i < min(start_track + num_tracks, syncData->num_tracks - 1); ++i)
	{
		renderChannel(syncData->tracks[i], x_pos, 42, 
				(start_pos + viewInfo->rowPos), 
				(start_pos + viewInfo->rowPos + 40));

		if (sel_track == i)
		{
			Emgui_fill(Emgui_color32(0xff, 0xff, 0x00, 0x80), x_pos, 257, 160, font_size + 2);

			if (trackData->editText)
				Emgui_drawText(trackData->editText, x_pos, 257, Emgui_color32(255, 255, 255, 255));
		}

		x_pos += 160;
	}	

	uint32_t color = Emgui_color32(127, 127, 127, 56);

	Emgui_fill(color, 0, 257, 800, font_size + 3);
}

