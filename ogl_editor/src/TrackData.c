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


