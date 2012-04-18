#include "RocketGui.h"
#include <string.h>

typedef struct RETrack
{
	char* name;
} RETrack;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

typedef struct Editor
{
	RETrack tracks[64]; // temporary max level
	int windowWidth;
	int windowHeight;
	int trackCount;
	int currentRow;
} Editor;

static Editor s_editor;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void Editor_init()
{
	memset(&s_editor, 0, sizeof(Editor));

	s_editor.tracks[0].name = strdup("DemoTrack");
	s_editor.tracks[1].name = strdup("CameraTestZ");
	s_editor.tracks[2].name = strdup("TestName 3");
	s_editor.tracks[3].name = strdup("TestName 4");
	s_editor.trackCount = 4;

	s_editor.windowWidth = 800;
	s_editor.windowHeight = 600;

	RocketGui_init();
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void Editor_guiUpdate()
{
	RocketGui_begin();

	RocketGui_beginHorizontalStackPanelXY(2, 2);

	if (RocketGui_button("PressMeNow!"))
	{
		RocketGui_textLabel("Pressed!");
	}
	else
	{
		RocketGui_textLabel("No Press0r!");
	}

	RocketGui_textLabel("Test");
	RocketGui_textLabel("Test2");
	RocketGui_textLabel("Test");

	RocketGui_fill(0xff0203ff, 10, 10, 100, 100);

	RocketGui_end();
}

