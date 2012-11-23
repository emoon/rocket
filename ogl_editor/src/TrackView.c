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

#define font_size 8
int track_size_folded = 20;
int min_track_size = 100;
int name_adjust = font_size * 2;
int font_size_half = font_size / 2;
int colorbar_adjust = ((font_size * 3) + 2);

static bool s_needsUpdate = false;

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
	int midPos;
};

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

static void drawFoldButton(int x, int y, bool* fold)
{
	bool old_state = *fold;

	Emgui_radioButtonImage(g_arrow_left_png, g_arrow_left_png_len, g_arrow_right_png, g_arrow_right_png_len,
							EMGUI_LOCATION_MEMORY, Emgui_color32(255, 255, 255, 255), x, y, fold);

	s_needsUpdate = old_state != *fold ? true : s_needsUpdate;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static int renderName(const char* name, int x, int y, int minSize, bool folded)
{
	int size = min_track_size;
	int text_size;
	int x_adjust = 0;
	int spacing = 30;

	text_size = (Emgui_getTextSize(name) & 0xffff) + spacing;

	if (text_size < minSize) 
		x_adjust = (minSize - text_size) / 2;
	else
		size = text_size + 1; 

	if (folded)
	{
		Emgui_drawTextFlipped(name, x, y + text_size, Emgui_color32(0xff, 0xff, 0xff, 0xff));
		size = text_size;
	}
	else
	{
		Emgui_drawText(name, x + x_adjust + 16, y, Emgui_color32(0xff, 0xff, 0xff, 0xff));
	}

	return size;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static int renderGroupHeader(Group* group, int x, int y, int groupSize, int windowSizeX)
{
	// If the group size is larger than the actual screen we adjust it a bit to make the text more visible

	if (x + groupSize > windowSizeX)
		groupSize = windowSizeX - x;

	drawColorButton(Emgui_color32(127, 127, 127, 255), x + 3, y - colorbar_adjust, groupSize);
	renderName(group->displayName, x, y - name_adjust, groupSize, group->folded);

	return 0;
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

static int findStartTrack(Group* group, Track* startTrack)
{
	int i, track_count = group->trackCount;

	for (i = 0; i < track_count; ++i)
	{
		if (group->t.tracks[i] == startTrack)
			return i;
	}

	return 0;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static int getTrackSize(TrackData* trackData, Track* track)
{
	int size = 0;

	if (track->folded)
		return track_size_folded; 

	size = (Emgui_getTextSize(track->displayName) & 0xffff) + 31; 
	size = emaxi(size, min_track_size);

	return size;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

int getGroupSize(TrackData* trackData, Group* group, int startTrack)
{
	int i, size = 0, count = group->trackCount;
	int text_size = (Emgui_getTextSize(group->name) & 0xffff) + 40;

	if (group->folded)
		return track_size_folded; 

	for (i = startTrack; i < count; ++i)
		size += getTrackSize(trackData, group->t.tracks[i]);

	size = emaxi(size, text_size);

	return size;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static int renderChannel(struct TrackInfo* info, int startX, int editRow, Track* trackData, bool valuesOnly)
{
	int y, y_offset;
	int text_size = 0;
	int size = min_track_size;
	int startPos = info->startPos;
	const int endPos = info->endPos;
	uint32_t borderColor = Emgui_color32(40, 40, 40, 255);
	struct sync_track* track = 0;
	const uint32_t color = trackData->color;
	bool folded;

	if (!valuesOnly)
	{
		drawFoldButton(startX + 6, info->startY - (font_size + 5), &trackData->folded);

		folded = trackData->folded;
		
		if (info->trackData->syncData.tracks)
			track = info->trackData->syncData.tracks[trackData->index];

		Emgui_setFont(info->viewInfo->smallFontId);

		if (trackData->displayName)
			size = renderName(trackData->displayName, startX, info->startY - (font_size * 2), min_track_size, trackData->folded);
		else
			size = renderName(track->name, startX, info->startY - (font_size * 2), min_track_size, trackData->folded);

		if (folded)
		{
			text_size = size;
			size = track_size_folded;
		}

		Emgui_drawBorder(borderColor, borderColor, startX, info->startY - font_size * 4, size, (info->endSizeY - info->startY) + 40);

		if (drawColorButton(color, startX + 4, info->startY - colorbar_adjust, size))
		{
			printf("Yah!\n");
		}
	}

	Emgui_setDefaultFont();

	y_offset = info->startY;

	folded = valuesOnly ? true : folded;
	size = valuesOnly ? track_size_folded : size;

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
		// bool selected;

		if (track)
			idx = sync_find_key(track, y);

		renderInterpolation(info, track, size, idx, offset, y_offset, folded);
		renderText(info, track, y, idx, offset, y_offset, y == editRow, folded);

		//selected = (trackIndex >= info->selectLeft && trackIndex <= info->selectRight) && 
		//	       (y >= info->selectTop && y < info->selectBottom);

		//if (selected)
		//	Emgui_fill(Emgui_color32(0x4f, 0x4f, 0x4f, 0x3f), startX, y_offset - font_size_half, size, font_size);  

		y_offset += font_size;

		if (y_offset > (info->endSizeY + font_size_half))
			break;
	}

	if (trackData->selected)
	{
		Emgui_fill(Emgui_color32(0xff, 0xff, 0x00, 0x80), startX, info->midPos, size, font_size + 1);
		//if (trackData->editText)
		//	Emgui_drawText(trackData->editText, x_pos, info->midPos, Emgui_color32(255, 255, 255, 255));
	}

	Emgui_setFont(info->viewInfo->smallFontId);

	return size;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static int renderGroup(Group* group, Track* startTrack, int posX, int* trackOffset, struct TrackInfo info, TrackData* trackData)
{
	int i, startTrackIndex = 0, size, track_count = group->trackCount;
	const int oldY = info.startY;
	const int windowSizeX = info.viewInfo->windowSizeX;

	drawFoldButton(posX + 6, oldY - (font_size + 5), &group->folded);

	Emgui_setFont(info.viewInfo->smallFontId);

	startTrackIndex = findStartTrack(group, startTrack);

	size = getGroupSize(trackData, group, startTrackIndex);

	printf("size %d\n", size);

	// TODO: Draw the group name and such here

	renderGroupHeader(group, posX, oldY, size, info.viewInfo->windowSizeX);

	info.startPos += 5;
	info.startY += 40;

	*trackOffset += (track_count - startTrackIndex);

	if (!group->folded)
	{
		for (i = startTrackIndex; i < track_count; ++i)
		{
			Track* t = group->t.tracks[i];
			posX += renderChannel(&info, posX, -1, t, false);

			if (posX >= windowSizeX)
			{
				if (trackData->activeTrack >= i)
					info.viewInfo->startTrack++;

				return size;
			}
		}
	}
	else
	{
		renderChannel(&info, posX, -1, group->t.tracks[0], true);
	}

	Emgui_setDefaultFont();

	return size;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void renderGroups(TrackViewInfo* viewInfo, TrackData* trackData)
{
	struct TrackInfo info;
	const int sel_track = trackData->activeTrack;
	int start_track = viewInfo->startTrack;
	int x_pos = 40;
	int end_track = 0;
	int i = 0;
	int track_size;
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
	info.midPos = mid_screen_y + adjust_top_size;

	if (trackData->groupCount == 0)
	{
		uint32_t color = Emgui_color32(127, 127, 127, 56);
		//renderChannel(&info, x_pos, 0, 0);
		Emgui_fill(color, 0, mid_screen_y + adjust_top_size, viewInfo->windowSizeX, font_size + 2);
		return;
	}

	for (i = start_track, end_track = (int)trackData->syncData.num_tracks; i < end_track; )
	{
		Track* track = &trackData->tracks[i];
		Group* group = track->group; 
		track_size = getTrackSize(trackData, track);

		if (x_pos + track_size >= viewInfo->windowSizeX)
		{
			if (sel_track >= i)
				viewInfo->startTrack++;

			break;
		}
		
		if (group->trackCount == 1)
		{
			x_pos += renderChannel(&info, x_pos, -1, track, false); ++i;
		}
		else
		{
			x_pos += renderGroup(group, track, x_pos, &i, info, trackData);
		}
	}

	if (sel_track < start_track)
		viewInfo->startTrack = emaxi(viewInfo->startTrack - 1, 0);

	Emgui_fill(Emgui_color32(127, 127, 127, 56), 0, mid_screen_y + adjust_top_size, viewInfo->windowSizeX, font_size + 1);
	/*
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
	*/

}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

bool TrackView_render(TrackViewInfo* viewInfo, TrackData* trackData)
{
	s_needsUpdate = false;

	renderGroups(viewInfo, trackData);

	return s_needsUpdate;

	/*
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
	*/
}

