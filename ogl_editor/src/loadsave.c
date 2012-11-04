#include "LoadSave.h"
#include "Dialog.h"
#include "TrackData.h"
#include "External/mxml/mxml.h"
#include "RemoteConnection.h"
#include "../../sync/data.h"
#include <Types.h>
#include <stdio.h>
#include <stdlib.h>

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static void parseXml(mxml_node_t* rootNode, TrackData* trackData)
{
	int track_index = 0;
	//struct sync_track** tracks = trackData->syncData.tracks;
	mxml_node_t* node = rootNode;

	// find the tracks element
	
	/*
	mxml_node_t* node = mxmlFindElement(rootNode, rootNode, "tracks", NULL, NULL, MXML_NO_DESCEND);

	if (!node)
	{
		node = mxmlFindElement(rootNode, rootNode, "tracks", NULL, NULL, MXML_DESCEND_FIRST);

		if (!node)
		{
			// TODO: Report back that we couldn't find tracks in xml file
			// Dialog_showError(...)
		
			printf("No tracks found\n");

			return;
		}
	}
	*/
	


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

				if (!strcmp("track", element_name))
				{
					int i;

					// TODO: Create the new track/channel here
			
                    const char* track_name = mxmlElementGetAttr(node, "name");
                    
					track_index = TrackData_createGetTrack(trackData, track_name);
					printf("track_index %d\n", track_index);

					struct sync_track* track = trackData->syncData.tracks[track_index];

					// If we already have this track loaded we delete all the existing keys
					
					for (i = 0; i < track->num_keys; ++i)
					{
						int row = track->keys[i].row;
						RemoteConnection_sendDeleteKeyCommand(track->name, row);
					}

					free(track->keys);

					track->keys = 0;
					track->num_keys = 0;

					printf("Creating track/channel with name %s\n", track_name);
				}
				else if (!strcmp("key", element_name))
				{
					struct sync_track* track = trackData->syncData.tracks[track_index];

					const char* row = mxmlElementGetAttr(node, "row"); 
					const char* value = mxmlElementGetAttr(node, "value"); 
					const char* interpolation = mxmlElementGetAttr(node, "interpolation"); 

					struct track_key k;
					k.row = atoi(row);
					k.value = (float)(atof(value));
					k.type = (atoi(interpolation));

					int is_key = is_key_frame(track, k.row);

					assert(!is_key);
					sync_set_key(track, &k);

					printf("Adding key: row %s | value %s | interpolation %s\n", row, value, interpolation);
				}
			}

			default: break;
		}
	}
}


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

int LoadSave_loadRocketXML(const char* path, TrackData* trackData)
{
	FILE* fp = 0;
	mxml_node_t* tree = 0;

	if (!(fp = fopen(path, "r")))
		return false;

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

int LoadSave_loadRocketXMLDialog(TrackData* trackData)
{
	char path[512];

	if (!Dialog_open(path))
		return false;

	return LoadSave_loadRocketXML(path, trackData);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

int LoadSave_saveRocketXML(const char* path, TrackData* trackData)
{
	mxml_node_t* xml;
	mxml_node_t* tracks;
	
	struct sync_data* sync_data = &trackData->syncData;

	xml = mxmlNewXML("1.0");	
	tracks = mxmlNewElement(xml, "tracks");

	mxmlElementSetAttr(tracks, "rows", "1000000"); // TODO: Fix me

	for (size_t i = 0; i < sync_data->num_tracks; ++i) 
	{
		const struct sync_track* t = sync_data->tracks[i];
		mxml_node_t* track = mxmlNewElement(tracks, "track");
		mxmlElementSetAttr(track, "name", t->name); 

		for (int i = 0; i < (int)t->num_keys; ++i) 
		{
			char temp0[256];
			char temp1[256];
			char temp2[256];

			int row = (int)t->keys[i].row;
			float value = t->keys[i].value;
			char interpolationType = (char)t->keys[i].type;

			mxml_node_t* key = mxmlNewElement(track, "key");

			memset(temp0, 0, sizeof(temp0));
			memset(temp1, 0, sizeof(temp1));
			memset(temp2, 0, sizeof(temp2));

			sprintf(temp0, "%d", row); 
			sprintf(temp1, "%f", value); 
			sprintf(temp2, "%d", interpolationType); 

			mxmlElementSetAttr(key, "row", temp0); 
			mxmlElementSetAttr(key, "value", temp1); 
			mxmlElementSetAttr(key, "interpolation", temp2); 
		}
	}

	FILE* fp = fopen(path, "wt");
    mxmlSaveFile(xml, fp, MXML_NO_CALLBACK);
    fclose(fp);

    return true;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

int LoadSave_saveRocketXMLDialog(TrackData* trackData)
{
	char path[512];

	if (!Dialog_save(path))
		return false;


	return LoadSave_saveRocketXML(path, trackData);
}

