#include "trackview.h"
#include <Emgui.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "TrackData.h"
#include "rlog.h"
#include "minmax.h"
#include "ImageData.h"
#include "../../sync/sync.h"
#include "../../sync/data.h"
#include "../../sync/track.h"

const int font_size = 8;
const int font_size_half = font_size / 2;
const int track_size_folded = 20;
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

static bool drawColorButton(uint32_t color, int x, int y, int size)
{
	const uint32_t colorFade = Emgui_color32(32, 32, 32, 255);
	Emgui_fill(color, x, y, size - 8, 8); 

	return Emgui_buttonCoords("", colorFade, x, y, size - 8, 8);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

struct TrackInfo
{
	TrackViewInfo* viewInfo;
	TrackData* trackData;
	int selectLeft;
	int selectRight;
	int selectTop;
	int selectBottom;
	int startY;
	int startPos;
	int endPos; 
	int endSizeY;
};

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static int renderTrackName(const struct TrackInfo* info, struct sync_track* track, int startX, bool folded)
{
	int size = min_track_size;
	int text_size;
	int x_adjust = 0;
	int spacing = folded ? 0 : 30;

	if (!track)
		return folded ? 1 : size;

	Emgui_setFont(info->viewInfo->smallFontId);
	text_size = (Emgui_getTextSize(track->name) & 0xffff) + spacing;

	// if text is smaller than min size we center the text

	if (text_size < min_track_size) 
		x_adjust = (min_track_size - text_size) / 2;
	else
		size = text_size + 1; 

	if (folded)
	{
		Emgui_drawTextFlipped(track->name, (startX + 3), info->startY + text_size, Emgui_color32(0xff, 0xff, 0xff, 0xff));
		size = text_size;
	}
	else
	{
		Emgui_drawText(track->name, (startX + 3) + x_adjust + 16, info->startY - font_size * 2, Emgui_color32(0xff, 0xff, 0xff, 0xff));
	}

	Emgui_setDefaultFont();

	return size;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static void renderInterpolation(const struct TrackInfo* info, struct sync_track* track, int size, int idx, int x, int y, bool folded) 
{
	int fidx;
	enum key_type interpolationType;
	uint32_t color = 0;

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

	if (info->viewInfo)
		Emgui_fill(color, x + (size - 8), y - font_size_half, 2, (info->endSizeY - y) + font_size - 1);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static void renderText(const struct TrackInfo* info, struct sync_track* track, int row, int idx, int x, int y, bool editRow, bool folded)
{
	uint32_t color = (row & 7) ? Emgui_color32(0x4f, 0x4f, 0x4f, 0xff) : Emgui_color32(0x7f, 0x7f, 0x7f, 0xff); 

	if (folded)
	{
		if (idx >= 0)
			Emgui_fill(color, x, y - font_size_half, 8, 8);
		else
			Emgui_drawText("-", x, y - font_size_half, color); 
	}
	else
	{
		if (idx >= 0)
		{
			char temp[256];
			float value = track->keys[idx].value;
			snprintf(temp, 256, "% .2f", value);

			if (!editRow)
				Emgui_drawText(temp, x, y - font_size_half, Emgui_color32(255, 255, 255, 255));
		}
		else
		{
			Emgui_drawText("---", x, y - font_size_half, color); 
		}
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static int renderChannel(struct TrackInfo* info, int startX, int editRow, int trackIndex)
{
	int y, y_offset;
	int text_size = 0;
	int size = min_track_size;
	int startPos = info->startPos;
	const int endPos = info->endPos;
	uint32_t borderColor = Emgui_color32(40, 40, 40, 255);
	struct sync_track* track = 0;
	const uint32_t color = info->trackData->tracks[trackIndex].color;
	bool folded;

	Emgui_radioButtonImage(g_arrow_left_png, g_arrow_left_png_len,
						   g_arrow_right_png, g_arrow_right_png_len,
						   EMGUI_LOCATION_MEMORY, Emgui_color32(255, 255, 255, 255), 
						   startX + 6, info->startY - (font_size + 5),
						   &info->trackData->tracks[trackIndex].folded);

	folded = info->trackData->tracks[trackIndex].folded;
	
	if (info->trackData->syncData.tracks)
		track = info->trackData->syncData.tracks[trackIndex];

	size = renderTrackName(info, track, startX, folded);

	if (folded)
	{
		text_size = size;
		size = track_size_folded;
	}

	Emgui_drawBorder(borderColor, borderColor, startX, info->startY - font_size * 4, size, info->endSizeY);

	if (drawColorButton(color, startX + 4, info->startY - ((font_size * 3) + 2), size))
	{
		printf("Yah!\n");
	}

	y_offset = info->startY;

	// if folded we should skip rendering the rows that are covered by the text

	if (folded)
	{
		int skip_rows = (text_size + font_size * 2) / font_size;

		if (startPos + skip_rows > 0)
		{
			startPos += skip_rows;
			y_offset += skip_rows * font_size;
		}
	}

	if (startPos < 0)
	{
		y_offset = info->startY + (font_size * -startPos);
		startPos = 0;
	}

	y_offset += font_size_half;

	for (y = startPos; y < endPos; ++y)
	{
		int idx = -1;
		int offset = startX + 6;
		bool selected;

		if (track)
			idx = sync_find_key(track, y);

		renderInterpolation(info, track, size, idx, offset, y_offset, folded);
		renderText(info, track, y, idx, offset, y_offset, y == editRow, folded);

		selected = (trackIndex >= info->selectLeft && trackIndex <= info->selectRight) && 
			       (y >= info->selectTop && y < info->selectBottom);

		if (selected)
			Emgui_fill(Emgui_color32(0x4f, 0x4f, 0x4f, 0x3f), startX, y_offset - font_size_half, size, font_size);  

		y_offset += font_size;

		if (y_offset > (info->endSizeY + font_size_half))
			break;
	}

	return size;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void TrackView_render(TrackViewInfo* viewInfo, TrackData* trackData)
{
	struct TrackInfo info;
	struct sync_data* syncData = &trackData->syncData;
	const int sel_track = trackData->activeTrack;
	//uint32_t color = Emgui_color32(127, 127, 127, 56);
	int start_track = viewInfo->startTrack;
	int x_pos = 40;
	int end_track = 0;
	int i = 0;
	int adjust_top_size;
	int mid_screen_y ;
	int y_pos_row, end_row, y_end_border;

	// Calc to position the selection in the ~middle of the screen 

	adjust_top_size = 5 * font_size;
	mid_screen_y = (viewInfo->windowSizeY / 2) & ~(font_size - 1);
	y_pos_row = viewInfo->rowPos - (mid_screen_y / font_size);

	// TODO: Calculate how many channels we can draw given the width

	end_row = viewInfo->windowSizeY / font_size;
	y_end_border = viewInfo->windowSizeY - 32; // adjust to have some space at the end of the screen

	printRowNumbers(2, adjust_top_size, end_row, y_pos_row, font_size, 8, y_end_border);

	// Shared info for all tracks

	info.selectLeft = emini(viewInfo->selectStartTrack, viewInfo->selectStopTrack);
	info.selectRight = emaxi(viewInfo->selectStartTrack, viewInfo->selectStopTrack);
	info.selectTop  = emini(viewInfo->selectStartRow, viewInfo->selectStopRow);
	info.selectBottom = emaxi(viewInfo->selectStartRow, viewInfo->selectStopRow);
	info.viewInfo = viewInfo;
	info.trackData = trackData;
	info.startY = adjust_top_size;
	info.startPos = y_pos_row;
	info.endPos = y_pos_row + end_row; 
	info.endSizeY = y_end_border;

	if (syncData->num_tracks == 0)
	{
		uint32_t color = Emgui_color32(127, 127, 127, 56);
		renderChannel(&info, x_pos, 0, 0);
		Emgui_fill(color, 0, mid_screen_y + adjust_top_size, viewInfo->windowSizeX, font_size + 2);
		return;
	}

	end_track = syncData->num_tracks;

	for (i = start_track; i < end_track; ++i)
	{
		int size, editRow = -1;

		if (sel_track == i && trackData->editText)
			editRow = viewInfo->rowPos;

		size = renderChannel(&info, x_pos, editRow, i);

		if (!Emgui_hasKeyboardFocus())
		{
			if (sel_track == i)
			{
				Emgui_fill(Emgui_color32(0xff, 0xff, 0x00, 0x80), x_pos, mid_screen_y + adjust_top_size, size, font_size + 1);

				if (trackData->editText)
					Emgui_drawText(trackData->editText, x_pos, mid_screen_y + adjust_top_size, Emgui_color32(255, 255, 255, 255));
			}
		}
		else
		{
			if (sel_track == i)
			{
				Emgui_fill(Emgui_color32(0x7f, 0x7f, 0x7f, 0x80), x_pos, mid_screen_y + adjust_top_size, size, font_size + 1);
			}
		}

		x_pos += size;

		if (x_pos >= viewInfo->windowSizeX)
		{
			if (sel_track + 1 >= i)
				viewInfo->startTrack++;

			break;
		}
	}	

	if (sel_track < start_track)
		viewInfo->startTrack = emaxi(viewInfo->startTrack - 1, 0);

	Emgui_fill(Emgui_color32(127, 127, 127, 56), 0, mid_screen_y + adjust_top_size, viewInfo->windowSizeX, font_size + 1);
}

