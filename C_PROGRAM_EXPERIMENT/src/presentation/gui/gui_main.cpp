#include "gui_main_window.h"

#include "business.h"

#include <windows.h>

int APIENTRY wWinMain(HINSTANCE instanceHandle, HINSTANCE, LPWSTR, int showCommand)
{
    RunMainGuiDialog(instanceHandle, showCommand);
    bizShutdown();
    return 0;
}
