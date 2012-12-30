#pragma once

#include "Types.h"
#include "../../sync/data.h"

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

enum
{
	EDITOR_MAX_TRACKS = 16 * 1024,
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
	uint32_t color;

	int width;           // width in pixels of the track
	int index;
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

	bool folded;
	union
	{
		Track* track;
		Track** tracks;
	} t;

	enum GroupType type;
	int trackCount;
} Group;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

typedef struct TrackData
{
	struct sync_data syncData;
	Track tracks[EDITOR_MAX_TRACKS];
	Group groups[EDITOR_MAX_TRACKS];
	int groupCount;
	int activeTrack;
	int lastColor;
	int trackCount;
	int startRow;
	int endRow;
	char* editText;
} TrackData;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Will get the get the track if it exists else create it

int TrackData_createGetTrack(TrackData* trackData, const char* name);
uint32_t TrackData_getNextColor(TrackData* trackData);
void TrackData_linkGroups(TrackData* trackData);
void TrackData_linkTrack(int index, const char* name, TrackData* trackData);
void TrackData_setActiveTrack(TrackData* trackData, int track);

