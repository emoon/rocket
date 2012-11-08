#include "trackview.h"
#include <Emgui.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "TrackData.h"
#include "rlog.h"
#include "minmax.h"
#include "../../sync/sync.h"
#include "../../sync/data.h"
#include "../../sync/track.h"

const int font_size = 8;
const int min_track_size = 100;

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

static int renderChannel(const TrackViewInfo* viewInfo, struct sync_track* track, int editRow, 
						 int startX, int startY, int startPos, int endPos, int endSizeY,
						 int trackId, int selectLeft, int selectRight, int selectTop, int selectBottom)
{
	int y, y_offset;
	int size = min_track_size;
	uint32_t color = Emgui_color32(40, 40, 40, 255);

	if (track)
	{
		int text_size;
		int x_adjust = 0;

		Emgui_setFont(viewInfo->smallFontId);
		text_size = (Emgui_getTextSize(track->name) & 0xffff) + 4;

		// if text is smaller than min size we center the text

		if (text_size < min_track_size) 
			x_adjust = (min_track_size - text_size) / 2;
		else
			size = text_size + 1; 

		Emgui_drawText(track->name, (startX + 3) + x_adjust, startY - 12, Emgui_color32(0xff, 0xff, 0xff, 0xff));
		Emgui_setDefaultFont();
	}

	Emgui_drawBorder(color, color, startX, startY - font_size * 2, size, endSizeY);

	y_offset = startY;// + font_size / 2;

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
		int fidx;
		enum key_type interpolationType;
		uint32_t color;
		bool selected;
		float value = 0.0f;

		if (track)
			idx = sync_find_key(track, y);

		// This is kinda crappy implementation as we will overdraw this quite a bit but might be fine

		fidx = idx >= 0 ? idx : -idx - 2;
		interpolationType = (fidx >= 0) ? track->keys[fidx].type : KEY_STEP;

		switch (interpolationType) 
		{
			case KEY_STEP   : color = Emgui_color32(0, 0, 0,   255);; break;
			case KEY_LINEAR : color = Emgui_color32(255, 0, 0, 255); break;
			case KEY_SMOOTH : color = Emgui_color32(0, 255, 0, 255); break;
			case KEY_RAMP   : color = Emgui_color32(0, 0, 255, 255); break;
			default: break;
		}

		if (viewInfo)
			Emgui_fill(color, startX + (size - 4), y_offset - font_size / 2, 2, (endSizeY - y_offset) + font_size - 1);

		// Draw text if we have any

		if (idx >= 0)
		{
			char temp[256];
			value = track->keys[idx].value;
			snprintf(temp, 256, "% .2f", value);

			if (y != editRow)
				Emgui_drawText(temp, offset, y_offset - font_size / 2, Emgui_color32(255, 255, 255, 255));
		}
		else
		{
			uint32_t color = (y & 7) ? Emgui_color32(0x4f, 0x4f, 0x4f, 0xff) : Emgui_color32(0x7f, 0x7f, 0x7f, 0xff); 
			Emgui_drawText("---", offset, y_offset - font_size / 2, color); 
		}

		selected = (trackId >= selectLeft && trackId <= selectRight) && (y >= selectTop && y < selectBottom);

		if (selected)
			Emgui_fill(Emgui_color32(0x4f, 0x4f, 0x4f, 0x3f), startX, y_offset - font_size/2, size, font_size);  

		y_offset += font_size;

		if (y_offset > (endSizeY + font_size/2))
			break;
	}

	return size;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void TrackView_render(const TrackViewInfo* viewInfo, TrackData* trackData)
{
	struct sync_data* syncData = &trackData->syncData;
	const int sel_track = trackData->activeTrack;
	//uint32_t color = Emgui_color32(127, 127, 127, 56);
	int num_tracks;
	int max_render_tracks;
	int start_track = 0;
	int x_pos = 40;
	int end_track = 0;
	int i = 0;
	int adjust_top_size;
	int mid_screen_y ;
	int y_pos_row, end_row, y_end_border;

	int selectLeft  = emini(viewInfo->selectStartTrack, viewInfo->selectStopTrack);
	int selectRight = emaxi(viewInfo->selectStartTrack, viewInfo->selectStopTrack);
	int selectTop    = emini(viewInfo->selectStartRow, viewInfo->selectStopRow);
	int selectBottom = emaxi(viewInfo->selectStartRow, viewInfo->selectStopRow);

	// Calc to position the selection in the ~middle of the screen 

	adjust_top_size = 3 * font_size;
	mid_screen_y = (viewInfo->windowSizeY / 2) & ~(font_size - 1);
	y_pos_row = viewInfo->rowPos - (mid_screen_y / font_size);

	// TODO: Calculate how many channels we can draw given the width

	end_row = viewInfo->windowSizeY / font_size;
	y_end_border = viewInfo->windowSizeY - 32; // adjust to have some space at the end of the screen

	printRowNumbers(2, adjust_top_size, end_row, y_pos_row, font_size, 8, y_end_border);

	if (syncData->num_tracks == 0)
	{
		uint32_t color = Emgui_color32(127, 127, 127, 56);
		renderChannel(0, 0, -1, 40 + (i * 64), adjust_top_size, y_pos_row, y_pos_row + end_row, y_end_border,
				      0, 0, 0, 0, 0);
		Emgui_fill(color, 0, mid_screen_y + adjust_top_size, viewInfo->windowSizeX, font_size + 2);
		return;
	}

	num_tracks = syncData->num_tracks;
	max_render_tracks = viewInfo->windowSizeX / min_track_size;

	if (num_tracks > max_render_tracks)
		num_tracks = max_render_tracks;

	if (sel_track > 3)
		start_track = sel_track - 3;

	end_track = emini(start_track + num_tracks, syncData->num_tracks);

	for (i = start_track; i < end_track; ++i)
	{
		int size, editRow = -1;

		if (sel_track == i && trackData->editText)
			editRow = viewInfo->rowPos;

		size = renderChannel(viewInfo, syncData->tracks[i], editRow, x_pos, adjust_top_size, y_pos_row, y_pos_row + end_row, y_end_border,
							 i, selectLeft, selectRight, selectTop, selectBottom);

		if (sel_track == i)
		{
			Emgui_fill(Emgui_color32(0xff, 0xff, 0x00, 0x80), x_pos, mid_screen_y + adjust_top_size, size, font_size + 1);

			if (trackData->editText)
				Emgui_drawText(trackData->editText, x_pos, mid_screen_y + adjust_top_size, Emgui_color32(255, 255, 255, 255));
		}

		x_pos += size;
	}	

	Emgui_fill(Emgui_color32(127, 127, 127, 56), 0, mid_screen_y + adjust_top_size, viewInfo->windowSizeX, font_size + 1);
}

