#pragma once

#include <emgui/Types.h>

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

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
bool Editor_needsSave();

void Editor_setLoadedFilename(const text_t* filename);
struct TrackData* Editor_getTrackData();
text_t** Editor_getRecentFiles();

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#define EDITOR_VERSION _T(" 1.2 Beta 1 ")
