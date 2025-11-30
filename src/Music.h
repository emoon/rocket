#ifndef OGLEDITOR_MUSIC_H
#define OGLEDITOR_MUSIC_H

#include <emgui/Types.h>

struct MusicData;

void Music_init(void);
void Music_deinit(void);

int Music_decode(text_t* path, struct MusicData* data);

#endif
