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
#include "TrackData.h"
#include "RemoteConnection.h"
#include "MinecraftiaFont.h"
#include "../../sync/sync.h"
#include "../../sync/base.h"
#include "../../sync/data.h"

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#if defined(EMGUI_MACOSX)
#define FONT_PATH "/Library/Fonts/"
#elif defined(EMGUI_WINDOWS)
#define FONT_PATH "C:\\Windows\\Fonts\\"
#else
#error "Unsupported platform"
#endif

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

typedef struct EditorData
{
	TrackViewInfo trackViewInfo;
	TrackData trackData;
} EditorData;

static EditorData s_editorData;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static inline struct sync_track** getTracks()
{
	return s_editorData.trackData.syncData.tracks;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static inline void setActiveTrack(int track)
{
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

void Editor_create()
{
	int id;
	Emgui_create("foo");
	id = Emgui_loadFontBitmap(g_minecraftiaFont, g_minecraftiaFontSize, EMGUI_FONT_MEMORY, 32, 128, g_minecraftiaFontLayout);
	memset(&s_editorData, 0, sizeof(s_editorData));

	RemoteConnection_createListner();

	s_editorData.trackViewInfo.smallFontId = id;

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

static void drawStatus()
{
	char temp[256];
	struct sync_track** tracks = getTracks();

	if (!tracks)
		return;

	int active_track = getActiveTrack();
	const struct sync_track* track = tracks[active_track];
	const int sizeY = s_editorData.trackViewInfo.windowSizeY;
	int row = s_editorData.trackViewInfo.rowPos;
	int idx = key_idx_floor(track, row);
	const char *str = "---";
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

	snprintf(temp, 256, "track %d row %d value %f type %s", active_track, row, sync_get_val(track, row), str);

	Emgui_fill(Emgui_color32(0x10, 0x10, 0x10, 0xff), 1, sizeY - 12, 400, 11); 
	Emgui_drawText(temp, 3, sizeY - 10, Emgui_color32(255, 255, 255, 255));
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void Editor_update()
{
	Emgui_begin();

	TrackView_render(&s_editorData.trackViewInfo, &s_editorData.trackData);

	drawStatus();

	Emgui_end();
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static char s_editBuffer[512];
static bool is_editing = false;

bool Editor_keyDown(int key, int modifiers)
{
	bool handled_key = true;
	bool paused = RemoteConnection_isPaused();
	struct sync_track** tracks = getTracks();
	int active_track = getActiveTrack();
	int row_pos = s_editorData.trackViewInfo.rowPos;

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

	switch (key)
	{
		case EMGUI_ARROW_DOWN:
		{
			int row = row_pos;

			row += modifiers & EDITOR_KEY_ALT ? 8 : 1;
			s_editorData.trackViewInfo.rowPos = row;

			RemoteConnection_sendSetRowCommand(row);

			break;
		}

		case EMGUI_ARROW_UP:
		{
			int row = row_pos;

			row -= modifiers & EDITOR_KEY_ALT ? 8 : 1;

			if ((modifiers & EDITOR_KEY_COMMAND) || row < 0)
				row = 0;

			s_editorData.trackViewInfo.rowPos = row;

			RemoteConnection_sendSetRowCommand(row);
			handled_key = true;

			break;
		}

		case EMGUI_ARROW_LEFT:
		{
			int track = getActiveTrack() - 1;

			if (modifiers & EDITOR_KEY_COMMAND)
				track = 0;

			setActiveTrack(track < 0 ? 0 : track);

			handled_key = true;

			break;
		}

		case EMGUI_ARROW_RIGHT:
		{
			int track = getActiveTrack(); track++;
			int track_count = getTrackCount();

			if (track >= track_count) 
				track = track_count - 1;

			if (modifiers & EDITOR_KEY_COMMAND)
				track = track_count - 1;

			setActiveTrack(track);

			handled_key = true;

			break;
		}

		default : handled_key = false; break;
	}

	// Handle biasing of values

	if ((key >= '1' && key <= '9') && ((modifiers & EDITOR_KEY_CTRL) || (modifiers & EDITOR_KEY_ALT)))
	{
		struct sync_track* track = tracks[active_track];
		int row = s_editorData.trackViewInfo.rowPos;

		float bias_value = 0.0f;

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

		bias_value = modifiers & EDITOR_KEY_ALT ? -bias_value : bias_value;

		int idx = key_idx_floor(track, row);
		if (idx < 0) 
			return false;

		// copy and modify
		struct track_key newKey = track->keys[idx];
		newKey.value += bias_value;

		sync_set_key(track, &newKey);

		RemoteConnection_sendSetKeyCommand(track->name, &newKey);

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

		s_editBuffer[strlen(s_editBuffer)] = key;
		s_editorData.trackData.editText = s_editBuffer;

		return true;
	}
	else if (is_editing)
	{
		struct track_key key;

		key.row = row_pos;
		key.value = atof(s_editBuffer);
		key.type = 0;

		struct sync_track* track = tracks[active_track];
		const char* track_name = track->name; 

		sync_set_key(track, &key);

		rlog(R_INFO, "Setting key %f at %d row %d (name %s)\n", key.value, active_track, key.row, track_name);

		RemoteConnection_sendSetKeyCommand(track_name, &key);

		is_editing = false;
		s_editorData.trackData.editText = 0;
	}

	if (key == 'i')
	{
		struct sync_track* track = tracks[active_track];
		int row = s_editorData.trackViewInfo.rowPos;

		int idx = key_idx_floor(track, row);
		if (idx < 0) 
			return false;

		// copy and modify
		struct track_key newKey = track->keys[idx];
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
				ret = 1;

				break;
			}

			case SET_ROW:
			{
				RemoteConnection_recv((char*)&newRow, sizeof(int), 0);
				s_editorData.trackViewInfo.rowPos = htonl(newRow);
				rlog(R_INFO, "row from demo %d\n", s_editorData.trackViewInfo.rowPos); 
				ret = 1;
				break;
			}
		}
	}

	return ret;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void Editor_timedUpdate()
{
	int processed_commands = 0;

	RemoteConnection_updateListner();

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

