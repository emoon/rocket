#include <string.h>
#include <stdlib.h>
#include <Emgui.h>
#include <stdio.h>
#include <math.h>
#include "Dialog.h"
#include "Editor.h"
#include "LoadSave.h"
#include "TrackView.h"
#include "rlog.h"
#include "minmax.h"
#include "TrackData.h"
#include "RemoteConnection.h"
#include "Commands.h"
#include "MinecraftiaFont.h"
#include "../../sync/sync.h"
#include "../../sync/base.h"
#include "../../sync/data.h"

extern void Window_setTitle(const char* title);
extern void Window_populateRecentList(const char** files);
static void updateNeedsSaving();

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

typedef struct CopyEntry
{
	int track;
	struct track_key keyFrame;
} CopyEntry;

typedef struct CopyData
{
	CopyEntry* entries;
	int bufferWidth;
	int bufferHeight;
	int count;

} CopyData;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

typedef struct EditorData
{
	TrackViewInfo trackViewInfo;
	TrackData trackData;
	CopyEntry* copyEntries;
	int copyCount;
} EditorData;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static EditorData s_editorData;
static CopyData s_copyData;
static int s_undoLevel = 0;
static bool reset_tracks = true;
static char s_filenames[5][2048];
static char* s_loadedFilename = 0;

static char* s_recentFiles[] =
{
	s_filenames[0],
	s_filenames[1],
	s_filenames[2],
	s_filenames[3],
	s_filenames[4],
};

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

char** Editor_getRecentFiles()
{
	return (char**)s_recentFiles;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

const char* getMostRecentFile()
{
	return s_recentFiles[0];
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void setMostRecentFile(const char* filename)
{
	int i;

	// move down all files
	for (i = 3; i >= 0; --i)
		strcpy(s_recentFiles[i+1], s_recentFiles[i]);

	strcpy(s_recentFiles[0], filename);
	s_loadedFilename = s_recentFiles[0];

	// check if the string was already present and remove it if that is the case by compacting the array
	
	for (i = 1; i < 5; ++i)
	{
		if (!strcmp(s_recentFiles[i], filename))
		{
			for (; i < 4; ++i)
				strcpy(s_recentFiles[i], s_recentFiles[i + 1]);

			break;
		}
	}

	Window_populateRecentList((const char**)s_recentFiles);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static inline struct sync_track** getTracks()
{
	return s_editorData.trackData.syncData.tracks;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static inline TrackViewInfo* getTrackViewInfo()
{
	return &s_editorData.trackViewInfo;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static inline TrackData* getTrackData()
{
	return &s_editorData.trackData;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static inline void setActiveTrack(int track)
{
	TrackData_setActiveTrack(getTrackData(), track);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static inline int getActiveTrack()
{
	return s_editorData.trackData.activeTrack;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static inline int getTrackCount()
{
	return s_editorData.trackData.syncData.num_tracks;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static inline int getRowPos()
{
	return s_editorData.trackViewInfo.rowPos;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static int getNextTrack()
{
	TrackData* trackData = &s_editorData.trackData;
	int i, track_count = getTrackCount(); 
	int active_track = getActiveTrack();
 
	for (i = active_track + 1; i < track_count; ++i)
	{
		Track* t = &trackData->tracks[i];

		// if track has no group its always safe to assume that can select the track
 
		if (!t->group)
			return i;
 
		if (!t->group->folded)
			return i;

		// if the track is the first in the group (and group is folded) we use that as the display track

		if (t->group->t.tracks[0] == t)
			return i;
	}
 
	return active_track;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static int getPrevTrack()
{
	TrackData* trackData = &s_editorData.trackData;
	int i, active_track = getActiveTrack();
 
	for (i = active_track - 1; i >= 0; --i)
	{
		Track* t = &trackData->tracks[i];

		// if track has no group its always safe to assume that can select the track
 
		if (!trackData->tracks[i].group)
			return i;
 
		if (!trackData->tracks[i].group->folded)
			return i;

		// if the track is the first in the group (and group is folded) we use that as the display track

		if (t->group->t.tracks[0] == t)
			return i;
	}
 
	return active_track;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void Editor_create()
{
	int id;
	Emgui_create("foo");
	id = Emgui_loadFontBitmap(g_minecraftiaFont, g_minecraftiaFontSize, EMGUI_LOCATION_MEMORY, 32, 128, g_minecraftiaFontLayout);

	RemoteConnection_createListner();

	s_editorData.trackViewInfo.smallFontId = id;
	s_editorData.trackData.startRow = 0;
	s_editorData.trackData.endRow = 10000;

	Emgui_setDefaultFont();
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void Editor_setWindowSize(int x, int y)
{
	s_editorData.trackViewInfo.windowSizeX = x;
	s_editorData.trackViewInfo.windowSizeY = y;
	Editor_updateTrackScroll();
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void Editor_init()
{
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static char s_currentRow[64] = "0";
static char s_startRow[64] = "0";
static char s_endRow[64] = "10000";

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static int drawConnectionStatus(int posX, int sizeY)
{
	char* conStatus;

	RemoteConnection_getConnectionStatus(&conStatus);

	Emgui_drawBorder(Emgui_color32(10, 10, 10, 255), Emgui_color32(10, 10, 10, 255), posX, sizeY - 17, 200, 15); 
	Emgui_drawText(conStatus, posX + 4, sizeY - 15, Emgui_color32(160, 160, 160, 255));

	return posX + 200;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static int drawCurrentValue(int posX, int sizeY)
{
	char valueText[256];
	float value = 0.0f; 
	int active_track = 0;
	int current_row = 0;
	const char *str = "---";
	struct sync_track** tracks = getTracks();

	active_track = getActiveTrack();
	current_row = getRowPos();

	if (tracks)
	{
		const struct sync_track* track = tracks[active_track];
		int idx = key_idx_floor(track, current_row);

		if (idx >= 0) 
		{
			switch (track->keys[idx].type) 
			{
				case KEY_STEP:   str = "step"; break;
				case KEY_LINEAR: str = "linear"; break;
				case KEY_SMOOTH: str = "smooth"; break;
				case KEY_RAMP:   str = "ramp"; break;
				default: break;
			}
		}

		value = sync_get_val(track, current_row);
	}

	snprintf(valueText, 256, "%f", value);
	Emgui_drawText(valueText, posX + 4, sizeY - 15, Emgui_color32(160, 160, 160, 255));
	Emgui_drawBorder(Emgui_color32(10, 10, 10, 255), Emgui_color32(10, 10, 10, 255), posX, sizeY - 17, 80, 15); 
	Emgui_drawText(str, posX + 85, sizeY - 15, Emgui_color32(160, 160, 160, 255));
	Emgui_drawBorder(Emgui_color32(10, 10, 10, 255), Emgui_color32(10, 10, 10, 255), posX + 80, sizeY - 17, 40, 15); 

	return 130;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static int drawNameValue(char* name, int posX, int sizeY, int* value, int low, int high, char* buffer)
{
	int current_value;
	int text_size = Emgui_getTextSize(name) & 0xffff;

	Emgui_drawText(name, posX + 4, sizeY - 15, Emgui_color32(160, 160, 160, 255));

	current_value = atoi(buffer);

	if (current_value != *value)
	{
		if (buffer[0] != 0)
			snprintf(buffer, 64, "%d", *value);
	}

	Emgui_editBoxXY(posX + text_size + 6, sizeY - 15, 50, 13, 64, buffer); 

	current_value = atoi(buffer);

	if (current_value != *value)
	{
		current_value = eclampi(current_value, low, high); 
		if (buffer[0] != 0)
			snprintf(buffer, 64, "%d", current_value);
		*value = current_value;
	}

	return text_size + 56;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static void drawStatus()
{
	int size = 0;
	const int sizeY = s_editorData.trackViewInfo.windowSizeY;
	const int sizeX = s_editorData.trackViewInfo.windowSizeX;
	const int prevRow = getRowPos(); 

	Emgui_setFont(s_editorData.trackViewInfo.smallFontId);

	Emgui_fill(Emgui_color32(20, 20, 20, 255), 2, sizeY - 15, sizeX - 3, 13); 
	Emgui_drawBorder(Emgui_color32(10, 10, 10, 255), Emgui_color32(10, 10, 10, 255), 0, sizeY - 17, sizeX - 2, 15); 

	size = drawConnectionStatus(0, sizeY);
	size += drawCurrentValue(size, sizeY);
	size += drawNameValue("Row", size, sizeY, &s_editorData.trackViewInfo.rowPos, 0, 20000 - 1, s_currentRow);
	size += drawNameValue("Start Row", size, sizeY, &s_editorData.trackData.startRow, 0, 10000000, s_startRow);
	size += drawNameValue("End Row", size, sizeY, &s_editorData.trackData.endRow, 0, 10000000, s_endRow);

	if (getRowPos() != prevRow)
		RemoteConnection_sendSetRowCommand(getRowPos());

	Emgui_setDefaultFont();
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void Editor_updateTrackScroll()
{
	int track_start_offset, sel_track, total_track_width = 0;
	int track_start_pixel = s_editorData.trackViewInfo.startPixel;
	TrackData* track_data = getTrackData();
	TrackViewInfo* view_info = getTrackViewInfo(); 

	total_track_width = TrackView_getWidth(getTrackViewInfo(), getTrackData());
	track_start_offset = TrackView_getStartOffset();

	track_start_pixel = eclampi(track_start_pixel, 0, emaxi(total_track_width - (view_info->windowSizeX / 2), 0)); 

	sel_track = TrackView_getScrolledTrack(view_info, track_data, track_data->activeTrack, 
										   track_start_offset - track_start_pixel);

	if (sel_track != track_data->activeTrack)
		TrackData_setActiveTrack(track_data, sel_track);

	s_editorData.trackViewInfo.startPixel = track_start_pixel;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static void drawHorizonalSlider()
{
	int large_val; 
	TrackViewInfo* info = getTrackViewInfo();
	const int old_start = s_editorData.trackViewInfo.startPixel;
	const int total_track_width = TrackView_getWidth(info, getTrackData()) - (info->windowSizeX / 2);

	large_val = emaxi(total_track_width / 10, 1);

	Emgui_slider(0, info->windowSizeY - 36, info->windowSizeX, 14, 0, total_track_width, large_val, 
				EMGUI_SLIDERDIR_HORIZONTAL, 1, &s_editorData.trackViewInfo.startPixel);

	if (old_start != s_editorData.trackViewInfo.startPixel)
		Editor_updateTrackScroll();
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static void drawVerticalSlider()
{
	int start_row = 0, end_row = 10000, width, large_val; 
	TrackData* trackData = getTrackData();
	TrackViewInfo* info = getTrackViewInfo();
	const int rowPos = getRowPos();

	if (trackData) 
	{
		start_row = trackData->startRow;
		end_row = trackData->endRow;
	}

	width = end_row - start_row;
	large_val = emaxi(width / 10, 1);

	Emgui_slider(info->windowSizeX - 26, 0, 20, info->windowSizeY, 0, width, large_val, 
				EMGUI_SLIDERDIR_VERTICAL, 1, &s_editorData.trackViewInfo.rowPos);

	if (rowPos != getRowPos())
		Editor_updateTrackScroll();
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// In some cases we need an extra update in case some controls has been re-arranged in such fashion so
// the trackview will report back if that is needed (usually happens if tracks gets resized)

static bool internalUpdate()
{
	int refresh;

	Emgui_begin();
	drawStatus();
	drawHorizonalSlider();
	drawVerticalSlider();

	refresh = TrackView_render(&s_editorData.trackViewInfo, &s_editorData.trackData);
	Emgui_end();

	return !!refresh; 
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void Editor_update()
{
	bool need_update = internalUpdate();

	if (need_update)
	{
		Editor_updateTrackScroll();
		internalUpdate();
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static void copySelection(int row, int track, int selectLeft, int selectRight, int selectTop, int selectBottom)
{
	CopyEntry* entry = 0;
	int copy_count = 0;
	struct sync_track** tracks = getTracks();

	// Count how much we need to copy

	for (track = selectLeft; track <= selectRight; ++track) 
	{
		struct sync_track* t = tracks[track];
		for (row = selectTop; row <= selectBottom; ++row) 
		{
			int idx = sync_find_key(t, row);
			if (idx < 0) 
				continue;

			copy_count++;
		}
	}

	free(s_copyData.entries);
	entry = s_copyData.entries = malloc(sizeof(CopyEntry) * copy_count);

	for (track = selectLeft; track <= selectRight; ++track) 
	{
		struct sync_track* t = tracks[track];
		for (row = selectTop; row <= selectBottom; ++row) 
		{
			int idx = sync_find_key(t, row);
			if (idx < 0) 
				continue;

			entry->track = track - selectLeft;
			entry->keyFrame = t->keys[idx];
			entry->keyFrame.row -= selectTop; 
			entry++;
		}
	}

	s_copyData.bufferWidth = selectRight - selectLeft + 1;
	s_copyData.bufferHeight = selectBottom - selectTop + 1;
	s_copyData.count = copy_count;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static void deleteArea(int rowPos, int track, int bufferWidth, int bufferHeight)
{
	int i, j;
	const int track_count = getTrackCount();
	struct sync_track** tracks = getTracks();

	Commands_beginMulti("deleteArea");

	for (i = 0; i < bufferWidth; ++i) 
	{
		struct sync_track* t;
		int trackPos = track + i;
		int trackIndex = trackPos;

		if (trackPos >= track_count) 
			continue;

		t = tracks[trackIndex];

		for (j = 0; j < bufferHeight; ++j) 
		{
			int row = rowPos + j;

			Commands_deleteKey(trackIndex, row);
		}
	}

	Commands_endMulti();
	updateNeedsSaving();
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static void biasSelection(float value, int selectLeft, int selectRight, int selectTop, int selectBottom)
{
	int track, row;
	struct sync_track** tracks = getTracks();

	Commands_beginMulti("biasSelection");

	for (track = selectLeft; track <= selectRight; ++track) 
	{
		struct sync_track* t = tracks[track];

		for (row = selectTop; row <= selectBottom; ++row) 
		{
			struct track_key newKey;
			int idx = sync_find_key(t, row);
			if (idx < 0) 
				continue;

			newKey = t->keys[idx];
			newKey.value += value;

			Commands_updateKey(track, &newKey);
		}
	}

	Commands_endMulti();
	updateNeedsSaving();
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static char s_editBuffer[512];
static bool is_editing = false;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static void endEditing()
{
	const char* track_name;
	struct track_key key;
	struct sync_track* track;
	int row_pos = getRowPos();
	int active_track = getActiveTrack();

	if (!is_editing || !getTracks())
		return;

	track = getTracks()[active_track];

	key.row = row_pos;
	key.value = (float)atof(s_editBuffer);
	key.type = 0;

	if (is_key_frame(track, row_pos))
	{
		int idx = key_idx_floor(track, row_pos);
		key.type = track->keys[idx].type;
	}

	track_name = track->name; 

	Commands_addOrUpdateKey(active_track, &key);
	updateNeedsSaving();

	is_editing = false;
	s_editorData.trackData.editText = 0;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

enum ArrowDirection
{
	ARROW_LEFT,
	ARROW_RIGHT,
};

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static void onArrowSide(enum ArrowDirection dir, int row, TrackData* trackData, int activeTrack, int keyMod)
{
	int track;
	const bool shouldFold = dir == ARROW_LEFT ? true : false;
	const int trackCount = getTrackCount();
	TrackViewInfo* viewInfo = getTrackViewInfo(); 

	if (keyMod & EMGUI_KEY_ALT)
	{
		Track* t = &trackData->tracks[activeTrack];

		if (keyMod & EMGUI_KEY_CTRL) 
		{
			if (t->group->trackCount > 1)
				t->group->folded = shouldFold;
		}
		else
			t->folded = shouldFold;

		Editor_updateTrackScroll();
		return;
	}

	if (dir == ARROW_LEFT)
	{
		track = getPrevTrack();

		if (keyMod & EMGUI_KEY_COMMAND)
			track = 0;

		track = emaxi(0, track); 
	}
	else
	{
		track = getNextTrack();

		if (track >= trackCount) 
			track = trackCount - 1;

		if (keyMod & EMGUI_KEY_COMMAND)
			track = trackCount - 1;
	}

	setActiveTrack(track);

	if (!TrackView_isSelectedTrackVisible(viewInfo, trackData, track))
	{
		s_editorData.trackViewInfo.startPixel += TrackView_getTracksOffset(viewInfo, trackData, activeTrack, track);
		Editor_updateTrackScroll();
	}

	if (keyMod & EMGUI_KEY_SHIFT)
	{
		Track* t = &trackData->tracks[track];

		// if this track has a folded group we can't select it so set back the selection to the old one

		if (t->group->folded)
			setActiveTrack(activeTrack);
		else
			viewInfo->selectStopTrack = track;
	}
	else
	{
		viewInfo->selectStartRow = viewInfo->selectStopRow = row;
		viewInfo->selectStartTrack = viewInfo->selectStopTrack = track;
	}

}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static void onArrowUp(int row, TrackData* trackData, struct sync_track* track, int activeTrack, int modifiers)
{
	TrackViewInfo* viewInfo = getTrackViewInfo(); 

	if (modifiers & EMGUI_KEY_CTRL)
	{
		if (track->keys)
		{
			int idx = sync_find_key(track, row);
			if (idx < 0)
				idx = -idx - 1;

			viewInfo->rowPos = row = track->keys[emaxi(idx - 1, 0)].row;

			if (modifiers & EMGUI_KEY_SHIFT)
				viewInfo->selectStartRow = row;
			else
				viewInfo->selectStartRow = viewInfo->selectStopRow = row;
		}
	}
	else
	{
		row -= modifiers & EMGUI_KEY_ALT ? 8 : 1;

		if ((modifiers & EMGUI_KEY_COMMAND) || row < trackData->startRow)
			row = TrackData_getPrevBookmark(trackData, row); 

		viewInfo->rowPos = row;

		if (modifiers & EMGUI_KEY_SHIFT)
		{
			viewInfo->selectStopRow = row;
			return;
		}

		viewInfo->selectStartRow = viewInfo->selectStopRow = row;
		viewInfo->selectStartTrack = viewInfo->selectStopTrack = activeTrack;
	}

	RemoteConnection_sendSetRowCommand(row);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static void onArrowDown(int rowPos, TrackData* trackData, struct sync_track* track, int activeTrack, int modifiers)
{
	int row = getRowPos();
	TrackViewInfo* viewInfo = getTrackViewInfo(); 

	if (modifiers & EMGUI_KEY_CTRL)
	{
		if (track->keys)
		{
			int idx = key_idx_floor(track, rowPos);

			if (idx < 0)
				row = track->keys[0].row;
			else if (idx > (int)track->num_keys - 2)
				row = track->keys[track->num_keys - 1].row;
			else
				row = track->keys[idx + 1].row;

			viewInfo->rowPos = row; 
	
			if (modifiers & EMGUI_KEY_SHIFT)
				viewInfo->selectStopRow = row;
			else
				viewInfo->selectStartRow = viewInfo->selectStopRow = row;
		}
	}
	else
	{
		row += modifiers & EMGUI_KEY_ALT ? 8 : 1;

		if ((modifiers & EMGUI_KEY_COMMAND) || row > trackData->endRow)
			row = TrackData_getNextBookmark(getTrackData(), row); 

		viewInfo->rowPos = row;

		if (modifiers & EMGUI_KEY_SHIFT)
		{
			viewInfo->selectStopRow = row;
			return;
		}

		viewInfo->selectStartRow = viewInfo->selectStopRow = row;
		viewInfo->selectStartTrack = viewInfo->selectStopTrack = activeTrack;
	}

	RemoteConnection_sendSetRowCommand(row);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static void onArrow(int key, int modifiers, int rowPos, struct sync_track* track, int activeTrack)
{
	TrackData* trackData = getTrackData();

	endEditing();

	switch (key)
	{
		case EMGUI_ARROW_DOWN:  onArrowDown(rowPos, trackData, track, activeTrack, modifiers); break;
		case EMGUI_ARROW_UP:    onArrowUp(rowPos, trackData, track, activeTrack, modifiers); break;
		case EMGUI_ARROW_RIGHT: onArrowSide(ARROW_RIGHT, rowPos, trackData, activeTrack, modifiers); break;
		case EMGUI_ARROW_LEFT:  onArrowSide(ARROW_LEFT, rowPos, trackData, activeTrack, modifiers); break;
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

bool Editor_keyDown(int key, int keyCode, int modifiers)
{
	bool handled_key = true;
	TrackData* trackData = &s_editorData.trackData;
	TrackViewInfo* viewInfo = &s_editorData.trackViewInfo;
	bool paused = RemoteConnection_isPaused();
	struct sync_track** tracks = getTracks();
	int active_track = getActiveTrack();
	int row_pos = viewInfo->rowPos;

	const int selectLeft = mini(viewInfo->selectStartTrack, viewInfo->selectStopTrack);
	const int selectRight = maxi(viewInfo->selectStartTrack, viewInfo->selectStopTrack);
	const int selectTop = mini(viewInfo->selectStartRow, viewInfo->selectStopRow);
	const int selectBottom = maxi(viewInfo->selectStartRow, viewInfo->selectStopRow);

	// If some emgui control has focus let it do its thing until its done

	if (Emgui_hasKeyboardFocus())
	{
		Editor_update();
		return true;
	}

	if (key == EMGUI_KEY_TAB)
	{
		Emgui_setFirstControlFocus();
		Editor_update();
		return true;
	}

	switch (key)
	{
		case EMGUI_KEY_BACKSPACE:
		{
			endEditing();
			deleteArea(row_pos, active_track, 1, 1);
			break;
		}

		case EMGUI_ARROW_DOWN:
		case EMGUI_ARROW_UP:
		case EMGUI_ARROW_RIGHT:
		case EMGUI_ARROW_LEFT: 
		{
			onArrow(key, modifiers, row_pos, tracks[active_track], active_track);

			if (paused)
				Editor_update();

			return true;
		}

		default : handled_key = false; break;
	}

	if (key == ' ')
	{
		// TODO: Don't start playing if we are in edit mode (but space shouldn't be added in edit mode but we still
		// shouldn't start playing if we do

		endEditing();
		RemoteConnection_sendPauseCommand(!paused);
		Editor_update();
		return true;
	}

	if (!paused)
		return true;

	// handle copy of tracks/values

	if (key == 'c' && (modifiers & EMGUI_KEY_COMMAND))
	{
		copySelection(row_pos, active_track, selectLeft, selectRight, selectTop, selectBottom);
		return true;
	}

	if (key == 'x' && (modifiers & EMGUI_KEY_COMMAND))
	{
		copySelection(row_pos, active_track, selectLeft, selectRight, selectTop, selectBottom);
		deleteArea(selectTop, selectLeft, s_copyData.bufferWidth, s_copyData.bufferHeight);
		handled_key = true;
	}

	if (key == 't' && (modifiers & EMGUI_KEY_COMMAND))
	{
		struct sync_track* t = getTracks()[active_track];

		viewInfo->selectStartTrack = viewInfo->selectStopTrack = active_track;

		if (t->keys)
		{
			viewInfo->selectStartRow = t->keys[0].row;
			viewInfo->selectStopRow = t->keys[t->num_keys - 1].row;
	
		}
		else
		{
			viewInfo->selectStartRow = viewInfo->selectStopRow = row_pos;
		}

		handled_key = true;
	}

	if (key == 'z' || key == 'Z')
	{
		if (modifiers & EMGUI_KEY_SHIFT)
			Commands_redo();
		else
			Commands_undo();

		updateNeedsSaving();
		Editor_update();

		return true;
	}

	if (key == 'b' || key == 'B')
	{
		TrackData_toogleBookmark(trackData, row_pos);
		Editor_update();
		return true;
	}

	// Handle paste of data

	if (key == 'v' && (modifiers & EMGUI_KEY_COMMAND))
	{
		const int buffer_width = s_copyData.bufferWidth;
		const int buffer_height = s_copyData.bufferHeight;
		const int buffer_size = s_copyData.count;
		const int track_count = getTrackCount();
		int i, trackPos;

		if (!s_copyData.entries)
			return false;

		// First clear the paste area

		deleteArea(row_pos, active_track, buffer_width, buffer_height);

		Commands_beginMulti("pasteArea");

		for (i = 0; i < buffer_size; ++i)
		{
			const CopyEntry* ce = &s_copyData.entries[i];
			
			trackPos = active_track + ce->track;
			if (trackPos < track_count)
			{
				size_t trackIndex = trackPos;
				struct track_key key = ce->keyFrame;
				key.row += row_pos;

				Commands_addOrUpdateKey(trackIndex, &key);
			}
		}

		Commands_endMulti();
		updateNeedsSaving();

		handled_key = true;
	}

	// Handle biasing of values

	if ((keyCode >=  0 && keyCode <=  6) || (keyCode >= 12 && keyCode <= 17))
	{
		float bias_value = 0.0f;

		switch (keyCode)
		{
			case 0 : bias_value = -0.01f; break;
			case 1 : bias_value = -0.1f; break;
			case 2 : bias_value = -1.0f; break;
			case 3 : bias_value = -10.f; break;
			case 5 : bias_value = -100.0f; break;
			case 4 : bias_value = -1000.0f; break;
			case 12 : bias_value = 0.01f; break;
			case 13 : bias_value = 0.1f; break;
			case 14 : bias_value = 1.0f; break;
			case 15 : bias_value = 10.f; break;
			case 17 : bias_value = 100.0f; break;
			case 16 : bias_value = 1000.0f; break;
		}

		biasSelection(bias_value, selectLeft, selectRight, selectTop, selectBottom);

		Editor_update();

		return true;
	}

	// do edit here and biasing here

	if ((key >= '0' && key <= '9') || key == '.' || key == '-')
	{
		if (!is_editing)
		{
			memset(s_editBuffer, 0, sizeof(s_editBuffer));
			is_editing = true;
		}

		s_editBuffer[strlen(s_editBuffer)] = (char)key;
		s_editorData.trackData.editText = s_editBuffer;

		return true;
	}
	else if (is_editing)
	{
		// if we press esc we discard the value

		if (!tracks)
			return true;

		if (key != 27)
		{
			endEditing();

		}

		is_editing = false;
		s_editorData.trackData.editText = 0;
	}

	if (key == EMGUI_KEY_TAB_ENTER)
	{
		struct sync_track* t = getTracks()[active_track];

		if (t->keys)
		{
			struct track_key key;
			int idx = sync_find_key(t, row_pos);
			if (idx < 0)
				idx = -idx - 1;

			key.row = row_pos;
			key.value = sync_get_val(t, row_pos);
			key.type = t->keys[emaxi(idx - 1, 0)].type;

			Commands_addOrUpdateKey(active_track, &key);
			updateNeedsSaving();
		}

		handled_key = true;
	}

	if (key == 'i' || key == 'I' || key == 'q')
	{
		struct track_key newKey;
		struct sync_track* track = tracks[active_track];
		int row = viewInfo->rowPos;

		int idx = key_idx_floor(track, row);
		if (idx < 0) 
			return false;

		newKey = track->keys[idx];
		newKey.type = ((newKey.type + 1) % KEY_TYPE_COUNT);

		Commands_addOrUpdateKey(active_track, &newKey);
		updateNeedsSaving();

		handled_key = true;
	}

	if (handled_key)
		Editor_update();

	return handled_key;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void Editor_scroll(float deltaX, float deltaY, int flags)
{
	int current_row = s_editorData.trackViewInfo.rowPos;
	int old_offset = s_editorData.trackViewInfo.startPixel;
	int start_row = 0, end_row = 10000;

	if (trackData)
	{
		start_row = trackData->startRow;
		end_row = trackData->endRow;
	}

	if (flags & EMGUI_KEY_ALT)
	{
		TrackViewInfo* viewInfo = &s_editorData.trackViewInfo;
		const int selectLeft = mini(viewInfo->selectStartTrack, viewInfo->selectStopTrack);
		const int selectRight = maxi(viewInfo->selectStartTrack, viewInfo->selectStopTrack);
		const int selectTop = mini(viewInfo->selectStartRow, viewInfo->selectStopRow);
		const int selectBottom = maxi(viewInfo->selectStartRow, viewInfo->selectStopRow);
		const float multiplier = flags & EMGUI_KEY_SHIFT ? 0.1f : 0.01f;

		biasSelection(-deltaY * multiplier, selectLeft, selectRight, selectTop, selectBottom);

		Editor_update();

		return;
	}

	current_row += (int)deltaY;

	if (current_row < start_row || current_row >= end_row)
		return;
    
	s_editorData.trackViewInfo.startPixel += (int)(deltaX * 4.0f);
	s_editorData.trackViewInfo.rowPos = eclampi(current_row, start_row, end_row);

	RemoteConnection_sendSetRowCommand(s_editorData.trackViewInfo.rowPos);

	if (old_offset != s_editorData.trackViewInfo.startPixel)
		Editor_updateTrackScroll(); 

	Editor_update();
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static int processCommands()
{
	int strLen, newRow, serverIndex;
	unsigned char cmd = 0;
	int ret = 0;

	if (RemoteConnection_recv((char*)&cmd, 1, 0)) 
	{
		switch (cmd) 
		{
			case GET_TRACK:
			{
				char trackName[4096];

				reset_tracks = true;	

				memset(trackName, 0, sizeof(trackName));

				RemoteConnection_recv((char *)&strLen, sizeof(int), 0);
				strLen = ntohl(strLen);

				if (!RemoteConnection_connected())
					return 0;

				if (!RemoteConnection_recv(trackName, strLen, 0))
					return 0;

				rlog(R_INFO, "Got trackname %s (%d) from demo\n", trackName, strLen);

				// find track
				
				serverIndex = TrackData_createGetTrack(&s_editorData.trackData, trackName);

				// setup remap and send the keyframes to the demo
				RemoteConnection_mapTrackName(trackName);
				RemoteConnection_sendKeyFrames(trackName, s_editorData.trackData.syncData.tracks[serverIndex]);
				TrackData_linkTrack(serverIndex, trackName, &s_editorData.trackData);

				s_editorData.trackData.tracks[serverIndex].active = true;

				setActiveTrack(0);

				ret = 1;

				break;
			}

			case SET_ROW:
			{
				int i = 0;
				ret = RemoteConnection_recv((char*)&newRow, sizeof(int), 0);

				if (ret == -1)
				{
					// retry to get the data and do it for max of 20 times otherwise disconnect

					for (i = 0; i < 20; ++i)
					{
						if (RemoteConnection_recv((char*)&newRow, sizeof(int), 0) == 4)
						{
							s_editorData.trackViewInfo.rowPos = htonl(newRow);
							rlog(R_INFO, "row from demo %d\n", s_editorData.trackViewInfo.rowPos);
							return 1;
						}
					}
				}
				else
				{
					s_editorData.trackViewInfo.rowPos = htonl(newRow);
					rlog(R_INFO, "row from demo %d\n", s_editorData.trackViewInfo.rowPos);
				}

				ret = 1;

				break;
			}
		}
	}

	return ret;
}


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static void updateTrackStatus()
{
	int i, track_count = getTrackCount();

	if (RemoteConnection_connected())
		return;

	if (reset_tracks)
	{
		for (i = 0; i < track_count; ++i)
			s_editorData.trackData.tracks[i].active = false;

		Editor_update();

		reset_tracks = false;
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void Editor_timedUpdate()
{
	int processed_commands = 0;

	RemoteConnection_updateListner(getRowPos());

	updateTrackStatus();

	while (RemoteConnection_pollRead())
		processed_commands |= processCommands();

	if (!RemoteConnection_isPaused() || processed_commands)
	{
		Editor_update();
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static void setWindowTitle(const char* path, bool needsSave)
{
	char windowTitle[4096];
	if (needsSave)
		sprintf(windowTitle, "RocketEditor - (%s) *", path);
	else
		sprintf(windowTitle, "RocketEditor - (%s)", path);

	Window_setTitle(windowTitle);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void updateNeedsSaving()
{
	int undoCount;

	if (!s_loadedFilename)
		return;

	undoCount = Commands_undoCount();

	if (s_undoLevel != undoCount)
		setWindowTitle(s_loadedFilename, true);
	else
		setWindowTitle(s_loadedFilename, false);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static void onFinishedLoad(const char* path)
{
	Editor_update();
	setWindowTitle(path, false);
	setMostRecentFile(path);
	s_undoLevel = Commands_undoCount();
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void Editor_loadRecentFile(int id)
{
	char path[2048];
	strcpy(path, s_recentFiles[id]);  // must be unique buffer when doing set mostRecent

	if (LoadSave_loadRocketXML(path, getTrackData()))
		onFinishedLoad(path);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static void onOpen()
{
	char currentFile[2048];

	if (LoadSave_loadRocketXMLDialog(currentFile, getTrackData()))
		onFinishedLoad(currentFile);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static bool onSaveDialog()
{
	char path[2048];
	int ret;

	if (!(ret = LoadSave_saveRocketXMLDialog(path, getTrackData())))
		return false;

	setMostRecentFile(path);
	setWindowTitle(path, false);
	s_undoLevel = Commands_undoCount();

	return true;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static void onSave()
{
	if (!s_loadedFilename)
		onSaveDialog();
	else
		LoadSave_saveRocketXML(getMostRecentFile(), getTrackData()); 

	setWindowTitle(getMostRecentFile(), false);
	s_undoLevel = Commands_undoCount();
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

bool Editor_saveBeforeExit()
{
	if (s_loadedFilename)
	{
		onSave();
		return true;
	}

	return onSaveDialog();
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void Editor_menuEvent(int menuItem)
{
	switch (menuItem)
	{
		case EDITOR_MENU_OPEN : onOpen(); break;
		case EDITOR_MENU_SAVE : onSave(); break;
		case EDITOR_MENU_SAVE_AS : onSaveDialog(); break;
		case EDITOR_MENU_REMOTE_EXPORT : RemoteConnection_sendSaveCommand(); break;
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void Editor_destroy()
{
}

