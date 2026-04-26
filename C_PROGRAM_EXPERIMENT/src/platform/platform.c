#include "platform.h"

#include <errno.h>
#include <locale.h>
#include <string.h>

#ifdef _WIN32
#include <direct.h>
#include <windows.h>
#else
#include <sys/stat.h>
#include <sys/types.h>
#endif

int platformEnsureDirectoryExists(const char *path)
{
    if (path == NULL || path[0] == '\0') {
        return -1;
    }

#ifdef _WIN32
    if (_mkdir(path) == 0 || errno == EEXIST) {
        return 0;
    }
#else
    if (mkdir(path, 0777) == 0 || errno == EEXIST) {
        return 0;
    }
#endif

    return -1;
}

void platformInitConsole(void)
{
#ifdef _WIN32
    setlocale(LC_ALL, ".UTF-8");
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
#else
    setlocale(LC_ALL, "");
#endif
}
