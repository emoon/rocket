#pragma once

#include <emgui/Types.h>

// Parsed command line arguments
typedef struct Args {
    const text_t* loadFile;  // File to load on startup (NULL if none)
} Args;

// Parse command line arguments
// argc/argv should be in the native format for the platform
// On Windows: use CommandLineToArgvW to get wide strings
// On Linux/macOS: use standard argc/argv
void Args_parse(Args* args, int argc, const text_t* const* argv);
