#include "Editor.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include "Menu.h"
#include "Dialog.h"
#include "loadsave.h"
#include "TrackView.h"
#include "rlog.h"
#include "minmax.h"
#include "TrackData.h"
#include "RemoteConnection.h"
#include "Commands.h"
#include "MinecraftiaFont.h"
#include "Window.h"
#include "../../sync/sync.h"
#include "../../sync/base.h"
#include "../../sync/data.h"
#include <emgui/Emgui.h>

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
static text_t s_filenames[5][2048];
static text_t* s_loadedFilename = 0;

static text_t* s_recentFiles[] =
{
	s_filenames[0],
	s_filenames[1],
	s_filenames[2],
	s_filenames[3],
	s_filenames[4],
};

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

text_t** Editor_getRecentFiles()
{
	return (text_t**)s_recentFiles;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

const text_t* getMostRecentFile()
{
	return s_recentFiles[0];
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static void textCopy(text_t* dest, const text_t* src)
{
#if defined(_WIN32)
	wcscpy_s(dest, 2048, src);
#else
	strcpy(dest, src);
#endif
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static int textCmp(text_t* t0, const text_t* t1)
{
#if defined(_WIN32)
	return wcscmp(t0, t1);
#else
	return strcmp(t0, t1);
#endif
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void setMostRecentFile(const text_t* filename)
{
	int i;

	// move down all files
	for (i = 3; i >= 0; --i)
		textCopy(s_recentFiles[i+1], s_recentFiles[i]);

	textCopy(s_recentFiles[0], filename);
	s_loadedFilename = s_recentFiles[0];

	// check if the string was already present and remove it if that is the case by compacting the array
	
	for (i = 1; i < 5; ++i)
	{
		if (!textCmp(s_recentFiles[i], filename))
		{
			for (; i < 4; ++i)
				textCopy(s_recentFiles[i], s_recentFiles[i + 1]);

			break;
		}
	}

	Window_populateRecentList((const text_t**)s_recentFiles);
}

// 

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

static inline void setRowPos(int pos)
{
	s_editorData.trackViewInfo.rowPos = pos;
	RemoteConnection_sendSetRowCommand(pos);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static int getNextTrack()
{
	Group* group;
	TrackData* trackData = getTrackData();
	int groupIndex = 0;
	int groupTrackCount = 0;
	int currentTrack = getActiveTrack();

	// Get the group for the currentTrack

	Track* track = &trackData->tracks[currentTrack];
	group = track->group; 
	groupTrackCount = group->trackCount;

	// Check If next track is within the group

	if (!group->folded)
	{
		if (track->groupIndex + 1 < group->trackCount)
			return group->tracks[track->groupIndex + 1]->index; 
	}

	groupIndex = group->groupIndex;

	// We are at the last track in the last group so just return the current one

	if (groupIndex >= trackData->groupCount-1)
		return currentTrack;

	// Get the next group and select the first track in it

	group = &trackData->groups[groupIndex + 1];
	return group->tracks[0]->index;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static int getPrevTrack()
{
	Group* group;
	TrackData* trackData = getTrackData();
	int trackIndex = 0;
	int groupIndex = 0;
	int groupTrackCount = 0;
	int currentTrack = getActiveTrack();

	// Get the group for the currentTrack

	Track* track = &trackData->tracks[currentTrack];
	group = track->group; 
	groupTrackCount = group->trackCount;

	// Check If next track is within the group

	if (track->groupIndex - 1 >= 0)
		return group->tracks[track->groupIndex - 1]->index; 

	groupIndex = group->groupIndex - 1;

	// We are at the last track in the last group so just return the current one

	if (groupIndex < 0)
		return currentTrack;

	// Get the next group and select the first track in it

	group = &trackData->groups[groupIndex];
	trackIndex = group->folded ? 0 : group->trackCount - 1;

	return group->tracks[trackIndex]->index;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void Editor_create()
{
	int id;
	Emgui_create();
	id = Emgui_loadFontBitmap(g_minecraftiaFont, g_minecraftiaFontSize, EMGUI_LOCATION_MEMORY, 32, 128, g_minecraftiaFontLayout);

	RemoteConnection_createListner();

	s_editorData.trackViewInfo.smallFontId = id;
	s_editorData.trackData.startRow = 0;
	s_editorData.trackData.endRow = 10000;
	s_editorData.trackData.highlightRowStep = 8;

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

		value = (float)sync_get_val(track, current_row);
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

	refresh = TrackView_render(getTrackViewInfo(), getTrackData()); 
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

static void biasSelection(float value)
{
	int track, row;
	struct sync_track** tracks = getTracks();
	TrackViewInfo* viewInfo = getTrackViewInfo();
	int selectLeft = mini(viewInfo->selectStartTrack, viewInfo->selectStopTrack);
	int selectRight = maxi(viewInfo->selectStartTrack, viewInfo->selectStopTrack);
	int selectTop = mini(viewInfo->selectStartRow, viewInfo->selectStopRow);
	int selectBottom = maxi(viewInfo->selectStartRow, viewInfo->selectStopRow);
	
	// If we have no selection and no currenty key bias the previous key

	if (selectLeft == selectRight && selectTop == selectBottom)
	{
		int idx;
		struct sync_track* track;
		struct sync_track** tracks = getTracks();

		if (!tracks || !tracks[getActiveTrack()]->keys)
			return;

		track = tracks[getActiveTrack()];

		idx = sync_find_key(track, getRowPos());
		
		if (idx < 0) 
		{
			idx = -idx - 1;
			selectTop = selectBottom = track->keys[emaxi(idx - 1, 0)].row;
		}
	}
	
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

	if (strcmp(s_editBuffer, ""))
	{

		key.row = row_pos;
		key.value = (float)atof(s_editBuffer);
		key.type = 0;

		if (track->num_keys > 0)
		{
			int idx = sync_find_key(track, getRowPos());

			if (idx > 0)
			{
				// exact match, keep current type
				key.type = track->keys[emaxi(idx, 0)].type;
			}
			else
			{
				// no match, grab type from previous key
				if (idx < 0)
					idx = -idx - 1;

				key.type = track->keys[emaxi(idx - 1, 0)].type;
			}
		}	

		track_name = track->name; 

		Commands_addOrUpdateKey(active_track, &key);
		updateNeedsSaving();
	}

	is_editing = false;
	s_editorData.trackData.editText = 0;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static void cancelEditing()
{
	is_editing = false;
	s_editorData.trackData.editText = 0;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void Editor_scroll(float deltaX, float deltaY, int flags)
{
	TrackData* trackData = getTrackData();
	int current_row = s_editorData.trackViewInfo.rowPos;
	int old_offset = s_editorData.trackViewInfo.startPixel;
	int start_row = 0, end_row = 10000;

	start_row = trackData->startRow;
	end_row = trackData->endRow + 1;	// +1 as we count from 0

	if (flags & EMGUI_KEY_ALT)
	{
		const float multiplier = flags & EMGUI_KEY_SHIFT ? 0.1f : 0.01f;
		biasSelection(-deltaY * multiplier);
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
	TrackViewInfo* viewInfo = getTrackViewInfo();

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
				// if it's the first one we get, select it too
				if (serverIndex == 0)
					setActiveTrack(0);

				// setup remap and send the keyframes to the demo
				RemoteConnection_mapTrackName(trackName);
				RemoteConnection_sendKeyFrames(trackName, s_editorData.trackData.syncData.tracks[serverIndex]);
				TrackData_linkTrack(serverIndex, trackName, &s_editorData.trackData);

				s_editorData.trackData.tracks[serverIndex].active = true;

				ret = 1;

				break;
			}

			case SET_ROW:
			{
				//int i = 0;
				ret = RemoteConnection_recv((char*)&newRow, sizeof(int), 0);

				if (ret)
				{
					viewInfo->rowPos = htonl(newRow);
					viewInfo->selectStartRow = viewInfo->selectStopRow = viewInfo->rowPos;
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

static void setWindowTitle(const text_t* path, bool needsSave)
{
	text_t windowTitle[4096];
#if defined(_WIN32)
	if (needsSave)
		swprintf_s(windowTitle, sizeof(windowTitle), L"RocketEditor" EDITOR_VERSION L"- (%s) *", path);
	else
		swprintf_s(windowTitle, sizeof(windowTitle), L"RocketEditor" EDITOR_VERSION L" - (%s)", path);
#else
	if (needsSave)
		sprintf(windowTitle, "RocketEditor" EDITOR_VERSION "- (%s) *", path);
	else
		sprintf(windowTitle, "RocketEditor" EDITOR_VERSION "- (%s)", path);
#endif

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

bool Editor_needsSave()
{
	return s_undoLevel != Commands_undoCount();
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static void onFinishedLoad(const text_t* path)
{
	Editor_update();
	setWindowTitle(path, false);
	setMostRecentFile(path);
	s_undoLevel = Commands_undoCount();
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void Editor_loadRecentFile(int id)
{
	text_t path[2048];
	textCopy(path, s_recentFiles[id]);  // must be unique buffer when doing set mostRecent

	if (LoadSave_loadRocketXML(path, getTrackData()))
		onFinishedLoad(path);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static void onOpen()
{
	text_t currentFile[2048];

	if (LoadSave_loadRocketXMLDialog(currentFile, getTrackData()))
		onFinishedLoad(currentFile);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static bool onSaveAs()
{
	text_t path[2048];
	int ret;

	memset(path, 0, sizeof(path));

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
		onSaveAs();
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

	return onSaveAs();
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static void onUndo()
{
	Commands_undo();
	updateNeedsSaving();
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static void onRedo()
{
	Commands_undo();
	updateNeedsSaving();
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static void onCancelEdit()
{
	cancelEditing();
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static void onDeleteKey()
{
	deleteArea(getRowPos(), getActiveTrack(), 1, 1);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static void onCutAndCopy(bool cut)
{
	TrackViewInfo* viewInfo = getTrackViewInfo();
	const int selectLeft = mini(viewInfo->selectStartTrack, viewInfo->selectStopTrack);
	const int selectRight = maxi(viewInfo->selectStartTrack, viewInfo->selectStopTrack);
	const int selectTop = mini(viewInfo->selectStartRow, viewInfo->selectStopRow);
	const int selectBottom = maxi(viewInfo->selectStartRow, viewInfo->selectStopRow);

	if (!cut)
	{
		copySelection(getRowPos(), getActiveTrack(), selectLeft, selectRight, selectTop, selectBottom);
	}
	else
	{
		copySelection(getRowPos(), getActiveTrack(), selectLeft, selectRight, selectTop, selectBottom);
		deleteArea(selectTop, selectLeft, s_copyData.bufferWidth, s_copyData.bufferHeight);
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static void onPaste()
{
	const int buffer_width = s_copyData.bufferWidth;
	const int buffer_height = s_copyData.bufferHeight;
	const int buffer_size = s_copyData.count;
	const int track_count = getTrackCount();
	const int row_pos = getRowPos();
	const int active_track = getActiveTrack();
	int i, trackPos;

	if (!s_copyData.entries)
		return;

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
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static void onSelectTrack()
{
	int activeTrack = getActiveTrack();
	TrackViewInfo* viewInfo = getTrackViewInfo();
	struct sync_track** tracks;
	struct sync_track* track;
	
	if (!(tracks = getTracks()))
		return;

	track = tracks[activeTrack];
	viewInfo->selectStartTrack = viewInfo->selectStopTrack = activeTrack;

	if (track->keys)
	{
		viewInfo->selectStartRow = track->keys[0].row;
		viewInfo->selectStopRow = track->keys[track->num_keys - 1].row;
	}
	else
	{
		viewInfo->selectStartRow = viewInfo->selectStopRow = getRowPos();
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static void onInterpolation()
{
	int idx;
	struct track_key newKey;
	struct sync_track* track;
	struct sync_track** tracks = getTracks();

	if (!tracks)
		return;

	track = tracks[getActiveTrack()];

	idx = key_idx_floor(track, getRowPos());
	if (idx < 0) 
		return;

	newKey = track->keys[idx];
	newKey.type = ((newKey.type + 1) % KEY_TYPE_COUNT);

	Commands_addOrUpdateKey(getActiveTrack(), &newKey);
	updateNeedsSaving();
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static void onEnterCurrentValue()
{
	int idx;
	struct track_key key;
	struct sync_track* track;
	struct sync_track** tracks = getTracks();
	const int rowPos = getRowPos();
	const int activeTrack = getActiveTrack();

	if (!tracks)
		return;

	track = tracks[activeTrack];

	if (!track->keys)
	{
		key.row = rowPos;
		key.value = 0.0f;
		key.type = 0;

		Commands_addOrUpdateKey(activeTrack, &key);
		updateNeedsSaving();
		return;
	}

	idx = sync_find_key(track, rowPos);
	if (idx < 0)
		idx = -idx - 1;

    key.row = rowPos;
   
    if (track->num_keys > 0)
    {
        key.value = (float)sync_get_val(track, rowPos);
        key.type = track->keys[emaxi(idx - 1, 0)].type;
    }
    else
    {
        key.value = 0.0f;
        key.type = 0;
    }
    
	Commands_addOrUpdateKey(activeTrack, &key);
	updateNeedsSaving();
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static void onPlay()
{
	RemoteConnection_sendPauseCommand(!RemoteConnection_isPaused());
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

enum ArrowDirection
{
	ARROW_LEFT,
	ARROW_RIGHT,
	ARROW_UP,
	ARROW_DOWN,
};

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

enum Selection
{
	DO_SELECTION,
	NO_SELECTION,
};

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static void onRowStep(int step, enum Selection selection)
{
	TrackViewInfo* viewInfo = getTrackViewInfo(); 
	TrackData* trackData = getTrackData();

	setRowPos(eclampi(getRowPos() + step, trackData->startRow, trackData->endRow));

	if (selection == DO_SELECTION)
	{
		viewInfo->selectStopRow = getRowPos();
	}
	else
		viewInfo->selectStartRow = viewInfo->selectStopRow = getRowPos();
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static void onTrackSide(enum ArrowDirection dir, bool startOrEnd, enum Selection selection)
{
	TrackViewInfo* viewInfo = getTrackViewInfo(); 
	TrackData* trackData = getTrackData();
	const int trackCount = getTrackCount();
	const int oldTrack = getActiveTrack();
	int track = 0;

	if (dir == ARROW_LEFT)
		track = startOrEnd ? 0 : getPrevTrack();
	else
		track = startOrEnd ? trackCount - 1 : getNextTrack();

	track = eclampi(track, 0, trackCount); 

	setActiveTrack(track);

	if (selection == DO_SELECTION)
	{
		Track* t = &trackData->tracks[track];

		// if this track has a folded group we can't select it so set back the selection to the old one

		if (t->group->folded)
			setActiveTrack(oldTrack);
		else
		{
			viewInfo->selectStopTrack = track;
		}
	}
	else
	{
		viewInfo->selectStartTrack = viewInfo->selectStopTrack = track;
		viewInfo->selectStartRow = viewInfo->selectStopRow = getRowPos();
	}

	if (!TrackView_isSelectedTrackVisible(viewInfo, trackData, track))
	{
		s_editorData.trackViewInfo.startPixel += TrackView_getTracksOffset(viewInfo, trackData, oldTrack, track);
		Editor_updateTrackScroll();
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static void onBookmarkDir(enum ArrowDirection dir, enum Selection selection)
{
	TrackData* trackData = getTrackData();
	TrackViewInfo* viewInfo = getTrackViewInfo(); 
	int row = getRowPos();

	if (dir == ARROW_UP) 
		row = TrackData_getPrevBookmark(trackData, row); 
	else
		row = TrackData_getNextBookmark(trackData, row); 
		
	if (selection == NO_SELECTION)
		viewInfo->selectStartRow = viewInfo->selectStopRow = row;
	else
		viewInfo->selectStopRow = row;

	setRowPos(row);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static void onPrevNextKey(bool prevKey, enum Selection selection)
{
	struct sync_track* track;
	struct sync_track** tracks = getTracks();
	TrackViewInfo* viewInfo = getTrackViewInfo(); 

	if (!tracks || !tracks[getActiveTrack()]->keys)
		return;

	track = tracks[getActiveTrack()];

	if (prevKey)
	{
		int idx = sync_find_key(track, getRowPos());
		if (idx < 0)
			idx = -idx - 1;

		setRowPos(track->keys[emaxi(idx - 1, 0)].row);

		if (selection == DO_SELECTION)
			viewInfo->selectStopRow = getRowPos();
		else
			viewInfo->selectStartRow = viewInfo->selectStopRow = getRowPos();
	}
	else
	{
		int row = 0;

		int idx = key_idx_floor(track, getRowPos());

		if (idx < 0)
			row = track->keys[0].row;
		else if (idx > (int)track->num_keys - 2)
			row = track->keys[track->num_keys - 1].row;
		else
			row = track->keys[idx + 1].row;

		setRowPos(row);	

		if (selection == DO_SELECTION)
			viewInfo->selectStopRow = row;
		else
			viewInfo->selectStartRow = viewInfo->selectStopRow = row;
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static void onFoldTrack(bool fold)
{
	Track* t = &getTrackData()->tracks[getActiveTrack()];
	t->folded = fold;
	Editor_updateTrackScroll();
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static void onFoldGroup(bool fold)
{
	Track* t = &getTrackData()->tracks[getActiveTrack()];

	if (t->group->trackCount > 1)
	{
		TrackViewInfo* viewInfo = getTrackViewInfo(); 
		int firstTrackIndex = t->group->tracks[0]->index;
		t->group->folded = fold;
		setActiveTrack(firstTrackIndex);
		viewInfo->selectStartTrack = viewInfo->selectStopTrack = firstTrackIndex;
	}

	Editor_updateTrackScroll();
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static void onToggleBookmark()
{
	Commands_toggleBookmark(getTrackData(), getRowPos());
	updateNeedsSaving();
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static void onClearBookmarks()
{
	Commands_clearBookmarks(getTrackData());
	updateNeedsSaving();
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static void onTab()
{
	Emgui_setFirstControlFocus();
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void Editor_menuEvent(int menuItem)
{
	int highlightRowStep = getTrackData()->highlightRowStep; 

	switch (menuItem)
	{
		case EDITOR_MENU_ENTER_CURRENT_V : 
		case EDITOR_MENU_ROWS_UP :
		case EDITOR_MENU_ROWS_DOWN :
		case EDITOR_MENU_PREV_BOOKMARK :
		case EDITOR_MENU_NEXT_BOOKMARK :
		case EDITOR_MENU_PREV_KEY :
		case EDITOR_MENU_NEXT_KEY :
		case EDITOR_MENU_PLAY : 
		{
			endEditing();
		}
	}

	cancelEditing();

	// If some internal control has focus we let it do its thing

	if (Emgui_hasKeyboardFocus())
	{
		Editor_update();
		return;
	}

	switch (menuItem)
	{
		// File

		case EDITOR_MENU_OPEN: onOpen(); break;
		case EDITOR_MENU_SAVE: onSave(); break;
		case EDITOR_MENU_SAVE_AS: onSaveAs(); break;
		case EDITOR_MENU_REMOTE_EXPORT : RemoteConnection_sendSaveCommand(); break;

		case EDITOR_MENU_RECENT_FILE_0:
		case EDITOR_MENU_RECENT_FILE_1:
		case EDITOR_MENU_RECENT_FILE_2:
		case EDITOR_MENU_RECENT_FILE_3:
		{

			Editor_loadRecentFile(menuItem - EDITOR_MENU_RECENT_FILE_0);
			break;
		}

		// Edit
		
		case EDITOR_MENU_UNDO : onUndo(); break;
		case EDITOR_MENU_REDO : onRedo(); break;

		case EDITOR_MENU_CANCEL_EDIT :  onCancelEdit(); break;
		case EDITOR_MENU_DELETE_KEY  :  onDeleteKey(); break;
		case EDITOR_MENU_CUT :          onCutAndCopy(true); break;
		case EDITOR_MENU_COPY :         onCutAndCopy(false); break;
		case EDITOR_MENU_PASTE :        onPaste(); break;
		case EDITOR_MENU_SELECT_TRACK : onSelectTrack(); break;

		case EDITOR_MENU_BIAS_P_001 : biasSelection(0.01f); break;
		case EDITOR_MENU_BIAS_P_01 :  biasSelection(0.1f); break;
		case EDITOR_MENU_BIAS_P_1:    biasSelection(1.0f); break;
		case EDITOR_MENU_BIAS_P_10:   biasSelection(10.0f); break;
		case EDITOR_MENU_BIAS_P_100:  biasSelection(100.0f); break;
		case EDITOR_MENU_BIAS_P_1000: biasSelection(1000.0f); break;
		case EDITOR_MENU_BIAS_N_001:  biasSelection(-0.01f); break;
		case EDITOR_MENU_BIAS_N_01:   biasSelection(-0.1f); break;
		case EDITOR_MENU_BIAS_N_1:    biasSelection(-1.0f); break;
		case EDITOR_MENU_BIAS_N_10:   biasSelection(-10.0f); break;
		case EDITOR_MENU_BIAS_N_100 : biasSelection(-100.0f); break;
		case EDITOR_MENU_BIAS_N_1000: biasSelection(-1000.0f); break;
		
		case EDITOR_MENU_INTERPOLATION : onInterpolation(); break;
		case EDITOR_MENU_ENTER_CURRENT_V : onEnterCurrentValue(); break;

		// View

		case EDITOR_MENU_PLAY : onPlay(); break;
		case EDITOR_MENU_ROWS_UP : onRowStep(-highlightRowStep , NO_SELECTION); break;
		case EDITOR_MENU_ROWS_DOWN : onRowStep(highlightRowStep , NO_SELECTION); break;
		case EDITOR_MENU_PREV_BOOKMARK : onBookmarkDir(ARROW_UP, NO_SELECTION); break;
		case EDITOR_MENU_NEXT_BOOKMARK : onBookmarkDir(ARROW_DOWN, NO_SELECTION); break;
		case EDITOR_MENU_FIRST_TRACK : onTrackSide(ARROW_LEFT, true, NO_SELECTION); break;
		case EDITOR_MENU_LAST_TRACK : onTrackSide(ARROW_RIGHT, true, NO_SELECTION); break;
		case EDITOR_MENU_PREV_KEY : onPrevNextKey(true, NO_SELECTION); break;
		case EDITOR_MENU_NEXT_KEY : onPrevNextKey(false, NO_SELECTION); break;
		case EDITOR_MENU_FOLD_TRACK : onFoldTrack(true); break;
		case EDITOR_MENU_UNFOLD_TRACK : onFoldTrack(false); break;
		case EDITOR_MENU_FOLD_GROUP : onFoldGroup(true); break;
		case EDITOR_MENU_UNFOLD_GROUP : onFoldGroup(false); break;
		case EDITOR_MENU_TOGGLE_BOOKMARK : onToggleBookmark(); break;
		case EDITOR_MENU_CLEAR_BOOKMARKS : onClearBookmarks(); break;
		case EDITOR_MENU_TAB : onTab(); break;
	}

	Editor_update();
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void Editor_destroy()
{
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static bool doEditing(int key)
{
	// special case if '.' key (in case of dvorak) would clatch with the same key for biasing we do this special case
	
	if ((key == '.' || key == EMGUI_KEY_BACKSPACE) && !is_editing)
		return false;

	if ((key >= '0' && key <= '9') || key == '.' || key == '-' || key == EMGUI_KEY_BACKSPACE)
	{
		if (!is_editing)
		{
			memset(s_editBuffer, 0, sizeof(s_editBuffer));
			is_editing = true;
		}

		if (key == EMGUI_KEY_BACKSPACE)
			s_editBuffer[emaxi(strlen(s_editBuffer) - 1, 0)] = 0;
		else
			s_editBuffer[strlen(s_editBuffer)] = (char)key;

		s_editorData.trackData.editText = s_editBuffer;

		return true;
	}

	return false;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// This code does handling of keys that isn't of the regular accelerators. This may be due to its being impractical
// to have in the menus or for other reasons (example you can have arrows in the menus but should you have diffrent
// ones when you press shift (making a selection) it all becomes a bit bloated so we leave some out.
//
// Also we assume here that keys that has been assigned to accelerators has been handled before this code
// as it reduces some checks an makes the configuration stay in Menu.h/Menu.c

bool Editor_keyDown(int key, int keyCode, int modifiers)
{
	enum Selection selection = modifiers & EMGUI_KEY_SHIFT ? DO_SELECTION : NO_SELECTION;
	int highlightRowStep = getTrackData()->highlightRowStep; 

	if (Emgui_hasKeyboardFocus())
	{
		Editor_update();
		return true;
	}

	// Update editing of values

	if (doEditing(key))
		return true;

	switch (key)
	{
		case EMGUI_KEY_ARROW_UP :
		case EMGUI_KEY_ARROW_DOWN :
		case EMGUI_KEY_ARROW_LEFT : 
		case EMGUI_KEY_ARROW_RIGHT : 
		case EMGUI_KEY_TAB : 
		case EMGUI_KEY_ENTER : 
		{
            if (is_editing)
            {
                endEditing();
                
                if (key == EMGUI_KEY_ENTER)
                    return true;
            }
            
			break;
		}

		case EMGUI_KEY_ESC :
		{
			cancelEditing();
			break;
		}
	}

	switch (key)
	{
		// this is a bit hacky but we don't want to have lots of combos in the menus
		// of arrow keys so this makes it a bit easier
	
		case EMGUI_KEY_ARROW_UP : 
		{
			if (modifiers & EMGUI_KEY_CTRL)
				onPrevNextKey(true, selection);
			else if (modifiers & EMGUI_KEY_COMMAND)
				onBookmarkDir(ARROW_UP, selection);
			else if (modifiers & EMGUI_KEY_ALT)
				onRowStep(-highlightRowStep , selection); 
			else
				onRowStep(-1, selection); 

			break;
		}

		case EMGUI_KEY_ARROW_DOWN : 
		{
			if (modifiers & EMGUI_KEY_CTRL)
				onPrevNextKey(false, selection);
			else if (modifiers & EMGUI_KEY_COMMAND)
				onBookmarkDir(ARROW_DOWN, selection);
			else if (modifiers & EMGUI_KEY_ALT)
				onRowStep(highlightRowStep, selection); 
			else
				onRowStep(1, selection); 
			break;
		}

		case EMGUI_KEY_ARROW_LEFT : onTrackSide(ARROW_LEFT, false, selection); break;
		case EMGUI_KEY_ARROW_RIGHT : onTrackSide(ARROW_RIGHT, false, selection); break;
		case EMGUI_KEY_TAB : onTab(); break;
		default : return false;
	}

	return true;
}

