#include "LoadSave.h"
#include "Dialog.h"
#include "External/mxml/mxml.h"
#include <Types.h>

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static void parseXml(mxml_node_t* rootNode)
{
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

					printf("Creating track/channel with name %s\n", mxmlElementGetAttr(node, "name")); 
				}
				else if (!strcmp("key", element_name))
				{
					const char* row = mxmlElementGetAttr(node, "row"); 
					const char* value = mxmlElementGetAttr(node, "value"); 
					const char* interpolation = mxmlElementGetAttr(node, "interpolation"); 

					printf("Adding key: row %s | value %s | interpolation %s\n", row, value, interpolation);
				}
			}

			default: break;
		}
	}
}


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

int LoadSave_loadRocketXML(const char* path)
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

	parseXml(tree);

	fclose(fp);
	mxmlDelete(tree);

	return true;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

int LoadSave_loadRocketXMLDialog()
{
	char path[512];

	if (!Dialog_open(path))
		return false;

	return LoadSave_loadRocketXML(path);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

int LoadSave_saveRocketXML(const char* path)
{
	return false; 
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

int LoadSave_saveRocketXMLDialog()
{
	return false;
}

