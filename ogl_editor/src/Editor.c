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

//static uint64_t fontIds[2];

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void Editor_create()
{
	Emgui_create("foo");
	//fontIds[0] = Emgui_loadFont("/Users/daniel/Library/Fonts/MicroKnight_v1.0.ttf", 11.0f);
	//fontIds[0] = Emgui_loadFont(FONT_PATH "Arial.ttf", 11.0f);
	Emgui_setDefaultFont();

	memset(&s_editorData, 0, sizeof(s_editorData));

	RemoteConnection_createListner();
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

	Emgui_fill(Emgui_color32(0x10, 0x10, 0x10, 0xff), 1, 588, 400, 11); 
	Emgui_drawText(temp, 3, 590, Emgui_color32(255, 255, 255, 255));
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

	switch (key)
	{
		case EMGUI_ARROW_DOWN:
		{
			if (paused)
			{
				int row = row_pos;

				if (modifiers & EDITOR_KEY_ALT)
					row += 8;
				else
					row++;	

				s_editorData.trackViewInfo.rowPos = row;

				RemoteConnection_sendSetRowCommand(row);
				handled_key = true;
			}

			break;
		}

		case EMGUI_ARROW_UP:
		{
			if (paused)
			{
				int row = row_pos;

				if (modifiers & EDITOR_KEY_ALT)
					row -= 8;
				else
					row--;

				if (row < 0)
					row = 0;

				s_editorData.trackViewInfo.rowPos = row;

				RemoteConnection_sendSetRowCommand(row);
				handled_key = true;
			}
			break;
		}

		case EMGUI_ARROW_LEFT:
		{
			if (paused)
			{
				int track = getActiveTrack(); track--;
				setActiveTrack(track < 0 ? 0 : track);

				handled_key = true;
			}

			break;
		}

		case EMGUI_ARROW_RIGHT:
		{
			if (paused)
			{
				int track = getActiveTrack(); track++;

				if (track >= getTrackCount())
					track = getTrackCount() - 1;

				setActiveTrack(track);

				handled_key = true;
			}

			break;
		}

		default : handled_key = false; break;
	}

	// do edit here

	if (paused)
	{
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
	}

	if (key == ' ')
	{
		// TODO: Don't start playing if we are in edit mode (but space shouldn't be added in edit mode but we still
		// shouldn't start playing if we do

		RemoteConnection_sendPauseCommand(!paused);
		handled_key = true;
	}

	if (handled_key)
		Editor_update();

	return handled_key;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static int processCommands()
{
	//SyncDocument *doc = trackView->getDocument();
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

