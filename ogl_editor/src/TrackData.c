#include "TrackData.h"
#include "Commands.h"
#include "rlog.h"
#include <stdio.h>

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

int TrackData_createGetTrack(TrackData* trackData, const char* name)
{
	int index = sync_find_track(&trackData->syncData, name); 
	if (index < 0)
	{
        index = sync_create_track(&trackData->syncData, name);
		memset(&trackData->tracks[index], 0, sizeof(Track));
		trackData->tracks[index].index = index;
		trackData->tracks[index].color = TrackData_getNextColor(trackData); 
	}

	if (trackData->syncData.tracks)
		Commands_init(trackData->syncData.tracks, trackData);

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
	return s_colors[trackData->lastColor++ & 0x7];
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static int findSeparator(const char* name)
{
	int i, len = strlen(name);

	for (i = 0; i < len; ++i)
	{
		if (name[i] == ':' || name[i] == '#')
			return i;
	}

	return -1;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static Group* findOrCreateGroup(const char* name, TrackData* trackData)
{
	Group* group;
	int i, group_count = trackData->groupCount;
	Group* groups = trackData->groups;

	for (i = 0; i < group_count; ++i)
	{
		group = &groups[i];

		if (!group->name)
			continue;

		if (!strcmp(name, group->name))
			return &groups[i];
	}

	group = &groups[trackData->groupCount++];
	memset(group, 0, sizeof(Group));

	group->type = GROUP_TYPE_GROUP;
	group->name = strdup(name);
	group->displayName = strdup(name);
	group->displayName[strlen(name)-1] = 0;
	group->groupIndex = trackData->groupCount - 1;

	printf("creating group %s\n", name);

	return group;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void TrackData_linkTrack(int index, const char* name, TrackData* trackData)
{
	int found;
	char group_name[256];
	Group* group;
	Group* groups = trackData->groups;
	Track* track = &trackData->tracks[index];

	if (track->group)
		return;

	found = findSeparator(name);

	if (found == -1)
	{
		Group* group = &groups[trackData->groupCount++];
		memset(group, 0, sizeof(Group));
		
		group->tracks = (Track**)malloc(sizeof(Track**));
		group->tracks[0] = track;
		group->type = GROUP_TYPE_TRACK;
		group->trackCount = 1;
		track->group = group;
		track->displayName = strdup(name); 
		group->groupIndex = trackData->groupCount - 1;

		printf("Linking track %s to group %s\n", name, name); 

		return;
	}

	memset(group_name, 0, sizeof(group_name));
	memcpy(group_name, name, found + 1); 

	group = findOrCreateGroup(group_name, trackData); 

	if (group->trackCount == 0)
		group->tracks = (Track**)malloc(sizeof(Track**));
	else
		group->tracks = (Track**)realloc(group->tracks, sizeof(Track**) * (group->trackCount + 1));

	printf("Linking track %s to group %s\n", name, group_name); 

	track->groupIndex = group->trackCount;
	group->tracks[group->trackCount++] = track;

	track->group = group;
	track->displayName = strdup(&name[found + 1]);

	printf("groupDisplayName %s\n", group->displayName);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void TrackData_linkGroups(TrackData* trackData)
{
	int i, track_count;
	struct sync_data* sync = &trackData->syncData;

	for (i = 0, track_count = sync->num_tracks; i < track_count; ++i)
		TrackData_linkTrack(i, sync->tracks[i]->name, trackData); 
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void TrackData_setActiveTrack(TrackData* trackData, int track)
{
	const int current_track = trackData->activeTrack;
	trackData->tracks[current_track].selected = false;
	trackData->tracks[track].selected = true;
	trackData->activeTrack = track;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static bool hasMark(int* marks, int count, int row)
{
	int middle, first, last;

	if (!marks)
		return false;

	first = 0;
	last = count - 1;
	middle = (first + last) / 2;

	while (first <= last)
	{
		if (marks[middle] < row)
			first = middle + 1;    
		else if (marks[middle] == row) 
			return true;
		else
			last = middle - 1;

		middle = (first + last) / 2;
	}

	return false;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

int compare(const void* a, const void* b)
{
	return *(int*)a - *(int*)b;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static void sortArray(int* bookmarks, int count)
{
	qsort(bookmarks, count, sizeof(int), compare);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void toogleMark(int** marksPtr, int* countPtr, int row)
{
	int i;
	int* marks = *marksPtr; 
	int count = *countPtr;

	if (!marks)
	{
		*marksPtr = marks = malloc(sizeof(int));
		*marks = row;
		*countPtr = 1;
		return;
	}

	for (i = 0; i < count; ++i)
	{
		if (marks[i] == row)
		{
			marks[i] = 0;
			sortArray(marks, count);
			return;
		}
	}

	// look for empty slot

	for (i = 0; i < count; ++i)
	{
		if (marks[i] == 0)
		{
			marks[i] = row;
			sortArray(marks, count);
			return;
		}
	}

	// no slot found so we will resize the array and add the bookmark at the end
	
	*marksPtr = marks = realloc(marks, sizeof(int) * (count + 1));
	marks[count] = row;
	sortArray(marks, count + 1);

	*countPtr = count + 1;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static int getNextMark(const int* marks, int count, int row, int defValue)
{
	int i;

	if (!marks)
		return defValue;

	for (i = 0; i < count; ++i)
	{
		const int v = marks[i]; 

		if (v > row)
			return v;
	}

	return defValue;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static int getPrevMark(const int* marks, int count, int row, int defValue)
{
	int i;

	if (!marks)
		return defValue;

	for (i = count; i >= 0; --i)
	{
		const int v = marks[i]; 

		if (v < row)
			return v;
	}

	return defValue;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

bool TrackData_hasBookmark(TrackData* trackData, int row)
{
	return hasMark(trackData->bookmarks, trackData->bookmarkCount, row);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void TrackData_toggleBookmark(TrackData* trackData, int row)
{
	toogleMark(&trackData->bookmarks, &trackData->bookmarkCount, row);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

int TrackData_getNextBookmark(TrackData* trackData, int row)
{
	return getNextMark(trackData->bookmarks, trackData->bookmarkCount, row, trackData->endRow);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

int TrackData_getPrevBookmark(TrackData* trackData, int row)
{
	return getPrevMark(trackData->bookmarks, trackData->bookmarkCount, row, trackData->startRow);
}


