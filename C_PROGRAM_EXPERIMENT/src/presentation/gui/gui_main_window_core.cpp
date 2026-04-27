#include "gui_main_window_core.h"

#include "gui_main_window_internal.h"

#include "business.h"
#include "gui_resource.h"

#include <commctrl.h>
#include <windows.h>


#pragma comment(lib, "Comctl32.lib")
static void ResizeAndCenterDialog(HWND dialog)
{
    int clientWidth = 760;
    int clientHeight = 560;
    LONG_PTR style = GetWindowLongPtrW(dialog, GWL_STYLE);
    RECT rect = {0, 0, clientWidth, clientHeight};
    AdjustWindowRectEx(&rect, static_cast<DWORD>(style), FALSE, static_cast<DWORD>(GetWindowLongPtrW(dialog, GWL_EXSTYLE)));

    int width = rect.right - rect.left;
    int height = rect.bottom - rect.top;
    RECT work = {0};
    SystemParametersInfoW(SPI_GETWORKAREA, 0, &work, 0);
    int x = work.left + ((work.right - work.left) - width) / 2;
    int y = work.top + ((work.bottom - work.top) - height) / 2;
    SetWindowPos(dialog, nullptr, x, y, width, height, SWP_NOZORDER | SWP_NOACTIVATE);
}

static INT_PTR CALLBACK MainDialogProc(HWND dialog, UINT message, WPARAM wParam, LPARAM lParam)
{
    GuiState *state = reinterpret_cast<GuiState *>(GetWindowLongPtrW(dialog, GWLP_USERDATA));

    switch (message) {
    case WM_INITDIALOG: {
        GuiState *initState = reinterpret_cast<GuiState *>(lParam);
        initState->listHandle = GetDlgItem(dialog, IDC_AMS_RESULT_LIST);
        bizInitSession(&initState->session);
        SetWindowLongPtrW(dialog, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(initState));
        ListView_SetExtendedListViewStyle(initState->listHandle, LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES | LVS_EX_DOUBLEBUFFER);
        ResizeAndCenterDialog(dialog);
        GuiSwitchMode(dialog, initState, GuiMode::AuthAdmin);
        return TRUE;
    }
    case WM_COMMAND:
        if (state == nullptr) {
            return FALSE;
        }

        switch (LOWORD(wParam)) {
        case IDC_AMS_SUBMIT:
            GuiExecuteSubmit(dialog, state);
            return TRUE;
        case IDC_AMS_NAV_ADD_CARD:
            if (bizIsAdminSession(&state->session)) {
                GuiSwitchMode(dialog, state, GuiMode::AdminQuery);
            } else if (bizIsUserSession(&state->session)) {
                GuiSwitchMode(dialog, state, GuiMode::UserBalance);
            } else {
                GuiSwitchMode(dialog, state, GuiMode::AuthAdmin);
            }
            return TRUE;
        case IDC_AMS_NAV_QUERY_CARD:
            if (bizIsAdminSession(&state->session)) {
                GuiSwitchMode(dialog, state, GuiMode::AdminStop);
            } else if (bizIsUserSession(&state->session)) {
                GuiSwitchMode(dialog, state, GuiMode::UserStart);
            } else {
                GuiSwitchMode(dialog, state, GuiMode::AuthUserLogin);
            }
            return TRUE;
        case IDC_AMS_NAV_LOGON:
            if (bizIsAdminSession(&state->session)) {
                GuiSwitchMode(dialog, state, GuiMode::AdminRecharge);
            } else if (bizIsUserSession(&state->session)) {
                GuiSwitchMode(dialog, state, GuiMode::UserStop);
            } else {
                GuiSwitchMode(dialog, state, GuiMode::AuthRegister);
            }
            return TRUE;
        case IDC_AMS_NAV_SETTLE:
            if (bizIsAdminSession(&state->session)) {
                GuiSwitchMode(dialog, state, GuiMode::AdminRefund);
            } else if (bizIsUserSession(&state->session)) {
                GuiSwitchMode(dialog, state, GuiMode::UserRecharge);
            } else {
                EndDialog(dialog, 0);
            }
            return TRUE;
        case IDC_AMS_NAV_RECHARGE:
            if (bizIsAdminSession(&state->session)) {
                GuiSwitchMode(dialog, state, GuiMode::AdminStatistics);
            } else if (bizIsUserSession(&state->session)) {
                GuiSwitchMode(dialog, state, GuiMode::UserRefund);
            }
            return TRUE;
        case IDC_AMS_NAV_REFUND:
            if (bizIsAdminSession(&state->session)) {
                GuiLogoutToLogin(dialog, state);
            } else if (bizIsUserSession(&state->session)) {
                GuiSwitchMode(dialog, state, GuiMode::UserCancel);
            }
            return TRUE;
        case IDC_AMS_NAV_CANCEL_CARD:
            if (bizIsUserSession(&state->session)) {
                GuiLogoutToLogin(dialog, state);
            }
            return TRUE;
        case IDC_AMS_NAV_EXIT:
            EndDialog(dialog, 0);
            return TRUE;
        default:
            return FALSE;
        }
    case WM_CLOSE:
        EndDialog(dialog, 0);
        return TRUE;
    default:
        return FALSE;
    }
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
