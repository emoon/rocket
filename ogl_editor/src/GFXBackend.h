#pragma once

void GFXBackend_create();
void GFXBackend_destroy();
void GFXBackend_updateViewPort(int width, int height);
void GFXBackend_setFont(void* data, int width, int height);
void GFXBackend_drawControls(void* data, int count);

