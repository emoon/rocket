#include "rlog.h"
#include <stdarg.h>
#include <stdio.h>

#if defined(WIN32)
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#endif

static int s_log_level = 0;
static int s_old_level = 0;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void rlog(int logLevel, const char* format, ...) {
    va_list ap;

    if (logLevel < s_log_level)
        return;

    va_start(ap, format);
#if defined(_WIN32)
    {
        char buffer[2048];
        vsprintf(buffer, format, ap);
        OutputDebugStringA(buffer);
    }
#else
    vprintf(format, ap);
#endif
    va_end(ap);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void rlog_set_level(int logLevel) {
    s_log_level = logLevel;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void rlog_level_push() {
    s_old_level = s_log_level;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void rlog_level_pop() {
    s_log_level = s_old_level;
}
