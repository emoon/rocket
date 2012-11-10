#pragma once

#include "Types.h"
#include "../../sync/data.h"

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

enum
{
	EDITOR_MAX_TRACKS = 16 * 1024,
};

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

typedef struct TrackData
{
	struct sync_data syncData;
	uint32_t colors[EDITOR_MAX_TRACKS];
	bool folded[EDITOR_MAX_TRACKS];
	bool hidden[EDITOR_MAX_TRACKS];
	int order[EDITOR_MAX_TRACKS];
	int orderCount;
	int activeTrack;
	int lastColor;
	char* editText;
} TrackData;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Will get the get the track if it exists else create it

int TrackData_createGetTrack(TrackData* trackData, const char* name);
uint32_t TrackData_getNextColor(TrackData* trackData);

