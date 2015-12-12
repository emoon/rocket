#include "Commands.h"
#include "RemoteConnection.h"
#include "TrackData.h"
#include "TrackView.h"
#include "../../lib/sync.h"
#include "../../lib/track.h"
#include <stdlib.h>
#include <stdio.h>
#include <emgui/Types.h>

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static struct sync_track** s_syncTracks;
static struct TrackData* s_trackData;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

typedef struct Command
{
	void* userData;
	void (*exec)(void* userData);
	void (*undo)(void* userData);
	struct Command* next;
	struct Command* prev;
} Command;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

typedef struct CommandList
{
	Command* first;
	Command* last;
} CommandList;

static CommandList s_undoStack;
static CommandList s_redoStack;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static void CommandList_addEntry(CommandList* commandList, Command* command);
//static void CommandList_delEntry(CommandList* commandList, Command* command);
static void CommandList_clear(CommandList* commandList);
static bool CommandList_isEmpty(CommandList* list);
static void CommandList_pop(CommandList* commandList);

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

struct MultiCommandData
{
	CommandList list;
};

static struct MultiCommandData* s_multiCommand = 0;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void Commands_init(struct sync_track** syncTracks, struct TrackData* trackData)
{
	s_syncTracks = syncTracks;
	s_trackData = trackData;

	memset(&s_undoStack, 0, sizeof(CommandList));
	memset(&s_redoStack, 0, sizeof(CommandList));
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static int countEntriesInList(CommandList* list)
{
	Command* command;
	int count = 0;

	for (command = list->first; command; command = command->next) 
		count++;

	return count;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

int Commands_undoCount()
{
	return countEntriesInList(&s_undoStack);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static void execCommand(Command* command)
{
	// set if we have multi command recording enabled)
	if (s_multiCommand)
	{
		CommandList_addEntry(&s_multiCommand->list, command);
	}
	else
	{
		CommandList_addEntry(&s_undoStack, command);
		command->exec(command->userData);
	}

	CommandList_clear(&s_redoStack);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static void execMultiCommand(void* userData)
{
	Command* command;
	struct MultiCommandData* data = (struct MultiCommandData*)userData;

	for (command = data->list.first; command; command = command->next) 
		command->exec(command->userData);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static void undoMultiCommand(void* userData)
{
	Command* command;
	struct MultiCommandData* data = (struct MultiCommandData*)userData;

	for (command = data->list.first; command; command = command->next) 
		command->undo(command->userData);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void Commands_beginMulti(const char* name)
{
	s_multiCommand = malloc(sizeof(struct MultiCommandData));
	memset(s_multiCommand, 0, sizeof(struct MultiCommandData)); 
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void Commands_endMulti()
{
	Command* command;

	// Check if any command was added during multi command

	if (CommandList_isEmpty(&s_multiCommand->list))
	{
		free(s_multiCommand);
		s_multiCommand = 0;
		return;
	}

	command = malloc(sizeof(Command));
	memset(command, 0, sizeof(Command));

	command->userData = s_multiCommand; 
	command->exec = execMultiCommand;
	command->undo = undoMultiCommand;

	s_multiCommand = 0;

	execCommand(command);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

struct DeleteKeyData
{
	int track;
	int row;
	int noDelete;
	struct track_key oldKey;
};

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static void execDeleteKey(void* userData)
{
	struct DeleteKeyData* data = (struct DeleteKeyData*)userData;
	struct sync_track* t = s_syncTracks[data->track];
	int idx = sync_find_key(t, data->row);

	if (idx <= -1)
	{
		data->noDelete = 1;
		return;
	}

	data->oldKey = t->keys[idx];
	sync_del_key(t, data->row);

	RemoteConnection_sendDeleteKeyCommand(t->name, data->row);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static void undoDeleteKey(void* userData)
{
	struct DeleteKeyData* data = (struct DeleteKeyData*)userData;
	struct sync_track* t = s_syncTracks[data->track];

	if (data->noDelete)
		return;

	sync_set_key(t, &data->oldKey);

	RemoteConnection_sendSetKeyCommand(t->name, &data->oldKey);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void Commands_deleteKey(int track, int row)
{
	struct DeleteKeyData* data;
	Command* command;
	struct sync_track* t = s_syncTracks[track];
	
	if (!is_key_frame(t, row))
		return;
	
	command = malloc(sizeof(Command));
	memset(command, 0, sizeof(Command));

	command->userData = data = malloc(sizeof(struct DeleteKeyData));
	command->exec = execDeleteKey;
	command->undo = undoDeleteKey;
	data->track = track;
	data->row = row;
	data->noDelete = 0;

	execCommand(command);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

struct UpdateKeyData
{
	int track;
	struct track_key key;
	struct track_key oldKey;
};

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static void execUpdateKey(void* userData)
{
	struct UpdateKeyData* data = (struct UpdateKeyData*)userData;
	struct sync_track* t = s_syncTracks[data->track];
	int idx = sync_find_key(t, data->key.row);

	if (idx == -1)
		return;

	data->oldKey = t->keys[idx];
	sync_set_key(t, &data->key);

	RemoteConnection_sendSetKeyCommand(t->name, &data->key);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static void undoUpdateKey(void* userData)
{
	struct UpdateKeyData* data = (struct UpdateKeyData*)userData;
	struct sync_track* t = s_syncTracks[data->track];
	sync_set_key(t, &data->oldKey);

	RemoteConnection_sendSetKeyCommand(t->name, &data->oldKey);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void Commands_updateKey(int track, struct track_key* key)
{
	struct UpdateKeyData* data;
	Command* command;
	
	command = malloc(sizeof(Command));
	memset(command, 0, sizeof(Command));

	command->userData = data = malloc(sizeof(struct UpdateKeyData));
	command->exec = execUpdateKey;
	command->undo = undoUpdateKey;
	data->track = track;
	data->key = *key;

	execCommand(command);
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////

struct ChangeSelectionData
{
	struct TrackViewInfo* viewInfo;
	struct TrackViewInfo oldViewInfo;

	int selectStartTrack;
	int selectStopTrack;
	int selectStartRow;
	int selectStopRow;
};

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static void execChangeSelection(void* userData)
{
	struct ChangeSelectionData* data = (struct ChangeSelectionData*)userData;

	data->viewInfo->selectStartTrack = data->selectStartTrack;
	data->viewInfo->selectStopTrack = data->selectStopTrack;
	data->viewInfo->selectStartRow = data->selectStartRow;
	data->viewInfo->selectStopRow = data->selectStopRow;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static void execUndoSelection(void* userData)
{
	struct ChangeSelectionData* data = (struct ChangeSelectionData*)userData;

	data->viewInfo->selectStartTrack = data->oldViewInfo.selectStartTrack;
	data->viewInfo->selectStopTrack = data->oldViewInfo.selectStopTrack;
	data->viewInfo->selectStartRow = data->oldViewInfo.selectStartRow;
	data->viewInfo->selectStopRow = data->oldViewInfo.selectStopRow;
	data->viewInfo->rowPos = data->oldViewInfo.rowPos;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void Commands_setSelection(struct TrackViewInfo* viewInfo, int startTrack, int endTrack, int startRow, int endRow)
{
	struct ChangeSelectionData* data;
	Command* command;
	
	command = malloc(sizeof(Command));
	memset(command, 0, sizeof(Command));

	command->userData = data = malloc(sizeof(struct ChangeSelectionData));
	command->exec = execChangeSelection;
	command->undo = execUndoSelection;

	data->viewInfo = viewInfo;
	data->oldViewInfo = *viewInfo;

	data->selectStartTrack = startTrack;
	data->selectStopTrack = endTrack;
	data->selectStartRow = startRow;
	data->selectStopRow = endRow;

	execCommand(command);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

struct InsertKeyData
{
	int track;
	struct track_key key;
};

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static void execInsertKey(void* userData)
{
	struct InsertKeyData* data = (struct InsertKeyData*)userData;
	struct sync_track* t = s_syncTracks[data->track];
	sync_set_key(t, &data->key);

	RemoteConnection_sendSetKeyCommand(t->name, &data->key);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static void undoInsertKey(void* userData)
{
	struct InsertKeyData* data = (struct InsertKeyData*)userData;
	struct sync_track* t = s_syncTracks[data->track];
	sync_del_key(t, data->key.row);

	RemoteConnection_sendDeleteKeyCommand(t->name, data->key.row);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void Commands_addOrUpdateKey(int track, struct track_key* key)
{
	struct InsertKeyData* data;
	Command* command;
	struct sync_track* t = s_syncTracks[track];

	if (is_key_frame(t, key->row))
	{
		Commands_updateKey(track, key);
		return;
	}

	command = malloc(sizeof(Command));
	memset(command, 0, sizeof(Command));

	command->userData = data = malloc(sizeof(struct InsertKeyData));
	command->exec = execInsertKey;
	command->undo = undoInsertKey;
	data->track = track;
	data->key = *key;

	execCommand(command);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void Commands_addKey(int track, struct track_key* key)
{
	struct InsertKeyData* data;
	Command* command;

	command = malloc(sizeof(Command));
	memset(command, 0, sizeof(Command));

	command->userData = data = malloc(sizeof(struct InsertKeyData));
	command->exec = execInsertKey;
	command->undo = undoInsertKey;
	data->track = track;
	data->key = *key;

	execCommand(command);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

struct BookmarkData
{
	struct TrackData* trackData;
	int row;
};

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static void toggleBookmark(void* userData)
{
	struct BookmarkData* data = (struct BookmarkData*)userData;
	TrackData_toggleBookmark(data->trackData, data->row);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void Commands_toggleBookmark(TrackData* trackData, int row)
{
	struct BookmarkData* data;
	Command* command;

	command = malloc(sizeof(Command));
	memset(command, 0, sizeof(Command));

	command->userData = data = malloc(sizeof(struct BookmarkData));
	command->exec = toggleBookmark;
	command->undo = toggleBookmark;
	data->trackData = trackData;
	data->row = row;

	execCommand(command);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void Commands_clearBookmarks(TrackData* trackData)
{
	int i, bookmarkCount = trackData->bookmarkCount;
	int* bookmarks = trackData->bookmarks;

	if (trackData->bookmarkCount == 0)
		return;

	Commands_beginMulti("clearBookmarks");

	for (i = 0; i < bookmarkCount; ++i)
	{
		const int bookmark = *bookmarks++;

		if (bookmark == 0)
			continue;

		Commands_toggleBookmark(trackData, bookmark);
	}

	Commands_endMulti();
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

struct LoopmarkData
{
	struct TrackData* trackData;
	int row;
};

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static void toggleLoopmark(void* userData)
{
	struct LoopmarkData* data = (struct LoopmarkData*)userData;
	TrackData_toggleLoopmark(data->trackData, data->row);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void Commands_toggleLoopmark(TrackData* trackData, int row)
{
	struct LoopmarkData* data;
	Command* command;

	command = malloc(sizeof(Command));
	memset(command, 0, sizeof(Command));

	command->userData = data = malloc(sizeof(struct LoopmarkData));
	command->exec = toggleLoopmark;
	command->undo = toggleLoopmark;
	data->trackData = trackData;
	data->row = row;

	execCommand(command);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void Commands_clearLoopmarks(TrackData* trackData)
{
	int i, loopmarkCount = trackData->loopmarkCount;
	int* loopmarks = trackData->loopmarks;

	if (trackData->loopmarkCount == 0)
		return;

	Commands_beginMulti("clearLoopmarks");

	for (i = 0; i < loopmarkCount; ++i)
	{
		const int loopmark = *loopmarks++;

		if (loopmark == 0)
			continue;

		Commands_toggleLoopmark(trackData, loopmark);
	}

	Commands_endMulti();
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

struct MuteData
{
	Track* track;
	struct sync_track* syncTrack;
	int row;
};

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static void toggleMute(void* userData)
{
	struct MuteData* data = (struct MuteData*)userData;

	// if we have mute data we should toggle back the track to it's old form

	if (data->track->muteBackup)
	{
		int i;

		sync_del_key(data->syncTrack, 0);
		RemoteConnection_sendDeleteKeyCommand(data->syncTrack->name, 0);

		for (i = 0; i < data->track->muteKeyCount; ++i)
		{
			struct track_key* key = &data->track->muteBackup[i];

			sync_set_key(data->syncTrack, key);
			RemoteConnection_sendSetKeyCommand(data->syncTrack->name, key);
		}

		data->track->disabled = false;

		free(data->track->muteBackup);

		data->track->muteBackup = 0;
		data->track->muteKeyCount = 0;
	}
	else
	{
		struct track_key defKey;
		int i, keysSize = sizeof(struct track_key) * data->syncTrack->num_keys;
		float currentValue = (float)sync_get_val(data->syncTrack, data->row);

		data->track->disabled = true;

		// No muteBackup, this means that we want to mute the channel

		data->track->muteBackup = malloc(keysSize);
		data->track->muteKeyCount = data->syncTrack->num_keys;

		memcpy(data->track->muteBackup, data->syncTrack->keys, keysSize);

		for (i = 0; i < data->syncTrack->num_keys; ++i)
		{
			int row = data->track->muteBackup[i].row;

			sync_del_key(data->syncTrack, row);
			RemoteConnection_sendDeleteKeyCommand(data->syncTrack->name, row);
		}

		defKey.row = 0;
		defKey.value = currentValue;
		defKey.type = KEY_STEP;

		// insert key with the current value

		sync_set_key(data->syncTrack, &defKey);
		RemoteConnection_sendSetKeyCommand(data->syncTrack->name, &defKey);
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void Commands_toggleMute(struct Track* track, struct sync_track* syncTrack, int row)
{
	struct MuteData* data;
	Command* command;

	command = malloc(sizeof(Command));
	memset(command, 0, sizeof(Command));

	command->userData = data = malloc(sizeof(struct MuteData));
	command->exec = toggleMute;
	command->undo = toggleMute;

	data->track = track;
	data->syncTrack = syncTrack;
	data->row = row;

	execCommand(command);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void Commands_undo()
{
	Command* command;

	if (CommandList_isEmpty(&s_undoStack))
	{
		return;
	}

	command = s_undoStack.last;
	CommandList_pop(&s_undoStack);

	command->prev = 0;
	command->next = 0;

	CommandList_addEntry(&s_redoStack, command);

	command->undo(command->userData);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void Commands_redo()
{
	Command* command;

	if (CommandList_isEmpty(&s_redoStack))
		return;

	command = s_redoStack.last;
	CommandList_pop(&s_redoStack);

	command->prev = 0;
	command->next = 0;

	CommandList_addEntry(&s_undoStack, command);

	command->exec(command->userData);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static void CommandList_addEntry(CommandList* list, Command* command)
{
	if (list->last) 
	{
		list->last->next = command;
		command->prev = list->last;
		list->last = command;
	}
	else 
	{
		list->first = command;
		list->last = command;
	}	
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static void CommandList_unlinkEntry(CommandList* list, Command* command)
{
	Command* prev;
	Command* next;

    prev = command->prev;
    next = command->next;

    if (prev) 
    {
        if (next) 
        {
            prev->next = next;
            next->prev = prev;
        }
        else 
        {
            prev->next = 0;
            list->last = prev;
        }
    }
    else 
    {
        if (next) 
        {
            next->prev = 0;
            list->first = next;
        }
        else 
        {
            list->first = 0;
            list->last = 0;
        }
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static void CommandList_delEntry(CommandList* list, Command* command)
{
	CommandList_unlinkEntry(list, command);

	free(command->userData);
    free(command);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static void CommandList_clear(CommandList* list)
{
	while (list->last)
	{
		CommandList_delEntry(list, list->last);
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static void CommandList_pop(CommandList* list)
{
	CommandList_unlinkEntry(list, list->last);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static bool CommandList_isEmpty(CommandList* list)
{
	return (!list->first && !list->last);
}


