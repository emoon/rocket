#include "LoadSave.h"
#include "Dialog.h"
#include "TrackData.h"
#include "External/mxml/mxml.h"
#include "../../sync/data.h"
#include <Types.h>

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static void parseXml(mxml_node_t* rootNode, TrackData* trackData)
{
	int track_index = 0;
	//struct sync_track** tracks = trackData->syncData.tracks;

	// find the tracks element
	
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
					// TODO: Create the new track/channel here
			
					track_index = TrackData_createGetTrack(trackData, element_name);
					printf("track_index %d\n", track_index);

					printf("Creating track/channel with name %s\n", mxmlElementGetAttr(node, "name")); 
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

					assert(!is_key_frame(track, k.row));
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
	/*
	char path[512];

	if (!Dialog_save(path))
		return false;
	*/

	return false; 
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

int LoadSave_saveRocketXMLDialog(TrackData* trackData)
{
	return false;
}

