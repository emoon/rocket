#pragma once

#include "Types.h"

void Editor_create();
void Editor_destroy();
void Editor_init();
void Editor_update();
void Editor_timedUpdate();
bool Editor_keyDown(int keyCode, int mod);
void Editor_setWindowSize(int x, int y);
void Editor_menuEvent(int menuItem);
void Editor_scroll(int deltaX, int deltaY);

enum
{
	EDITOR_MENU_NEW,
	EDITOR_MENU_OPEN,
	EDITOR_MENU_SAVE,
	EDITOR_MENU_SAVE_AS,
};

enum
{
	EDITOR_KEY_SHIFT = 1,
	EDITOR_KEY_ALT = 2,
	EDITOR_KEY_CTRL = 4,
	EDITOR_KEY_COMMAND = 8,
};

