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
#include "MinecraftiaFont.h"
#include "../../sync/sync.h"
#include "../../sync/base.h"
#include "../../sync/data.h"

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

static EditorData s_editorData;
static CopyData s_copyData;
static bool reset_tracks = true;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static inline struct sync_track** getTracks()
{
	return s_editorData.trackData.syncData.tracks;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static inline void setActiveTrack(int track)
{
	const int current_track = s_editorData.trackData.activeTrack;
	s_editorData.trackData.tracks[current_track].selected = false;
	s_editorData.trackData.tracks[track].selected = true;
	s_editorData.trackData.activeTrack = track;
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
	memset(&s_editorData, 0, sizeof(s_editorData));

	RemoteConnection_createListner();

	s_editorData.trackViewInfo.smallFontId = id;
	s_editorData.trackViewInfo.endRow = 10000;

	Emgui_setDefaultFont();
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void Editor_setWindowSize(int x, int y)
{
	s_editorData.trackViewInfo.windowSizeX = x;
	s_editorData.trackViewInfo.windowSizeY = y;
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
	current_row = s_editorData.trackViewInfo.rowPos;

	if (tracks)
	{
		const struct sync_track* track = tracks[active_track];
		int row = s_editorData.trackViewInfo.rowPos;
		int idx = key_idx_floor(track, row);

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

		value = sync_get_val(track, row);
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

	Emgui_setFont(s_editorData.trackViewInfo.smallFontId);

	Emgui_fill(Emgui_color32(20, 20, 20, 255), 2, sizeY - 15, sizeX - 3, 13); 
	Emgui_drawBorder(Emgui_color32(10, 10, 10, 255), Emgui_color32(10, 10, 10, 255), 0, sizeY - 17, sizeX - 2, 15); 

	size = drawConnectionStatus(0, sizeY);
	size += drawCurrentValue(size, sizeY);
	size += drawNameValue("Row", size, sizeY, &s_editorData.trackViewInfo.rowPos, 0, 20000 - 1, s_currentRow);
	size += drawNameValue("Start Row", size, sizeY, &s_editorData.trackViewInfo.startRow, 0, 10000000, s_startRow);
	size += drawNameValue("End Row", size, sizeY, &s_editorData.trackViewInfo.endRow, 0, 10000000, s_endRow);

	Emgui_setDefaultFont();
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/*
static void drawHorizonalSlider()
{
	const int sizeY = s_editorData.trackViewInfo.windowSizeY;
	const int sizeX = s_editorData.trackViewInfo.windowSizeX;

	static int value = 0;

	Emgui_slider(sizeX - 20, 2, 14, sizeY - 20, 0, 40, EMGUI_SLIDERDIR_VERTICAL, 1, &value);
}
*/

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// In some cases we need an extra update in case some controls has been re-arranged in such fashion so
// the trackview will report back if that is needed (usually happens if tracks gets resized)

static bool internalUpdate()
{
	int refresh;

	Emgui_begin();
	drawStatus();
	//drawHorizonalSlider();

	refresh = TrackView_render(&s_editorData.trackViewInfo, &s_editorData.trackData);
	Emgui_end();

	return refresh; 
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void Editor_update()
{
	bool need_update = internalUpdate();

	if (need_update)
		internalUpdate();
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

			if (is_key_frame(t, row))
			{
				sync_del_key(t, row);
				RemoteConnection_sendDeleteKeyCommand(t->name, row);
			}
		}
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static char s_editBuffer[512];
static bool is_editing = false;

bool Editor_keyDown(int key, int modifiers)
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

	if (key == ' ')
	{
		// TODO: Don't start playing if we are in edit mode (but space shouldn't be added in edit mode but we still
		// shouldn't start playing if we do

		RemoteConnection_sendPauseCommand(!paused);
		Editor_update();
		return true;
	}

	if (!paused)
		return false;

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
			deleteArea(row_pos, active_track, 1, 1);
			break;
		}


		case EMGUI_ARROW_DOWN:
		{
			const int active_track = getActiveTrack();
			int row = row_pos;

			if (modifiers & EMGUI_KEY_CTRL)
			{
				struct sync_track* t = getTracks()[active_track];

				if (t->keys)
				{
					int idx = key_idx_floor(t, row_pos);

					if (idx < 0)
						row = t->keys[0].row;
					else if (idx > (int)t->num_keys - 2)
						row = t->keys[t->num_keys - 1].row;
					else
						row = t->keys[idx + 1].row;

					viewInfo->rowPos = row; 
				}

				break;
			}

			row += modifiers & EMGUI_KEY_ALT ? 8 : 1;

			if ((modifiers & EMGUI_KEY_COMMAND) || row > viewInfo->endRow)
				row = viewInfo->endRow;

			viewInfo->rowPos = row;

			if (modifiers & EMGUI_KEY_SHIFT)
			{
				viewInfo->selectStopRow = row;
				break;
			}

			viewInfo->selectStartRow = viewInfo->selectStopRow = row;
			viewInfo->selectStartTrack = viewInfo->selectStopTrack = active_track;

			RemoteConnection_sendSetRowCommand(row);

			break;
		}

		case EMGUI_ARROW_UP:
		{
			int row = row_pos;

			if (modifiers & EMGUI_KEY_CTRL)
			{
				struct sync_track* t = getTracks()[active_track];

				if (t->keys)
				{
					int idx = sync_find_key(t, row);
					if (idx < 0)
						idx = -idx - 1;

					viewInfo->rowPos = t->keys[emaxi(idx - 1, 0)].row;
				}

				break;
			}

			row -= modifiers & EMGUI_KEY_ALT ? 8 : 1;

			if ((modifiers & EMGUI_KEY_COMMAND) || row < viewInfo->startRow)
				row = viewInfo->startRow;

			viewInfo->rowPos = row;

			if (modifiers & EMGUI_KEY_SHIFT)
			{
				viewInfo->selectStopRow = row;
				break;
			}

			viewInfo->selectStartRow = viewInfo->selectStopRow = row;
			viewInfo->selectStartTrack = viewInfo->selectStopTrack = active_track;

			RemoteConnection_sendSetRowCommand(row);
			handled_key = true;

			break;
		}

		case EMGUI_ARROW_LEFT:
		{
			const int current_track = getActiveTrack();
			int track = getPrevTrack();

			if (modifiers & EMGUI_KEY_ALT)
			{
				Track* t = &trackData->tracks[current_track];

				if (modifiers & EMGUI_KEY_CTRL) 
				{
					if (t->group->trackCount > 1)
						t->group->folded = true;
				}
				else
					t->folded = true;

				Editor_update();
				return true;
			}

			if (modifiers & EMGUI_KEY_COMMAND)
				track = 0;

			setActiveTrack(track < 0 ? 0 : track);

			if (modifiers & EMGUI_KEY_SHIFT)
			{
				Track* t = &trackData->tracks[track];

				// if this track has a folded group we can't select it so set back the selection to the old one

				if (t->group->folded)
					setActiveTrack(current_track);
				else
					viewInfo->selectStopTrack = track;

				break;
			}

			viewInfo->selectStartRow = viewInfo->selectStopRow = row_pos;
			viewInfo->selectStartTrack = viewInfo->selectStopTrack = track;

			handled_key = true;

			break;
		}

		case EMGUI_ARROW_RIGHT:
		{
			const int current_track = getActiveTrack();
			int track = getNextTrack();
			int track_count = getTrackCount();

			if (modifiers & EMGUI_KEY_ALT)
			{
				Track* t = &trackData->tracks[current_track];

				if (modifiers & EMGUI_KEY_CTRL) 
				{
					if (t->group->trackCount > 1)
						t->group->folded = false;
				}
				else
					t->folded = false;

				Editor_update();
				return true;
			}

			if (track >= track_count) 
				track = track_count - 1;

			if (modifiers & EMGUI_KEY_COMMAND)
				track = track_count - 1;

			setActiveTrack(track);

			if (modifiers & EMGUI_KEY_SHIFT)
			{
				Track* t = &trackData->tracks[track];
				if (t->group->folded)
					setActiveTrack(current_track);
				else
					viewInfo->selectStopTrack = track;
				break;
			}

			viewInfo->selectStartRow = viewInfo->selectStopRow = row_pos;
			viewInfo->selectStartTrack = viewInfo->selectStopTrack = track;

			handled_key = true;

			break;
		}

		default : handled_key = false; break;
	}

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

		for (i = 0; i < buffer_size; ++i)
		{
			const CopyEntry* ce = &s_copyData.entries[i];
			
			assert(ce->track >= 0);
			assert(ce->track < buffer_width);
			assert(ce->keyFrame.row >= 0);
			assert(ce->keyFrame.row < buffer_height);

			trackPos = active_track + ce->track;
			if (trackPos < track_count)
			{
				size_t trackIndex = trackPos;
				struct track_key key = ce->keyFrame;
				key.row += row_pos;

				rlog(R_INFO, "key.row %d\n", key.row);

				sync_set_key(tracks[trackIndex], &key);

				RemoteConnection_sendSetKeyCommand(tracks[trackIndex]->name, &key);
			}
		}

		handled_key = true;
	}

	// Handle biasing of values

	if ((key >= '1' && key <= '9') && ((modifiers & EMGUI_KEY_CTRL) || (modifiers & EMGUI_KEY_ALT)))
	{
		struct sync_track** tracks;
		int track, row;

		float bias_value = 0.0f;
		tracks = getTracks();

		switch (key)
		{
			case '1' : bias_value = 0.01f; break;
			case '2' : bias_value = 0.1f; break;
			case '3' : bias_value = 1.0f; break;
			case '4' : bias_value = 10.f; break;
			case '5' : bias_value = 100.0f; break;
			case '6' : bias_value = 1000.0f; break;
			case '7' : bias_value = 10000.0f; break;
		}

		bias_value = modifiers & EMGUI_KEY_ALT ? -bias_value : bias_value;

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
				newKey.value += bias_value;

				sync_set_key(t, &newKey);

				RemoteConnection_sendSetKeyCommand(t->name, &newKey);
			}
		}

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
			const char* track_name;
			struct track_key key;
			struct sync_track* track = tracks[active_track];

			key.row = row_pos;
			key.value = (float)atof(s_editBuffer);
			key.type = 0;

			if (is_key_frame(track, row_pos))
			{
				int idx = key_idx_floor(track, row_pos);
				key.type = track->keys[idx].type;
			}

			track_name = track->name; 

			sync_set_key(track, &key);

			rlog(R_INFO, "Setting key %f at %d row %d (name %s)\n", key.value, active_track, key.row, track_name);

			RemoteConnection_sendSetKeyCommand(track_name, &key);
		}

		is_editing = false;
		s_editorData.trackData.editText = 0;
	}

	if (key == 'i')
	{
		struct track_key newKey;
		struct sync_track* track = tracks[active_track];
		int row = viewInfo->rowPos;

		int idx = key_idx_floor(track, row);
		if (idx < 0) 
			return false;

		// copy and modify
		newKey = track->keys[idx];
		newKey.type = ((newKey.type + 1) % KEY_TYPE_COUNT);

		sync_set_key(track, &newKey);

		RemoteConnection_sendSetKeyCommand(track->name, &newKey);

		handled_key = true;
	}

	if (handled_key)
		Editor_update();

	return handled_key;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void Editor_scroll(int deltaX, int deltaY)
{
	int i, current_row = s_editorData.trackViewInfo.rowPos;
	TrackViewInfo* viewInfo = &s_editorData.trackViewInfo;

	current_row += deltaY;

	s_editorData.trackViewInfo.rowPos = eclampi(current_row, viewInfo->startRow, viewInfo->endRow);

	if (deltaX < 0)
	{
		for (i = 0; i < -deltaX; ++i)
			setActiveTrack(getPrevTrack());
	}
	else if (deltaX > 0)
	{
		for (i = 0; i < deltaX; ++i)
			setActiveTrack(getNextTrack());
	}

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

	RemoteConnection_updateListner();

	updateTrackStatus();

	while (RemoteConnection_pollRead())
		processed_commands |= processCommands();

	if (!RemoteConnection_isPaused() || processed_commands)
	{
		Editor_update();
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static void onOpen()
{
	if (LoadSave_loadRocketXMLDialog(&s_editorData.trackData))
		Editor_update();
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static void onSave()
{
	LoadSave_saveRocketXMLDialog(&s_editorData.trackData);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void Editor_menuEvent(int menuItem)
{
	printf("%d\n", menuItem);
	switch (menuItem)
	{
		//case EDITOR_MENU_NEW : onNew(); break;
		case EDITOR_MENU_OPEN : onOpen(); break;
		case EDITOR_MENU_SAVE : 
		case EDITOR_MENU_SAVE_AS : onSave(); break;
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void Editor_destroy()
{
}

