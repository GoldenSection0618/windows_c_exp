#include <windows.h>

#define MainDialogProc MainDialogProc_Base
#define RunMainGuiDialog RunMainGuiDialog_Base
#include "gui_main_window.cpp"
#undef RunMainGuiDialog
#undef MainDialogProc

static void ResizeAndCenterDialog(HWND dialog)
{
    int clientWidth = DialogUnitToPixelX(dialog, 430);
    int clientHeight = DialogUnitToPixelY(dialog, 340);

    LONG_PTR style = GetWindowLongPtrW(dialog, GWL_STYLE);
    style &= ~WS_VSCROLL;
    SetWindowLongPtrW(dialog, GWL_STYLE, style);
    ShowScrollBar(dialog, SB_VERT, FALSE);

    RECT targetRect = {0, 0, clientWidth, clientHeight};
    LONG_PTR exStyle = GetWindowLongPtrW(dialog, GWL_EXSTYLE);
    AdjustWindowRectEx(&targetRect, static_cast<DWORD>(style), FALSE, static_cast<DWORD>(exStyle));

    int windowWidth = targetRect.right - targetRect.left;
    int windowHeight = targetRect.bottom - targetRect.top;

    MONITORINFO monitorInfo = {0};
    monitorInfo.cbSize = sizeof(monitorInfo);
    HMONITOR monitorHandle = MonitorFromWindow(dialog, MONITOR_DEFAULTTONEAREST);
    if (!GetMonitorInfoW(monitorHandle, &monitorInfo)) {
        SetWindowPos(dialog, nullptr, 0, 0, windowWidth, windowHeight,
            SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOMOVE | SWP_FRAMECHANGED);
        return;
    }

    int workWidth = monitorInfo.rcWork.right - monitorInfo.rcWork.left;
    int workHeight = monitorInfo.rcWork.bottom - monitorInfo.rcWork.top;
    int centeredX = monitorInfo.rcWork.left + (workWidth - windowWidth) / 2;
    int centeredY = monitorInfo.rcWork.top + (workHeight - windowHeight) / 2;

    SetWindowPos(dialog, nullptr, centeredX, centeredY, windowWidth, windowHeight,
        SWP_NOZORDER | SWP_NOACTIVATE | SWP_FRAMECHANGED);
}

static INT_PTR CALLBACK MainDialogProc(HWND dialog, UINT message, WPARAM wParam, LPARAM lParam)
{
    if (message == WM_GETMINMAXINFO) {
        MINMAXINFO *info = reinterpret_cast<MINMAXINFO *>(lParam);
        int clientWidth = DialogUnitToPixelX(dialog, 430);
        int clientHeight = DialogUnitToPixelY(dialog, 340);
        RECT targetRect = {0, 0, clientWidth, clientHeight};
        LONG_PTR style = GetWindowLongPtrW(dialog, GWL_STYLE) & ~WS_VSCROLL;
        LONG_PTR exStyle = GetWindowLongPtrW(dialog, GWL_EXSTYLE);

        AdjustWindowRectEx(&targetRect, static_cast<DWORD>(style), FALSE, static_cast<DWORD>(exStyle));
        info->ptMinTrackSize.x = targetRect.right - targetRect.left;
        info->ptMinTrackSize.y = targetRect.bottom - targetRect.top;
        return TRUE;
    }

    INT_PTR result = MainDialogProc_Base(dialog, message, wParam, lParam);
    if (message == WM_INITDIALOG) {
        ResizeAndCenterDialog(dialog);
    }
    return result;
}

INT_PTR RunMainGuiDialog(HINSTANCE instanceHandle, int showCommand)
{
    GuiState state = {};
    INITCOMMONCONTROLSEX controls = {0};

    controls.dwSize = sizeof(controls);
    controls.dwICC = ICC_LISTVIEW_CLASSES | ICC_STANDARD_CLASSES;
    InitCommonControlsEx(&controls);

    (void)showCommand;
    return DialogBoxParamW(instanceHandle,
                           MAKEINTRESOURCEW(IDD_AMS_MAIN_GUI),
                           nullptr,
                           MainDialogProc,
                           reinterpret_cast<LPARAM>(&state));
}
