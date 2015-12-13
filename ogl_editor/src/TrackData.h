#pragma once

#include <emgui/Types.h>
#include "../../lib/track.h"

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

enum
{
	EDITOR_MAX_TRACKS = 16 * 1024,
	EDITOR_MAX_BOOKMARKS = 32 * 1024,
};

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

enum GroupType
{
	GROUP_TYPE_TRACK,
	GROUP_TYPE_GROUP,
};

struct Group;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

typedef struct Track
{
	char* displayName;
	struct Group* group;
	struct track_key* muteBackup;
	uint32_t color;

	int width;           // width in pixels of the track
	int index;
	int groupIndex;
	int muteKeyCount;
	int muteKeyIter;
	bool disabled;
	bool hidden;
	bool folded;
	bool selected;
	bool active;

} Track;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Grouping (notice that even one track without group becomes it own group to keep the code easier)

typedef struct Group
{
	const char* name;
	char* displayName;
	int width;

	Track** tracks;

	enum GroupType type;
	int trackCount;
	int groupIndex;
	bool folded;
} Group;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

typedef struct TrackData
{
	struct sync_track **syncTracks;
	size_t num_syncTracks;
	Track tracks[EDITOR_MAX_TRACKS];
	Group groups[EDITOR_MAX_TRACKS];
	int* loopmarks;
	int* bookmarks;
	int bookmarkCount;
	int loopmarkCount;
	int groupCount;
	int activeTrack;
	int lastColor;
	int trackCount;
	int startRow;
	int endRow;
	int highlightRowStep;
	char* editText;
	bool isPlaying;
	bool isLooping;
	int startLoop;
	int endLoop;
} TrackData;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

bool TrackData_hasBookmark(TrackData* trackData, int row);
void TrackData_toggleBookmark(TrackData* trackData, int row);
int TrackData_getNextBookmark(TrackData* trackData, int row);
int TrackData_getPrevBookmark(TrackData* trackData, int row);

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

bool TrackData_hasLoopmark(TrackData* trackData, int row);
void TrackData_toggleLoopmark(TrackData* trackData, int row);
int TrackData_getNextLoopmark(TrackData* trackData, int row);
int TrackData_getPrevLoopmark(TrackData* trackData, int row);

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Will get the get the track if it exists else create it

int TrackData_createGetTrack(TrackData* trackData, const char* name);
uint32_t TrackData_getNextColor(TrackData* trackData);
void TrackData_linkGroups(TrackData* trackData);
void TrackData_linkTrack(int index, const char* name, TrackData* trackData);
void TrackData_setActiveTrack(TrackData* trackData, int track);
