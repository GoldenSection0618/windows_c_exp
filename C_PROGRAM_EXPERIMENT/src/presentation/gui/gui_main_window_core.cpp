#include "gui_main_window_core.h"

#include "gui_main_window_internal.h"

#include "business.h"
#include "gui_resource.h"

#include <commctrl.h>
#include <windows.h>

#pragma comment(lib, "Comctl32.lib")

enum class NavAction {
    SwitchMode,
    Logout,
    Exit
};

struct NavRoute {
    LoginRole role;
    int controlId;
    NavAction action;
    GuiMode targetMode;
};

static const NavRoute kNavRoutes[] = {
    {LOGIN_ROLE_NONE, IDC_AMS_NAV_ADD_CARD, NavAction::SwitchMode, GuiMode::AuthAdmin},
    {LOGIN_ROLE_NONE, IDC_AMS_NAV_QUERY_CARD, NavAction::SwitchMode, GuiMode::AuthUserLogin},
    {LOGIN_ROLE_NONE, IDC_AMS_NAV_LOGON, NavAction::SwitchMode, GuiMode::AuthRegister},
    {LOGIN_ROLE_NONE, IDC_AMS_NAV_SETTLE, NavAction::Exit, GuiMode::AuthAdmin},

    {LOGIN_ROLE_ADMIN, IDC_AMS_NAV_ADD_CARD, NavAction::SwitchMode, GuiMode::AdminQuery},
    {LOGIN_ROLE_ADMIN, IDC_AMS_NAV_QUERY_CARD, NavAction::SwitchMode, GuiMode::AdminAdvancedQuery},
    {LOGIN_ROLE_ADMIN, IDC_AMS_NAV_LOGON, NavAction::SwitchMode, GuiMode::AdminStop},
    {LOGIN_ROLE_ADMIN, IDC_AMS_NAV_SETTLE, NavAction::SwitchMode, GuiMode::AdminRecharge},
    {LOGIN_ROLE_ADMIN, IDC_AMS_NAV_RECHARGE, NavAction::SwitchMode, GuiMode::AdminRefund},
    {LOGIN_ROLE_ADMIN, IDC_AMS_NAV_REFUND, NavAction::SwitchMode, GuiMode::AdminStatistics},
    {LOGIN_ROLE_ADMIN, IDC_AMS_NAV_CANCEL_CARD, NavAction::Logout, GuiMode::AuthAdmin},

    {LOGIN_ROLE_USER, IDC_AMS_NAV_ADD_CARD, NavAction::SwitchMode, GuiMode::UserBalance},
    {LOGIN_ROLE_USER, IDC_AMS_NAV_QUERY_CARD, NavAction::SwitchMode, GuiMode::UserStart},
    {LOGIN_ROLE_USER, IDC_AMS_NAV_LOGON, NavAction::SwitchMode, GuiMode::UserStop},
    {LOGIN_ROLE_USER, IDC_AMS_NAV_SETTLE, NavAction::SwitchMode, GuiMode::UserRecharge},
    {LOGIN_ROLE_USER, IDC_AMS_NAV_RECHARGE, NavAction::SwitchMode, GuiMode::UserRefund},
    {LOGIN_ROLE_USER, IDC_AMS_NAV_REFUND, NavAction::SwitchMode, GuiMode::UserCancel},
    {LOGIN_ROLE_USER, IDC_AMS_NAV_CANCEL_CARD, NavAction::Logout, GuiMode::AuthAdmin},

    {LOGIN_ROLE_NONE, IDC_AMS_NAV_EXIT, NavAction::Exit, GuiMode::AuthAdmin},
    {LOGIN_ROLE_ADMIN, IDC_AMS_NAV_EXIT, NavAction::Exit, GuiMode::AuthAdmin},
    {LOGIN_ROLE_USER, IDC_AMS_NAV_EXIT, NavAction::Exit, GuiMode::AuthAdmin},
};

static LoginRole CurrentRole(const GuiState *state)
{
    if (state == nullptr) {
        return LOGIN_ROLE_NONE;
    }
    if (bizIsAdminSession(&state->session)) {
        return LOGIN_ROLE_ADMIN;
    }
    if (bizIsUserSession(&state->session)) {
        return LOGIN_ROLE_USER;
    }
    return LOGIN_ROLE_NONE;
}

static const NavRoute *FindNavRoute(LoginRole role, int controlId)
{
    for (const NavRoute &route : kNavRoutes) {
        if (route.role == role && route.controlId == controlId) {
            return &route;
        }
    }
    return nullptr;
}

static int HandleNavCommand(HWND dialog, GuiState *state, int controlId)
{
    const NavRoute *route = FindNavRoute(CurrentRole(state), controlId);
    if (route == nullptr) {
        return 0;
    }

    switch (route->action) {
    case NavAction::SwitchMode:
        GuiSwitchMode(dialog, state, route->targetMode);
        return 1;
    case NavAction::Logout:
        GuiLogoutToLogin(dialog, state);
        return 1;
    case NavAction::Exit:
        EndDialog(dialog, 0);
        return 1;
    default:
        return 0;
    }
}

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

        if (LOWORD(wParam) == IDC_AMS_SUBMIT) {
            GuiExecuteSubmit(dialog, state);
            return TRUE;
        }

        return HandleNavCommand(dialog, state, LOWORD(wParam)) ? TRUE : FALSE;
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
