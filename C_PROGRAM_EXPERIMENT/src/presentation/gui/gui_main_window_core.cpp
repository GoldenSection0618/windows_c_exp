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

static const int kMinClientWidthDlu = 580;
static const int kMinClientHeightDlu = 320;
static const int kNavLeftDlu = 6;
static const int kNavTopDlu = 6;
static const int kNavWidthDlu = 120;
static const int kContentLeftDlu = 140;
static const int kRightMarginDlu = 10;
static const int kFieldTopDlu = 20;
static const int kLabelHeightDlu = 16;
static const int kEditTopDlu = 38;
static const int kEditHeightDlu = 24;
static const int kColumnGapDlu = 20;
static const int kSubmitTopDlu = 72;
static const int kSubmitWidthDlu = 80;
static const int kSubmitHeightDlu = 26;
static const int kStatusLeftGapDlu = 10;
static const int kStatusTopDlu = 78;
static const int kStatusHeightDlu = 20;
static const int kListTopDlu = 110;
static const int kBottomMarginDlu = 8;

static const NavRoute kNavRoutes[] = {
    {LOGIN_ROLE_NONE, IDC_AMS_NAV_ADD_CARD, NavAction::SwitchMode, GuiMode::AuthAdmin},
    {LOGIN_ROLE_NONE, IDC_AMS_NAV_QUERY_CARD, NavAction::SwitchMode, GuiMode::AuthUserLogin},
    {LOGIN_ROLE_NONE, IDC_AMS_NAV_LOGON, NavAction::SwitchMode, GuiMode::AuthRegister},
    {LOGIN_ROLE_NONE, IDC_AMS_NAV_SETTLE, NavAction::Exit, GuiMode::AuthAdmin},

    {LOGIN_ROLE_ADMIN, IDC_AMS_NAV_ADD_CARD, NavAction::SwitchMode, GuiMode::AdminQuery},
    {LOGIN_ROLE_ADMIN, IDC_AMS_NAV_QUERY_CARD, NavAction::SwitchMode, GuiMode::AdminFuzzyQuery},
    {LOGIN_ROLE_ADMIN, IDC_AMS_NAV_LOGON, NavAction::SwitchMode, GuiMode::AdminAdvancedQuery},
    {LOGIN_ROLE_ADMIN, IDC_AMS_NAV_SETTLE, NavAction::SwitchMode, GuiMode::AdminStop},
    {LOGIN_ROLE_ADMIN, IDC_AMS_NAV_RECHARGE, NavAction::SwitchMode, GuiMode::AdminRecharge},
    {LOGIN_ROLE_ADMIN, IDC_AMS_NAV_REFUND, NavAction::SwitchMode, GuiMode::AdminRefund},
    {LOGIN_ROLE_ADMIN, IDC_AMS_NAV_CANCEL_CARD, NavAction::SwitchMode, GuiMode::AdminStatistics},
    {LOGIN_ROLE_ADMIN, IDC_AMS_NAV_BILLING, NavAction::SwitchMode, GuiMode::AdminFileMaintenance},
    {LOGIN_ROLE_ADMIN, IDC_AMS_NAV_STAT, NavAction::Logout, GuiMode::AuthAdmin},

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

static RECT DluRect(HWND dialog, int left, int top, int right, int bottom)
{
    RECT rect = {left, top, right, bottom};
    MapDialogRect(dialog, &rect);
    return rect;
}

static int DialogUnitToPixelX(HWND dialog, int value)
{
    RECT rect = DluRect(dialog, 0, 0, value, 0);
    return rect.right;
}

static int DialogUnitToPixelY(HWND dialog, int value)
{
    RECT rect = DluRect(dialog, 0, 0, 0, value);
    return rect.bottom;
}

static void MoveControl(HWND dialog, int controlId, int x, int y, int width, int height)
{
    HWND control = GetDlgItem(dialog, controlId);
    if (control != nullptr) {
        SetWindowPos(control, nullptr, x, y, width, height, SWP_NOZORDER | SWP_NOACTIVATE);
    }
}

static RECT WindowRectForClientSize(HWND dialog, int clientWidth, int clientHeight)
{
    RECT rect = {0, 0, clientWidth, clientHeight};
    AdjustWindowRectEx(&rect,
                       static_cast<DWORD>(GetWindowLongPtrW(dialog, GWL_STYLE)),
                       FALSE,
                       static_cast<DWORD>(GetWindowLongPtrW(dialog, GWL_EXSTYLE)));
    return rect;
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
    HMONITOR monitor = MonitorFromWindow(dialog, MONITOR_DEFAULTTOPRIMARY);
    MONITORINFO monitorInfo = {sizeof(MONITORINFO)};
    if (GetMonitorInfoW(monitor, &monitorInfo)) {
        int clientWidth = DialogUnitToPixelX(dialog, kMinClientWidthDlu);
        int clientHeight = DialogUnitToPixelY(dialog, kMinClientHeightDlu);
        int workWidth = monitorInfo.rcWork.right - monitorInfo.rcWork.left;
        int workHeight = monitorInfo.rcWork.bottom - monitorInfo.rcWork.top;

        RECT rect = WindowRectForClientSize(dialog, clientWidth, clientHeight);
        int frameWidth = (rect.right - rect.left) - clientWidth;
        int frameHeight = (rect.bottom - rect.top) - clientHeight;
        clientWidth = min(clientWidth, max(1, workWidth - frameWidth));
        clientHeight = min(clientHeight, max(1, workHeight - frameHeight));
        rect = WindowRectForClientSize(dialog, clientWidth, clientHeight);

        int width = rect.right - rect.left;
        int height = rect.bottom - rect.top;
        int x = monitorInfo.rcWork.left + ((monitorInfo.rcWork.right - monitorInfo.rcWork.left) - width) / 2;
        int y = monitorInfo.rcWork.top + ((monitorInfo.rcWork.bottom - monitorInfo.rcWork.top) - height) / 2;
        SetWindowPos(dialog, nullptr, x, y, width, height, SWP_NOZORDER | SWP_NOACTIVATE);
    }
}

static void LayoutDialog(HWND dialog, int clientWidth, int clientHeight)
{
    int navLeft = DialogUnitToPixelX(dialog, kNavLeftDlu);
    int navTop = DialogUnitToPixelY(dialog, kNavTopDlu);
    int navWidth = DialogUnitToPixelX(dialog, kNavWidthDlu);
    int contentLeft = DialogUnitToPixelX(dialog, kContentLeftDlu);
    int rightMargin = DialogUnitToPixelX(dialog, kRightMarginDlu);
    int columnGap = DialogUnitToPixelX(dialog, kColumnGapDlu);
    int labelTop = DialogUnitToPixelY(dialog, kFieldTopDlu);
    int labelHeight = DialogUnitToPixelY(dialog, kLabelHeightDlu);
    int editTop = DialogUnitToPixelY(dialog, kEditTopDlu);
    int editHeight = DialogUnitToPixelY(dialog, kEditHeightDlu);
    int submitTop = DialogUnitToPixelY(dialog, kSubmitTopDlu);
    int submitWidth = DialogUnitToPixelX(dialog, kSubmitWidthDlu);
    int submitHeight = DialogUnitToPixelY(dialog, kSubmitHeightDlu);
    int statusLeftGap = DialogUnitToPixelX(dialog, kStatusLeftGapDlu);
    int statusTop = DialogUnitToPixelY(dialog, kStatusTopDlu);
    int statusHeight = DialogUnitToPixelY(dialog, kStatusHeightDlu);
    int listTop = DialogUnitToPixelY(dialog, kListTopDlu);
    int bottomMargin = DialogUnitToPixelY(dialog, kBottomMarginDlu);
    int contentRight = clientWidth - rightMargin;
    int contentWidth = max(1, contentRight - contentLeft);
    int columnWidth = max(1, (contentWidth - (columnGap * 2)) / 3);

    MoveControl(dialog, IDC_AMS_NAV_FRAME, navLeft, navTop, navWidth, max(1, clientHeight - (navTop * 2)));

    int x1 = contentLeft;
    int x2 = x1 + columnWidth + columnGap;
    int x3 = x2 + columnWidth + columnGap;
    MoveControl(dialog, IDC_AMS_LABEL_1, x1, labelTop, columnWidth, labelHeight);
    MoveControl(dialog, IDC_AMS_CARD_NAME, x1, editTop, columnWidth, editHeight);
    MoveControl(dialog, IDC_AMS_LABEL_2, x2, labelTop, columnWidth, labelHeight);
    MoveControl(dialog, IDC_AMS_CARD_PASSWORD, x2, editTop, columnWidth, editHeight);
    MoveControl(dialog, IDC_AMS_LABEL_3, x3, labelTop, columnWidth, labelHeight);
    MoveControl(dialog, IDC_AMS_CARD_MONEY, x3, editTop, columnWidth, editHeight);

    MoveControl(dialog, IDC_AMS_SUBMIT, contentLeft, submitTop, submitWidth, submitHeight);
    MoveControl(dialog,
                IDC_AMS_STATUS_TEXT,
                contentLeft + submitWidth + statusLeftGap,
                statusTop,
                max(1, contentRight - (contentLeft + submitWidth + statusLeftGap)),
                statusHeight);
    MoveControl(dialog,
                IDC_AMS_RESULT_LIST,
                contentLeft,
                listTop,
                contentWidth,
                max(1, clientHeight - listTop - bottomMargin));
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
        RECT client = {0};
        GetClientRect(dialog, &client);
        LayoutDialog(dialog, client.right - client.left, client.bottom - client.top);
        GuiSwitchMode(dialog, initState, GuiMode::AuthAdmin);
        return TRUE;
    }
    case WM_GETMINMAXINFO: {
        MINMAXINFO *mmi = reinterpret_cast<MINMAXINFO *>(lParam);
        RECT minRect = WindowRectForClientSize(dialog,
                                               DialogUnitToPixelX(dialog, kMinClientWidthDlu),
                                               DialogUnitToPixelY(dialog, kMinClientHeightDlu));
        mmi->ptMinTrackSize.x = minRect.right - minRect.left;
        mmi->ptMinTrackSize.y = minRect.bottom - minRect.top;
        return TRUE;
    }
    case WM_SIZE: {
        int cx = LOWORD(lParam);
        int cy = HIWORD(lParam);
        if (cx == 0 || cy == 0) return TRUE;
        LayoutDialog(dialog, cx, cy);
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
