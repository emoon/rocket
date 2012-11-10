#include "TrackData.h"

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

int TrackData_createGetTrack(TrackData* trackData, const char* name)
{
	int index = sync_find_track(&trackData->syncData, name); 
	if (index < 0)
	{
        index = sync_create_track(&trackData->syncData, name);
		trackData->order[trackData->orderCount] = index;
		trackData->orderCount++;
	}

	return index;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static uint32_t s_colors[] =
{
	0xffb27474,
	0xffb28050,
	0xffa9b250,
	0xff60b250,

	0xff4fb292,
	0xff4f71b2,
	0xff8850b2,
	0xffb25091,
};

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

uint32_t TrackData_getNextColor(TrackData* trackData)
{
	return s_colors[(trackData->lastColor++) & 0x7];
}

