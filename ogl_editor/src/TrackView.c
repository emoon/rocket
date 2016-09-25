#include "TrackView.h"
#include <emgui/Emgui.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "TrackData.h"
#include "rlog.h"
#include "minmax.h"
#include "ImageData.h"
#include "RenderAudio.h"
#include "../../lib/sync.h"
#include "../../lib/track.h"

#if defined(__APPLE__)
#include <OpenGL/OpenGL.h>
#include <OpenGL/gl.h>
#elif !defined(EMGUI_UNIX)
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
const uint32_t bookmark_color = EMGUI_COLOR32(0x3f, 0x2f, 0xaf, 0x7f);
const uint32_t loopmark_color = EMGUI_COLOR32(0x9f, 0x9f, 0x2f, 0x7f);
const uint32_t track_selection_color = EMGUI_COLOR32(0xff, 0xff, 0x00, 0xff);

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

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

struct DrawSelectedTrack
{
	int drawTrack;
	int width;
	int startX;
	int startY;
	int endY;
};

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static struct DrawSelectedTrack s_drawSelectedTrack;

extern void Dialog_showColorPicker(uint32_t* color);

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void TrackView_init()
{
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static void printRowNumbers(int x, int y, int rowCount, int rowOffset, int rowSpacing, int maskBpm, int endY)
{
	int i;

	Emgui_setDefaultFont();

	if (rowOffset < 0)
	{
		y += rowSpacing * -rowOffset;
		rowOffset = 0;
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
// TODO: Optimize (only one draw is needed)

static void printLoopmarks(int x, int y, int rowCount, int rowOffset, int rowSpacing, int maskBpm, int endY,
						   int loopStart, int loopEnd)
{
	int i;

	Emgui_setDefaultFont();

	if (rowOffset < 0)
	{
		y += rowSpacing * -rowOffset;
		rowOffset = 0;
	}

	for (i = 0; i < rowCount; ++i)
	{
		if (loopStart != -1 && loopEnd != -1)
		{
			if (rowOffset >= loopStart && rowOffset <= loopEnd)
				Emgui_fill(loopmark_color, x + 46, y, 2, rowSpacing);
		}

		y += rowSpacing;
		rowOffset++;

		if (y > endY)
			break;
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/*
static void drawBookmarks(TrackData* trackData, int x, int y, int rowCount, int rowOffset, int width, int endY)
{
	int i;

	if (rowOffset < 0)
	{
		y += 8 * -rowOffset;
		rowOffset = 0;
	}

	for (i = 0; i < rowCount; ++i)
	{
		if (rowOffset != 0)
		{
			if (TrackData_hasBookmark(trackData, rowOffset))
				Emgui_fill(bookmark_color, x, y, width, 8);
		}

		y += 8; rowOffset++;

		if (y > endY)
			break;
	}
}
*/

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

int Track_getSize(TrackViewInfo* viewInfo, Track* track)
{
	return getTrackSize(viewInfo, track);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

int getGroupSize(TrackViewInfo* viewInfo, Group* group, int startTrack)
{
	int i, size = 0, count = group->trackCount;

	if (group->width == 0)
	{
		if (group->name)
			group->width = (Emgui_getTextSize(group->name) & 0xffff) + 40;
		else
			group->width = (Emgui_getTextSize(group->tracks[0]->displayName) & 0xffff) + 40;
	}

	if (group->folded)
		return track_size_folded;

	if (!group->name)
		return getTrackSize(viewInfo, group->tracks[0]);

	for (i = startTrack; i < count; ++i)
		size += getTrackSize(viewInfo, group->tracks[i]);

	size = emaxi(size, group->width);

	return size;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static void drawSelectedTrackOutline(int width, int startX, int startY, int endY)
{
	int drawWidth = width / 4;

	// Draw top selection

	Emgui_fill(track_selection_color, startX, startY, drawWidth, 2);
	Emgui_fill(track_selection_color, (startX + width) - drawWidth, startY, drawWidth, 2);
	Emgui_fill(track_selection_color, startX, startY, 2, drawWidth);
	Emgui_fill(track_selection_color, startX + width, startY, 2, drawWidth);

	// Draw bottom selection

	Emgui_fill(track_selection_color, startX, endY, drawWidth, 2);
	Emgui_fill(track_selection_color, (startX + width) - drawWidth + 2, endY, drawWidth, 2);
	Emgui_fill(track_selection_color, startX, endY - drawWidth, 2, drawWidth);
	Emgui_fill(track_selection_color, startX + width, endY - drawWidth, 2, drawWidth);
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
	bool renderTrack = true;
	bool folded = false;
	const int yEnd = (info->endSizeY - info->startY) + 40;

	if (!valuesOnly)
	{
		drawFoldButton(startX + 6, info->startY - (font_size + 5), &trackData->folded, trackData->active);

		folded = trackData->folded;

		if (info->trackData->syncTracks)
			track = info->trackData->syncTracks[trackData->index];

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
				if (!info->trackData->isPlaying)
				{
					Dialog_showColorPicker(&trackData->color);

					if (trackData->color != color)
						s_needsUpdate = true;
				}
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

	// don't render anything if it's muted

	if (trackData->muteBackup)
		renderTrack = false;

	if (valuesOnly)
	{
		Emgui_fill(border_color, startX + size, info->startY - font_size * 4, 2, yEnd);
		Emgui_fill(border_color, startX, info->startY - font_size * 4, 2, yEnd);
	}
	else
	{
		Emgui_drawBorder(border_color, border_color, startX, info->startY - font_size * 4, size, yEnd);
	}

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

		if (renderTrack)
		{
			if (!(trackData->selected && info->viewInfo->rowPos == y && info->editText))
				renderText(info, track, y, idx, offset, y_offset, folded);
		}
		else
		{
			Emgui_fill(border_color , startX, y_offset - font_size_half, size, font_size);
		}

		selected = (trackIndex >= info->selectLeft && trackIndex <= info->selectRight) &&
			       (y >= info->selectTop && y <= info->selectBottom);

		if (selected)
			Emgui_fill(selection_color, startX, y_offset - font_size_half, size, font_size);

		if (y != 0)
		{
			bool overlapping = false;

			if (TrackData_hasBookmark(info->trackData, y))
			{
				overlapping = true;
				Emgui_fill(bookmark_color, startX, y_offset - font_size_half, size, 8);
			}

			if (TrackData_hasLoopmark(info->trackData, y))
			{
				if (!overlapping)
					Emgui_fill(loopmark_color , startX, y_offset - font_size_half, size, 8);
				else
					Emgui_fillStipple(loopmark_color , startX, y_offset - font_size_half, size, 8);
			}
		}

		y_offset += font_size;

		if (y_offset > (info->endSizeY + font_size_half))
			break;
	}

	if (!Emgui_hasKeyboardFocus())
	{
		if (trackData->selected)
		{
			if (!trackData->group->folded)
			{
				s_drawSelectedTrack.drawTrack = 1;
				s_drawSelectedTrack.width = size;
				s_drawSelectedTrack.startX = startX;
				s_drawSelectedTrack.startY = info->startY - font_size * 4;
				s_drawSelectedTrack.endY = info->endSizeY + 8;
			}

			Emgui_fill(trackData->group->folded ? dark_active_track_color : active_track_color, startX, info->midPos, size, font_size + 1);

			if (info->editText)
				Emgui_drawText(info->editText, startX + 2, info->midPos, Emgui_color32(255, 255, 255, 255));
		}
	}

	Emgui_setFont(info->viewInfo->smallFontId);

	return size;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static int renderGroup(Group* group, int posX, int* trackOffset, struct TrackInfo info, TrackData* trackData)
{
	int i, startTrackIndex = 0, size, track_count = group->trackCount;
	const int oldY = info.startY;
	const int windowSizeX = info.viewInfo->windowSizeX;

	drawFoldButton(posX + 6, oldY - (font_size + 5), &group->folded, true);

	Emgui_setFont(info.viewInfo->smallFontId);

	size = getGroupSize(info.viewInfo, group, 0);

	renderGroupHeader(group, posX, oldY, size, info.viewInfo->windowSizeX);

	info.startPos += 5;
	info.startY += 40;

	*trackOffset += (track_count - startTrackIndex);

	if (!group->folded)
	{
		for (i = startTrackIndex; i < track_count; ++i)
		{
			Track* t = group->tracks[i];
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
		renderChannel(&info, posX, group->tracks[0], true);
	}

	Emgui_setDefaultFont();

	return size;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// This function will figure out what the next track to be selected is when the trackView is scrolled.
//
// The way it does that is by looking at all the groups (which a linear, while tracks are not) and figure out the start
// visible track and the end visible track. It also check if the current track is the activeTrack and if it's hidden on
// the left or right side.
//
// This code actually does more work than needed (possible to just break out when we found on which side the track
// is hidden but this code should be fairly fast anyway.

int TrackView_getScrolledTrack(TrackViewInfo* viewInfo, TrackData* trackData, int activeTrack, int posX)
{
	int i, j, foundState = 0, group_count = trackData->groupCount;
	Track* activeTrackPtr = &trackData->tracks[activeTrack];
	const int window_size = viewInfo->windowSizeX - 80;

	posX -= 40;

	for (i = 0; i < group_count; ++i)
	{
		Group* group = &trackData->groups[i];
		int track_count = group->trackCount;
		const bool folded = group->folded;
		int t_pos;

		if (folded)
			track_count = 1;

		t_pos = posX;

		for (j = 0; j < track_count; ++j)
		{
			Track* currentTrack = group->tracks[j];
			const int track_size = folded ? track_size_folded : getTrackSize(viewInfo, currentTrack);

			// if we are on the current track we check where the current position is located
			// so if we are above the window limit wi

			if (currentTrack == activeTrackPtr)
			{
				if (t_pos < 0 && currentTrack == activeTrackPtr)
					foundState = -1;

				// if we are with in the limits we can just return directly here

				if (t_pos >= 0 && (t_pos + track_size) < window_size)
					return activeTrack;
			}

			// if we just crossed over to positive side and the activeTrack was found negative then the current track
			// should be the next active one

			if (t_pos >= 0 && foundState == -1)
				return currentTrack->index;

			// if we have crossed the page boundry and not yet has returned the track the last visible one
			// will be seleceted as the active one

			if (t_pos + track_size > window_size)
				return currentTrack->index;

            t_pos += track_size;
		}

		posX += getGroupSize(viewInfo, group, 0);
	}

	return activeTrack;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

bool TrackView_render(TrackViewInfo* viewInfo, TrackData* trackData)
{
	struct TrackInfo info;
	int group_count;
	int x_pos = 128;
	//int end_track = 0;
	int i = 0;
	//int track_size;
	int adjust_top_size;
	int mid_screen_y ;
	int y_pos_row, end_row, y_end_border;
	const int endLoop = TrackData_getNextLoopmark(trackData, viewInfo->rowPos);
	const int startLoop = TrackData_getPrevLoopmark(trackData, viewInfo->rowPos);

    if (trackData->rowsPerBeat == 0)
        trackData->rowsPerBeat = 1;

	s_needsUpdate = false;
	s_drawSelectedTrack.drawTrack = 0;

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

	x_pos = TrackView_getStartOffset() + -viewInfo->startPixel;

	printRowNumbers(2, adjust_top_size, end_row, y_pos_row, font_size, trackData->rowsPerBeat, y_end_border);

	Emgui_drawBorder(border_color, border_color, 48, info.startY - font_size * 4, viewInfo->windowSizeX - 80, (info.endSizeY - info.startY) + 40);

	Emgui_setLayer(1);
	Emgui_setScissor(48, 0, viewInfo->windowSizeX - 80, viewInfo->windowSizeY);
	Emgui_setFont(viewInfo->smallFontId);


	for (i = 0, group_count = trackData->groupCount; i < group_count; ++i)
	{
		Group* group = &trackData->groups[i];

		if (group->trackCount == 1 && !group->displayName)
		{
			Track* track = group->tracks[0];
			int track_size = getTrackSize(viewInfo, track);

			if ((x_pos + track_size > 0) && (x_pos < viewInfo->windowSizeX))
			{
				// if selected track is less than the first rendered track then we need to reset it to this one

				Emgui_setFont(info.viewInfo->smallFontId);
				renderChannel(&info, x_pos, track, false);
			}

			x_pos += track_size;
		}
		else
		{
			int temp = 0;
			x_pos += renderGroup(group, x_pos, &temp, info, trackData);
		}
	}

	if (s_drawSelectedTrack.drawTrack)
	{
		drawSelectedTrackOutline(
				s_drawSelectedTrack.width,
				s_drawSelectedTrack.startX,
				s_drawSelectedTrack.startY,
				s_drawSelectedTrack.endY);
	}

	printLoopmarks(2, adjust_top_size, end_row, y_pos_row, font_size, trackData->rowsPerBeat, y_end_border, startLoop, endLoop);

	// draw loop outline if we are with in the range

	/*
	if (startLoop != -1 && endLoop != -1)
	{
		int x = TrackView_getStartOffset() + -viewInfo->startPixel;
		int y = y_pos_row + (startLoop * font_size);
		int end_y = endLoop * font_size;

		Emgui_fill(loopmark_color, x + 2, y, 2, end_y);
	}
	*/

    RenderAudio_update(trackData, viewInfo->windowSizeX + 2, adjust_top_size, end_row, y_pos_row, font_size, y_end_border);

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
		size += getGroupSize(viewInfo, group, 0);
	}

	return size;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

int TrackView_getStartOffset()
{
	return 48;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

bool TrackView_isSelectedTrackVisible(TrackViewInfo* viewInfo, TrackData* trackData, int track)
{
	return TrackView_getScrolledTrack(viewInfo, trackData, track, TrackView_getStartOffset() - viewInfo->startPixel) == track;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static int getTrackOffset(TrackViewInfo* viewInfo, TrackData* trackData, int track)
{
	int i = 0, j = 0;
	int size = 0;

	for (i = 0; i < trackData->groupCount; ++i)
	{
		Group* g = &trackData->groups[i];
		const int trackCount = g->trackCount;

		if (g->folded)
			size += track_size_folded;

		for (j = 0; j < trackCount; ++j)
		{
			Track* t = g->tracks[j];

			if (!g->folded)
				size += getTrackSize(viewInfo, t);

			if (t->index == track)
				return size;
		}
	}

	return size;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

int TrackView_getTracksOffset(TrackViewInfo* viewInfo, TrackData* trackData, int prevTrack, int nextTrack)
{
	int prevOffset, nextOffset;

	if (prevTrack == nextTrack)
		return 0;

	prevOffset = getTrackOffset(viewInfo, trackData, prevTrack);
	nextOffset = getTrackOffset(viewInfo, trackData, nextTrack);

	return nextOffset - prevOffset;
}
