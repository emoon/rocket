#pragma once

#ifndef _WIN32
#define TEXT(text) text
#endif

#include <emgui/Types.h>

int Dialog_open(text_t* dest);
int Dialog_save(text_t* dest);
void Dialog_showError(const text_t* text);
