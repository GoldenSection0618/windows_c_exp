#include "gui_main_window_core.h"

#include <windows.h>

INT_PTR RunCenteredMainGuiDialog(HINSTANCE instanceHandle, int showCommand)
{
    return RunMainGuiDialog(instanceHandle, showCommand);
}
