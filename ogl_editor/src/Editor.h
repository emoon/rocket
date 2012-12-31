#pragma once

#include "Types.h"

void Editor_create();
void Editor_destroy();
void Editor_init();
void Editor_update();
void Editor_timedUpdate();
bool Editor_keyDown(int key, int keyCode, int mod);
void Editor_keyUp();
void Editor_setWindowSize(int x, int y);
void Editor_menuEvent(int menuItem);
void Editor_scroll(float deltaX, float deltaY, int flags);
void Editor_updateTrackScroll();
void Editor_loadRecentFile(int file);
bool Editor_saveBeforeExit();

char** Editor_getRecentFiles();

enum
{
	EDITOR_MENU_NEW,
	EDITOR_MENU_OPEN,
	EDITOR_MENU_SAVE,
	EDITOR_MENU_SAVE_AS,
	EDITOR_MENU_REMOTE_EXPORT,
};

enum
{
	EDITOR_KEY_SHIFT = 1,
	EDITOR_KEY_ALT = 2,
	EDITOR_KEY_CTRL = 4,
	EDITOR_KEY_COMMAND = 8,
};

