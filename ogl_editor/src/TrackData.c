#include "TrackData.h"
#include "rlog.h"

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

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static int findSeparator(const char* name)
{
	int i, len = strlen(name);

	for (i = 0; i < len; ++i)
	{
		if (name[i] == ':')
			return i;
	}

	return -1;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static int countGroup(const char* name, struct sync_data* syncData, int index)
{
	int i, group_count = 0, count = syncData->num_tracks;

	for (i = index; i < count; ++i)
	{
		if (strstr(syncData->tracks[i]->name, name))
			group_count++;
	}

	return group_count;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static void insertTracksInGroup(Group* group, const char* name, bool* processedTracks, TrackData* trackData, int index)
{
	int i, group_index = 0, count = trackData->syncData.num_tracks;
	struct sync_data* sync = &trackData->syncData;

	for (i = index; i < count; ++i)
	{
		char* split_name;

		if (processedTracks[i])
			continue;

		if ((split_name = strstr(sync->tracks[i]->name, name)))
		{
			Track* t = &trackData->tracks[i];
			int sep = findSeparator(sync->tracks[i]->name);
			rlog(R_DEBUG, "Inserted track %s into group %s (%s)\n", sync->tracks[i]->name, group->name, split_name);
			t->displayName = strdup(&split_name[sep + 1]);
			t->group = group; 
			group->t.tracks[group_index++] = t;
			processedTracks[i] = true;
		}
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void TrackData_linkGroups(TrackData* trackData)
{
	int i, found, current_group = 0, track_count;
	char group_name[256];
	bool processed_tracks[EDITOR_MAX_TRACKS];
	struct sync_data* sync = &trackData->syncData;

	// set whatever we have processed a track or not

	memset(processed_tracks, 0, sizeof(processed_tracks));

	trackData->groupCount = 0;

	if (!sync->tracks)
		return;

	for (i = 0, track_count = sync->num_tracks; i < track_count; ++i)
	{
		int group_count;
		const char* track_name = sync->tracks[i]->name;
		Group* group = &trackData->groups[current_group];

		if (processed_tracks[i])
			continue;

		Track* track = &trackData->tracks[i];
		found = findSeparator(track_name);

		if (found == -1)
		{
			rlog(R_DEBUG, "Track %s didn't have any group. Adding as single track\n", track_name);
			group->type = GROUP_TYPE_TRACK;
			group->t.track = track;
			group->trackCount = 1;
			processed_tracks[i] = true;
			current_group++;
			continue;
		}

		rlog(R_DEBUG, "Found track with grouping %s\n", track_name);

		// Found a group, lets dig out the groupname

		memset(group_name, 0, sizeof(group_name));
		memcpy(group_name, track_name, found + 1); 

		group->name = strdup(group_name);
		group->displayName = strdup(group_name);
		group->type = GROUP_TYPE_GROUP;
		group->displayName[found] = 0;

		rlog(R_DEBUG, "Group name %s\n", group_name);

		// count tracks that are in the group and allocate space for them

		group_count = countGroup(group_name, sync, i);

		rlog(R_DEBUG, "Found %d tracks for group %s\n", group_count, group_name);

		group->t.tracks = (Track**)malloc(sizeof(Track**) * group_count);
		group->trackCount = group_count;

		insertTracksInGroup(group, group_name, processed_tracks, trackData, i);

		current_group++;
	}

	trackData->groupCount = current_group;

	rlog(R_DEBUG, "Total amount of groups (and separate tracks) %d\n", current_group);
}

