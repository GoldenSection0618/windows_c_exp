#include <windows.h>

static BOOL GuiInvalidateWithoutErase(HWND windowHandle, const RECT *rect, BOOL eraseBackground)
{
    (void)eraseBackground;
    return RedrawWindow(windowHandle,
                        rect,
                        nullptr,
                        RDW_INVALIDATE | RDW_ALLCHILDREN | RDW_UPDATENOW) ? TRUE : FALSE;
}

#define InvalidateRect GuiInvalidateWithoutErase
#define RunMainGuiDialog RunMainGuiDialog_Uncentered
#include "gui_main_window_scroll.cpp"
#undef RunMainGuiDialog
#undef InvalidateRect

static HHOOK gCenterDialogHook = nullptr;

static void EnableLowFlickerDialogStyles(HWND windowHandle)
{
    LONG_PTR style = GetWindowLongPtrW(windowHandle, GWL_STYLE);
    SetWindowLongPtrW(windowHandle, GWL_STYLE, style | WS_CLIPCHILDREN | WS_CLIPSIBLINGS);
    SetWindowPos(windowHandle,
                 nullptr,
                 0,
                 0,
                 0,
                 0,
                 SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOSIZE | SWP_FRAMECHANGED);
}

static void CenterWindowInMonitorWorkArea(HWND windowHandle)
{
    RECT windowRect = {0};
    MONITORINFO monitorInfo = {0};
    monitorInfo.cbSize = sizeof(monitorInfo);

    EnableLowFlickerDialogStyles(windowHandle);

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
