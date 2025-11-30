#include "args.h"
#include <string.h>

#if defined(_WIN32)
#include <windows.h>
#define STRCMP wcscmp
#define STRICMP _wcsicmp
#define STRLEN wcslen
#define STR(s) L##s
#else
#include <strings.h>
#define STRCMP strcmp
#define STRICMP strcasecmp
#define STRLEN strlen
#define STR(s) s
#endif

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static int hasExtension(const text_t* path, const text_t* ext) {
    size_t pathLen = STRLEN(path);
    size_t extLen = STRLEN(ext);
    if (pathLen < extLen) return 0;
    return STRICMP(path + pathLen - extLen, ext) == 0;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void Args_parse(Args* args, int argc, const text_t* const* argv) {
    args->loadFile = NULL;

    for (int i = 1; i < argc; i++) {
        // Check for --load or -l flag
        if ((STRCMP(argv[i], STR("--load")) == 0 || STRCMP(argv[i], STR("-l")) == 0) && i + 1 < argc) {
            args->loadFile = argv[i + 1];
            return;
        }
        // Check for bare filename (ends with .rocket or .xml)
        if (hasExtension(argv[i], STR(".rocket")) || hasExtension(argv[i], STR(".xml"))) {
            args->loadFile = argv[i];
            return;
        }
    }
}
