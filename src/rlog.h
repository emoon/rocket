#pragma once

enum {
    R_DEBUG,
    R_INFO,
    R_ERROR,
};

void rlog(int logLevel, const char* format, ...);
void rlog_set_level(int logLevel);
void rlog_level_push(void);
void rlog_level_pop(void);
