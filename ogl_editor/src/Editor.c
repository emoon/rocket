#include <string.h>
#include <Emgui.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "Dialog.h"
#include "Editor.h"
#include "LoadSave.h"

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

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#if defined(EMGUI_MACOSX)
#define FONT_PATH "/Library/Fonts/"
#elif defined(EMGUI_WINDOWS)
#define FONT_PATH "C:\\Windows\\Fonts\\"
#else
#error "Unsupported platform"
#endif

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

typedef struct Channel
{
	char name[512];
	float values[1000];
	bool set[1000];
	int maxRange;

} Channel;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static uint64_t fontIds[2];
const int font_size = 12;
int start_pos = -19;
static Channel s_testChannel;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void Editor_create()
{
	Emgui_create("foo");
	fontIds[0] = Emgui_loadFont("/Users/daniel/Library/Fonts/MicroKnight_v1.0.ttf", 11.0f);
	memset(&s_testChannel, 0, sizeof(Channel));

	s_testChannel.values[0] = 12.0f;
	s_testChannel.set[0] = true;

	s_testChannel.values[1] = 0.00120f;
	s_testChannel.set[1] = true;

	s_testChannel.values[2] = 0.120f;
	s_testChannel.set[2] = true;

	s_testChannel.values[4] = 100.120f;
	s_testChannel.set[4] = true;

	s_testChannel.maxRange = 1000;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void Editor_init()
{
	/*
	memset(&s_editor, 0, sizeof(Editor));

	s_editor.tracks[0].name = strdup("DemoTrack");
	s_editor.tracks[1].name = strdup("CameraTestZ");
	s_editor.tracks[2].name = strdup("TestName 3");
	s_editor.tracks[3].name = strdup("TestName 4");
	s_editor.trackCount = 4;

	s_editor.windowWidth = 800;
	s_editor.windowHeight = 600;

	RocketGui_init();
	*/
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static void printRowNumbers(int x, int y, int rowCount, int rowOffset, int rowSpacing, int maskBpm)
{
	int i;

	if (rowOffset < 0)
	{
		y += rowSpacing * -rowOffset;
		rowOffset  = 0;
	}

	for (i = 0; i < rowCount; ++i)
	{
		char rowText[16];
		sprintf(rowText, "%04d\n", rowOffset);


		if (rowOffset % maskBpm)
			Emgui_drawText(rowText, x, y, Emgui_color32(0xa0, 0xa0, 0xa0, 0xff));
		else
			Emgui_drawText(rowText, x, y, Emgui_color32(0xf0, 0xf0, 0xf0, 0xff));

		y += rowSpacing;
		rowOffset++;
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void renderChannel(Channel* channel, int startX, int startY, int startPos, int endPos)
{
	uint x, y;

	uint32_t color = Emgui_color32(40, 40, 40, 255);
	Emgui_drawBorder(color, color, startX, startY, 64, 600);
	int y_offset = (startY + font_size) + font_size/2;

	if (startPos < 0)
	{
		y_offset = ((startY + font_size * -startPos) + font_size) + font_size/2;
		startPos = 0;
		endPos = 40;
	}

	//int count = 0;

	//float values[40];

	for (y = startPos; y < endPos; ++y)
	{
		int offset = startX + 6;

		float value = 0.0f;
		bool set = false;

		if (y < channel->maxRange)
		{
			set = channel->set[y];
			value = channel->values[y];
		}

		if (set)
		{
			char valueText[16];

			if (value < 0.0f)
				sprintf(valueText, "%0.8f", value);
			else
				sprintf(valueText, "%8.4f", value);

			Emgui_drawText(valueText, offset, y_offset + (font_size / 2), Emgui_color32(255, 255, 255, 255));
		}
		else
		{
			int points[64];
			int* points_ptr = (int*)&points[0];

			for (x = 0; x < 6; ++x)
			{
				points_ptr[0] = offset + 0;
				points_ptr[1] = y_offset;
				points_ptr[2] = offset + 2;
				points_ptr[3] = y_offset;
				points_ptr[4] = offset + 4;
				points_ptr[5] = y_offset;

				points_ptr += 6;
				offset += 10;
			}

			if (y & 7)
				Emgui_drawDots(0x004f4f4f, (int*)&points[0], 18 * 2);
			else
				Emgui_drawDots(0x007f7f7f, (int*)&points[0], 18 * 2);
		}

		y_offset += font_size;
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void Editor_update()
{
	uint i = 0; //, channel_count = 10; 

	Emgui_begin();

	printRowNumbers(2, 42, 40, start_pos, font_size, 8);

	i = 0;

	//for (i = 0; i < channel_count; ++i)
		renderChannel(&s_testChannel, 40 + (i * 64), 20, start_pos, start_pos + 40);

	uint32_t color = Emgui_color32(127, 127, 127, 56);

	Emgui_fill(color, 0, 257, 800, font_size + 3);

	Emgui_end();
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

bool Editor_keyDown(int key)
{
	bool handled_key = true;

	//printf("callBack %d\n", key);

	switch (key)
	{
		case EMGUI_ARROW_DOWN : start_pos++; break;
		case EMGUI_ARROW_UP : start_pos--; break;
		default : handled_key = false; break;
	}

	if (start_pos < -19)
		start_pos = -19;

	if (key == '1')
	{
		int offset = start_pos + 19;
		printf("offset %d\n", offset);
		s_testChannel.values[offset] = 2.0f;
		s_testChannel.set[offset] = true;

		if (offset > s_testChannel.maxRange)
			s_testChannel.maxRange = offset;

		return true;
	}

	return handled_key;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void Editor_timedUpdate()
{
	


}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static void onOpen()
{
	LoadSave_loadRocketXMLDialog();
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void Editor_menuEvent(int menuItem)
{
	switch (menuItem)
	{
		//case EDITOR_MENU_NEW : onNew(); break;
		case EDITOR_MENU_OPEN : onOpen(); break;
		//case EDITOR_MENU_SAVE : onSave(); break;
		//case EDITOR_MENU_SAVE_AS : onSaveAs(); break;
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void Editor_destroy()
{

}

