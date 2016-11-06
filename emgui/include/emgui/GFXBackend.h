#ifndef EMGFXBACKEND_H_
#define EMGFXBACKEND_H_

#include "Types.h"

bool EMGFXBackend_create();
bool EMGFXBackend_destroy();
void EMGFXBackend_updateViewPort(int width, int height);
void EMGFXBackend_render();

uint64_t EMGFXBackend_createFontTexture(void* imageBuffer, int w, int h);
uint64_t EMGFXBackend_createTexture(void* imageBuffer, int w, int h, int comp);
void EMGFXBackend_updateTexture(uint64_t texId, int w, int h, void* data);

#endif

