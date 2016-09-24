#include "loadsave.h"
#include "Dialog.h"
#include "TrackData.h"
#include "../external/mxml/mxml.h"
#include "RemoteConnection.h"
#include <stdio.h>
#include <stdlib.h>
#include <emgui/Types.h>
#include <assert.h>

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static char* s_foldedGroupNames[16 * 1024];

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static void parseXml(mxml_node_t* rootNode, TrackData* trackData)
{
	struct track_key k;
	int g, i, foldedGroupCount = 0, is_key, track_index = 0;
	mxml_node_t* node = rootNode;

	free(trackData->bookmarks);
	free(trackData->loopmarks);

	trackData->bookmarks = NULL;
	trackData->bookmarkCount = 0;

	trackData->loopmarks = NULL;
	trackData->loopmarkCount = 0;

	trackData->rowsPerBeat = 8;

	// Traverse the tracks node data

	while (1)
	{
		node = mxmlWalkNext(node, rootNode, MXML_DESCEND);

		if (!node)
			break;

		switch (mxmlGetType(node))
		{
			case MXML_ELEMENT:
			{
				const char* element_name = mxmlGetElement(node);

				if (!strcmp("bookmark", element_name))
				{
					const char* row = mxmlElementGetAttr(node, "row");

					if (row)
						TrackData_toggleBookmark(trackData, atoi(row));
				}

				if (!strcmp("loopmark", element_name))
				{
					const char* row = mxmlElementGetAttr(node, "row");

					if (row)
						TrackData_toggleLoopmark(trackData, atoi(row));
				}

				if (!strcmp("group", element_name))
				{
					s_foldedGroupNames[foldedGroupCount++] = strdup(mxmlElementGetAttr(node, "name"));
				}

				if (!strcmp("tracks", element_name))
				{
					const char* start_row = mxmlElementGetAttr(node, "startRow");
					const char* end_row = mxmlElementGetAttr(node, "endRow");
					// same value but kept for compatability
					const char* rows_per_beat = mxmlElementGetAttr(node, "rowsPerBeat");
					const char* hlrow_step = mxmlElementGetAttr(node, "highlightRowStep");

					if (start_row)
						trackData->startRow = atoi(start_row);

					if (end_row)
						trackData->endRow = atoi(end_row);

					if (hlrow_step)
						trackData->rowsPerBeat = atoi(hlrow_step);

					if (rows_per_beat)
						trackData->rowsPerBeat = atoi(rows_per_beat);
				}

				if (!strcmp("track", element_name))
				{
					int i;
					struct sync_track* track;
					float muteValue = 0.0f;
					Track* t;

					// TODO: Create the new track/channel here

					const char* track_name = mxmlElementGetAttr(node, "name");
					const char* color_text = mxmlElementGetAttr(node, "color");
					const char* folded_text = mxmlElementGetAttr(node, "folded");
					const char* mute_key_count_text = mxmlElementGetAttr(node, "muteKeyCount");
					const char* mute_value_text = mxmlElementGetAttr(node, "muteValue");

					track_index = TrackData_createGetTrack(trackData, track_name);

					t = &trackData->tracks[track_index];
					track = trackData->syncTracks[track_index];

					if (!color_text && t->color == 0)
					{
						t->color = TrackData_getNextColor(trackData);
					}
					else
					{
						if (color_text)
							t->color = strtoul(color_text, 0, 16);
					}

					if (folded_text)
					{
						if (folded_text[0] == '1')
							t->folded = true;
					}

					if (mute_key_count_text)
					{
						int muteKeyCount = atoi(mute_key_count_text);
						if (muteKeyCount > 0)
						{
							t->disabled = true;
							t->muteKeyCount = muteKeyCount;
							t->muteKeyIter = 0;
							t->muteBackup = malloc(sizeof(struct track_key) * muteKeyCount);
						}
					}

					if (mute_value_text)
						muteValue = (float)atof(mute_value_text);

					// If we already have this track loaded we delete all the existing keys

					for (i = 0; i < track->num_keys; ++i)
					{
						int row = track->keys[i].row;
						RemoteConnection_sendDeleteKeyCommand(track->name, row);
					}

					free(track->keys);

					track->keys = 0;
					track->num_keys = 0;

					// add one key as the track is disabled/mute

					if (t->disabled)
					{
						k.row = 0;
						k.value = muteValue;
						k.type = 0;

						sync_set_key(track, &k);

						RemoteConnection_sendSetKeyCommand(track->name, &k);
					}
				}
				else if (!strcmp("key", element_name))
				{
					struct sync_track* track = trackData->syncTracks[track_index];
					Track* t = &trackData->tracks[track_index];

					const char* row = mxmlElementGetAttr(node, "row");
					const char* value = mxmlElementGetAttr(node, "value");
					const char* interpolation = mxmlElementGetAttr(node, "interpolation");

					k.row = atoi(row);
					k.value = (float)(atof(value));
					k.type = (atoi(interpolation));

					// if track is muted we load the data into the muted data and not the regular keys

					if (t->muteKeyCount > 0)
					{
						t->muteBackup[t->muteKeyIter++] = k;
					}
					else
					{
						is_key = is_key_frame(track, k.row);

						assert(!is_key);
						sync_set_key(track, &k);

						RemoteConnection_sendSetKeyCommand(track->name, &k);
					}
				}
			}

			default: break;
		}
	}

	TrackData_linkGroups(trackData);

	// Apply fold status on the groups

	for (i = 0; i < foldedGroupCount; ++i)
	{
		for (g = 0; g < trackData->groupCount; ++g)
		{
			Group* group = &trackData->groups[g];
			const char* groupName = group->name;
			const char* foldedName = s_foldedGroupNames[i];

			// groups with 1 track is handled as non-grouped

			if (group->trackCount == 1)
				continue;

			if (!strcmp(foldedName, groupName))
			{
				trackData->groups[g].folded = true;
				break;
			}
		}

		free(s_foldedGroupNames[i]);
	}

	trackData->tracks[0].selected = true;
}


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

int LoadSave_loadRocketXML(const text_t* path, TrackData* trackData)
{
	FILE* fp = 0;
	mxml_node_t* tree = 0;

#if defined(_WIN32)
	if (_wfopen_s(&fp, path, L"r") != 0)
		return false;
#else
	if (!(fp = fopen(path, "r")))
		return false;
#endif

	if (!(tree = mxmlLoadFile(NULL, fp, MXML_TEXT_CALLBACK)))
	{
		fclose(fp);
		return false;
	}

	parseXml(tree, trackData);

	fclose(fp);
	mxmlDelete(tree);

	return true;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

int LoadSave_loadRocketXMLDialog(text_t* path, TrackData* trackData)
{
	if (!Dialog_open(path))
		return false;

	return LoadSave_loadRocketXML(path, trackData);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static const char* whitespaceCallback(mxml_node_t* node, int where)
{
	const char* name = mxmlGetElement(node);

	if (where == MXML_WS_BEFORE_CLOSE)
	{
		if (!strcmp("tracks", name))
			return NULL;

		return "\t";
	}

	if (where == MXML_WS_AFTER_CLOSE)
		return "\n";

	if (where == MXML_WS_BEFORE_OPEN)
	{
		if (!strcmp("key", name))
			return "\t\t";

		if (!strcmp("tracks", name))
			return NULL;

		if (!strcmp("track", name))
			return "\t";

		if (!strcmp("group", name))
			return "\t";

		if (!strcmp("bookmark", name))
			return "\t";

		if (!strcmp("loopmark", name))
			return "\t";
	}

	if (where == MXML_WS_AFTER_OPEN)
		return "\n";

	return NULL;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static void setElementInt(mxml_node_t* node, const char* attr, const char* format, int v)
{
	char temp[256];
	memset(temp, 0, sizeof(temp));
	sprintf(temp, format, v);
	mxmlElementSetAttr(node, attr, temp);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static void setElementFloat(mxml_node_t* node, char* attr, float v)
{
	char temp[256];
	memset(temp, 0, sizeof(temp));
	sprintf(temp, "%f", v);
	mxmlElementSetAttr(node, attr, temp);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static void saveTrackData(mxml_node_t* track, struct track_key* keys, int count)
{
	int i;

	for (i = 0; i < count; ++i)
	{
		mxml_node_t* key = mxmlNewElement(track, "key");
		setElementInt(key, "row", "%d", keys[i].row);
		setElementFloat(key, "value", keys[i].value);
		setElementInt(key, "interpolation", "%d", (int)keys[i].type);
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

int LoadSave_saveRocketXML(const text_t* path, TrackData* trackData)
{
	mxml_node_t* xml;
	mxml_node_t* tracks;
	mxml_node_t* rootElement;

	FILE* fp;
	size_t p;
	int* bookmarks = trackData->bookmarks;
	int* loopmarks = trackData->loopmarks;

	xml = mxmlNewXML("1.0");
	rootElement = mxmlNewElement(xml, "rootElement");

	// save all bookmarks

	for (p = 0; p < (size_t)trackData->bookmarkCount; ++p)
	{
		mxml_node_t* node;
		const int bookmark = *bookmarks++;

		if (bookmark == 0)
			continue;

		node = mxmlNewElement(rootElement, "bookmark");
		setElementInt(node, "row", "%d", bookmark);
	}

	// save all loopmarks

	for (p = 0; p < (size_t)trackData->loopmarkCount; ++p)
	{
		mxml_node_t* node;
		const int loopmark = *loopmarks++;

		if (loopmark == 0)
			continue;

		node = mxmlNewElement(rootElement, "loopmark");
		setElementInt(node, "row", "%d", loopmark);
	}

	// save groups that are folded

	for (p = 0; p < (size_t)trackData->groupCount; ++p)
	{
		mxml_node_t* node;
		Group* group = &trackData->groups[p];

		if (!group->folded)
			continue;

		node = mxmlNewElement(rootElement, "group");
		mxmlElementSetAttr(node, "name", group->name);
	}

	tracks = mxmlNewElement(rootElement, "tracks");

	mxmlElementSetAttr(tracks, "rows", "10000");
	setElementInt(tracks, "startRow", "%d", trackData->startRow);
	setElementInt(tracks, "endRow", "%d", trackData->endRow);
	setElementInt(tracks, "rowsPerBeat", "%d", trackData->rowsPerBeat);

	for (p = 0; p < trackData->num_syncTracks; ++p)
	{
		const struct sync_track* t = trackData->syncTracks[p];
		mxml_node_t* track = mxmlNewElement(tracks, "track");

		bool isMuted = trackData->tracks[p].muteBackup ? true : false;

		mxmlElementSetAttr(track, "name", t->name);
		mxmlElementSetAttr(track, "folded", trackData->tracks[p].folded ? "1" : "0");
		setElementInt(track, "muteKeyCount", "%d", trackData->tracks[p].muteKeyCount);
		setElementInt(track, "color", "%08x", trackData->tracks[p].color);

		if (isMuted)
		{
			setElementFloat(track, "muteValue", t->keys[0].value);
			saveTrackData(track, trackData->tracks[p].muteBackup, trackData->tracks[p].muteKeyCount);
		}
		else
		{
			saveTrackData(track, t->keys, t->num_keys);
		}
	}

#if defined(_WIN32)
	_wfopen_s(&fp, path, L"wt");
#else
	fp = fopen(path, "wt");
#endif

	mxmlSaveFile(xml, fp, whitespaceCallback);
	fclose(fp);

	return true;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

int LoadSave_saveRocketXMLDialog(text_t* path, TrackData* trackData)
{
	if (!Dialog_save(path))
		return false;

	return LoadSave_saveRocketXML(path, trackData);
}
