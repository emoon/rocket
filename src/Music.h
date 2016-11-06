#ifndef OGLEDITOR_MUSIC_H
#define OGLEDITOR_MUSIC_H

#include <emgui/Types.h>

struct MusicData;

void Music_init();
void Music_deinit();

int Music_decode(text_t* path, MusicData* data);

#endif
