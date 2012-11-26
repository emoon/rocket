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

#if defined(__APPLE__)
#include <OpenGL/OpenGL.h>
#include <OpenGL/gl.h>
#else
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <gl/gl.h>
#endif

#define font_size 8
int track_size_folded = 20;
int min_track_size = 100;
int name_adjust = font_size * 2;
int font_size_half = font_size / 2;
int colorbar_adjust = ((font_size * 3) + 2);

// Colors

const uint32_t active_track_color = EMGUI_COLOR32(0x5f, 0x6f, 0x40, 0x80);
const uint32_t dark_active_track_color = EMGUI_COLOR32(0xaf, 0x1f, 0x10, 0x80);
const uint32_t active_text_color = EMGUI_COLOR32(0xff, 0xff, 0xff, 0xff);
const uint32_t inactive_text_color = EMGUI_COLOR32(0x5f, 0x5f, 0x5f, 0xff);
const uint32_t border_color = EMGUI_COLOR32(40, 40, 40, 255);
const uint32_t selection_color = EMGUI_COLOR32(0x5f, 0x5f, 0x5f, 0x4f);

static bool s_needsUpdate = false;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

struct TrackInfo
{
	TrackViewInfo* viewInfo;
	TrackData* trackData;
	char* editText;
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

extern void Dialog_showColorPicker(uint32_t* color);

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
		sprintf(rowText, "%05d", rowOffset);

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

static void drawFoldButton(int x, int y, bool* fold, bool active)
{
	bool old_state = *fold;
	uint32_t color = active ? active_text_color : inactive_text_color;

	Emgui_radioButtonImage(g_arrow_left_png, g_arrow_left_png_len, g_arrow_right_png, g_arrow_right_png_len,
							EMGUI_LOCATION_MEMORY, color, x, y, fold);

	s_needsUpdate = old_state != *fold ? true : s_needsUpdate;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static int renderName(const char* name, int x, int y, int minSize, bool folded, bool active)
{
	int size = min_track_size;
	int text_size_full;
	int text_size;
	int x_adjust = 0;
	int spacing = 30;
	const uint32_t color = active ? active_text_color : inactive_text_color;

	text_size_full = Emgui_getTextSize(name);
	text_size = (text_size_full & 0xffff) + spacing;

	if (text_size < minSize) 
		x_adjust = (minSize - text_size) / 2;
	else
		size = text_size + 1; 

	if (folded)
	{
		Emgui_drawTextFlipped(name, x + 4, y + text_size - 10, color);
		size = text_size - spacing;
	}
	else
	{
		Emgui_drawText(name, x + x_adjust + 16, y, color);
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
	renderName(group->displayName, x, y - name_adjust, groupSize, group->folded, true);

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

static void renderText(const struct TrackInfo* info, struct sync_track* track, int row, int idx, int x, int y, bool folded)
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

static int getTrackSize(TrackViewInfo* viewInfo, Track* track)
{
	if (track->folded)
		return track_size_folded; 

	if (track->width == 0)
	{
		Emgui_setFont(viewInfo->smallFontId);
		track->width = (Emgui_getTextSize(track->displayName) & 0xffff) + 31; 
		track->width = emaxi(track->width, min_track_size);
	}

	return track->width;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

int getGroupSize(TrackViewInfo* viewInfo, Group* group, int startTrack)
{
	int i, size = 0, count = group->trackCount;

	if (group->width == 0)
		group->width = (Emgui_getTextSize(group->name) & 0xffff) + 40;

	if (group->folded)
		return track_size_folded; 

	for (i = startTrack; i < count; ++i)
		size += getTrackSize(viewInfo, group->t.tracks[i]);

	size = emaxi(size, group->width);

	return size;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static int renderChannel(struct TrackInfo* info, int startX, Track* trackData, bool valuesOnly)
{
	int y, y_offset;
	int text_size = 0;
	int size = min_track_size;
	int startPos = info->startPos;
	const int trackIndex = trackData->index;
	const int endPos = info->endPos;
	struct sync_track* track = 0;
	const uint32_t color = trackData->color;
	bool folded;

	if (!valuesOnly)
	{
		drawFoldButton(startX + 6, info->startY - (font_size + 5), &trackData->folded, trackData->active);

		folded = trackData->folded;
		
		if (info->trackData->syncData.tracks)
			track = info->trackData->syncData.tracks[trackData->index];


		size = renderName(trackData->displayName, startX, info->startY - (font_size * 2), min_track_size, folded, trackData->active);

		if (folded)
		{
			text_size = size;
			size = track_size_folded;
		}

		if (trackData->active)
		{
			if (drawColorButton(color, startX + 4, info->startY - colorbar_adjust, size))
			{
				Dialog_showColorPicker(&trackData->color);

				if (trackData->color != color)
					s_needsUpdate = true;
			}
		}
		else
		{
			Emgui_fill(border_color, startX + 4, info->startY - colorbar_adjust, size - 8, 8);
		}
	}

	Emgui_setDefaultFont();

	y_offset = info->startY;

	folded = valuesOnly ? true : folded;
	size = valuesOnly ? track_size_folded : size;

	Emgui_fill(border_color, startX + size, info->startY - font_size * 4, 2, (info->endSizeY - info->startY) + 40);

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

		if (!(trackData->selected && info->viewInfo->rowPos == y && info->editText))
			renderText(info, track, y, idx, offset, y_offset, folded);

		selected = (trackIndex >= info->selectLeft && trackIndex <= info->selectRight) && 
			       (y >= info->selectTop && y < info->selectBottom);

		if (selected)
			Emgui_fill(selection_color, startX, y_offset - font_size_half, size, font_size);  

		y_offset += font_size;

		if (y_offset > (info->endSizeY + font_size_half))
			break;
	}

	if (!Emgui_hasKeyboardFocus())
	{
		if (trackData->selected)
		{
			Emgui_fill(trackData->group->folded ? dark_active_track_color : active_track_color, startX, info->midPos, size, font_size + 1);

			if (info->editText)
				Emgui_drawText(info->editText, startX + 2, info->midPos, Emgui_color32(255, 255, 255, 255));
		}
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

	drawFoldButton(posX + 6, oldY - (font_size + 5), &group->folded, true);

	Emgui_setFont(info.viewInfo->smallFontId);

	startTrackIndex = findStartTrack(group, startTrack);

	size = getGroupSize(info.viewInfo, group, startTrackIndex);

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
			posX += renderChannel(&info, posX, t, false);

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
		renderChannel(&info, posX, group->t.tracks[0], true);
	}

	Emgui_setDefaultFont();

	return size;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static int processTrack(Track* track, int posX, int* startTrack, int* endTrack, TrackViewInfo* viewInfo)
{
	int track_size = getTrackSize(viewInfo, track);

	if (posX > 0 && *startTrack == -1)
		*startTrack = track->index;

	if (((posX + 50 + track_size) > (viewInfo->windowSizeX - 80)) && *endTrack == -1)
		*endTrack = track->index;

	return track_size;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static int setActiveTrack(TrackViewInfo* viewInfo, TrackData* trackData, int activeTrack, int posX)
{
	int i, j,  track_count = trackData->syncData.num_tracks;
	int start_track = -1, end_track = -1;

	posX -= 40;

	for (i = 0; i < track_count; )
	{
		Track* track = &trackData->tracks[i];
		Group* group = track->group; 

		if (group->trackCount == 1)
		{
			posX += processTrack(track, posX, &start_track, &end_track, viewInfo);
		}
		else
		{
			if (group->folded)
			{
				posX += processTrack(group->t.tracks[0], posX, &start_track, &end_track, viewInfo);
			}
			else
			{
				for (j = 0; j < group->trackCount; ++j)
					posX += processTrack(group->t.tracks[j], posX, &start_track, &end_track, viewInfo);
			}
		}

		i += group->trackCount;
	}

	if (activeTrack > start_track && activeTrack < end_track)
	{
		if (activeTrack == -1)
			__asm__ ("int $3");
		return activeTrack;
	}

	if (activeTrack < start_track)
	{
		if (start_track == -1)
			__asm__ ("int $3");

		return start_track;
	}

	if (activeTrack > end_track)
	{
		if (end_track == -1)
			__asm__ ("int $3");
		return end_track;
	}

	return activeTrack;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

bool TrackView_render(TrackViewInfo* viewInfo, TrackData* trackData)
{
	struct TrackInfo info;
	int sel_track = 0; //trackData->activeTrack;
	int start_track = 0; //viewInfo->startTrack;
	int x_pos = 128;
	int end_track = 0;
	int i = 0;
	//int track_size;
	int adjust_top_size;
	int mid_screen_y ;
	int y_pos_row, end_row, y_end_border;

	s_needsUpdate = false;

	// Calc to position the selection in the ~middle of the screen 

	adjust_top_size = 5 * font_size;
	mid_screen_y = (viewInfo->windowSizeY / 2) & ~(font_size - 1);
	y_pos_row = viewInfo->rowPos - (mid_screen_y / font_size);

	// TODO: Calculate how many channels we can draw given the width

	end_row = viewInfo->windowSizeY / font_size;
	y_end_border = viewInfo->windowSizeY - 48; // adjust to have some space at the end of the screen

	// Shared info for all tracks

	info.editText = trackData->editText;
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
		return false;

	x_pos = 50 + -viewInfo->startPixel;

	printRowNumbers(2, adjust_top_size, end_row, y_pos_row, font_size, 8, y_end_border);
	Emgui_drawBorder(border_color, border_color, 48, info.startY - font_size * 4, viewInfo->windowSizeX - 80, (info.endSizeY - info.startY) + 40);

	Emgui_setLayer(1);
	Emgui_setScissor(48, 0, viewInfo->windowSizeX - 80, viewInfo->windowSizeY);
	Emgui_setFont(viewInfo->smallFontId);

	sel_track = setActiveTrack(viewInfo, trackData, trackData->activeTrack, x_pos);

	if (sel_track != trackData->activeTrack)
		TrackData_setActiveTrack(trackData, sel_track);

	for (i = start_track, end_track = trackData->syncData.num_tracks; i < end_track; )
	{
		Track* track = &trackData->tracks[i];
		Group* group = track->group; 

		if (group->trackCount == 1)
		{
			int track_size = getTrackSize(viewInfo, track);
		
			if ((x_pos + track_size > 0) && (x_pos < viewInfo->windowSizeX))
			{
				// if selected track is less than the first rendered track then we need to reset it to this one

				renderChannel(&info, x_pos, track, false);
			}

			x_pos += track_size;
			++i;
		}
		else
		{
			x_pos += renderGroup(group, track, x_pos, &i, info, trackData);
		}
	}

	Emgui_setDefaultFont();

	Emgui_fill(Emgui_color32(127, 127, 127, 56), 0, mid_screen_y + adjust_top_size, viewInfo->windowSizeX, font_size + 1);

	Emgui_setLayer(0);

	return s_needsUpdate;
}


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

int TrackView_getWidth(TrackViewInfo* viewInfo, struct TrackData* trackData)
{
	int i, size = 0, group_count = trackData->groupCount;

	if (trackData->groupCount == 0)
		return 0;

	for (i = 0; i < group_count; ++i)
	{
		Group* group = &trackData->groups[i];

		if (group->trackCount == 1)
			size += getTrackSize(viewInfo, group->t.track);
		else
			size += getGroupSize(viewInfo, group, 0);
	}

	return size;
}

