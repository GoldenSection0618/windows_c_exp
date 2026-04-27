#include <windows.h>

#define RunMainGuiDialog RunMainGuiDialog_Uncentered
#include "gui_main_window_scroll.cpp"
#undef RunMainGuiDialog

static HHOOK gCenterDialogHook = nullptr;

static void CenterWindowInMonitorWorkArea(HWND windowHandle)
{
    RECT windowRect = {0};
    MONITORINFO monitorInfo = {0};
    monitorInfo.cbSize = sizeof(monitorInfo);

    GetWindowRect(windowHandle, &windowRect);
    HMONITOR monitorHandle = MonitorFromWindow(windowHandle, MONITOR_DEFAULTTONEAREST);
    if (!GetMonitorInfoW(monitorHandle, &monitorInfo)) {
        return;
    }

    int windowWidth = windowRect.right - windowRect.left;
    int windowHeight = windowRect.bottom - windowRect.top;
    int workWidth = monitorInfo.rcWork.right - monitorInfo.rcWork.left;
    int workHeight = monitorInfo.rcWork.bottom - monitorInfo.rcWork.top;

    int centeredX = monitorInfo.rcWork.left + (workWidth - windowWidth) / 2;
    int centeredY = monitorInfo.rcWork.top + (workHeight - windowHeight) / 2;

    SetWindowPos(windowHandle, nullptr, centeredX, centeredY, 0, 0,
        SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOSIZE);
}

static LRESULT CALLBACK CenterDialogHookProc(int code, WPARAM wParam, LPARAM lParam)
{
    if (code == HCBT_ACTIVATE && gCenterDialogHook != nullptr) {
        CenterWindowInMonitorWorkArea(reinterpret_cast<HWND>(wParam));
        UnhookWindowsHookEx(gCenterDialogHook);
        gCenterDialogHook = nullptr;
    }

    return CallNextHookEx(gCenterDialogHook, code, wParam, lParam);
}

INT_PTR RunMainGuiDialog(HINSTANCE instanceHandle, int showCommand)
{
    gCenterDialogHook = SetWindowsHookExW(WH_CBT, CenterDialogHookProc, nullptr, GetCurrentThreadId());

    INT_PTR result = RunMainGuiDialog_Uncentered(instanceHandle, showCommand);

    if (gCenterDialogHook != nullptr) {
        UnhookWindowsHookEx(gCenterDialogHook);
        gCenterDialogHook = nullptr;
    }

    return result;
}
