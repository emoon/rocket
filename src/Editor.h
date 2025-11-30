#pragma once

#include <emgui/Types.h>

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void Editor_create(void);
void Editor_destroy(void);
void Editor_init(void);
void Editor_update(void);
void Editor_timedUpdate(void);
bool Editor_keyDown(int key, int keyCode, int mod);
void Editor_keyUp(void);
void Editor_setWindowSize(int x, int y);
void Editor_menuEvent(int menuItem);
void Editor_scroll(float deltaX, float deltaY, int flags);
void Editor_updateTrackScroll(void);
void Editor_loadRecentFile(int file);
bool Editor_saveBeforeExit(void);
bool Editor_needsSave(void);

void Editor_setLoadedFilename(const text_t* filename);
struct TrackData* Editor_getTrackData(void);
text_t** Editor_getRecentFiles(void);

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#define EDITOR_VERSION _T(" 1.2 Beta 1 ")
