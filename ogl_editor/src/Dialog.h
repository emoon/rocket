#pragma once

#if defined(_WIN32)

int Dialog_open(wchar_t* dest);
int Dialog_save(wchar_t* dest);

#else

int Dialog_open(char* dest);
int Dialog_save(char* dest);

#endif

