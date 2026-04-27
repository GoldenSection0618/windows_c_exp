#include "gui_main_window_core.h"

#include "business.h"
#include "gui_resource.h"
#include "gui_utils.h"

#include <commctrl.h>
#include <windows.h>

#include <string>
#include <vector>

#pragma comment(lib, "Comctl32.lib")
int DialogUnitToPixelX(HWND dialog, int value)
{
    RECT rect = {0, 0, value, 0};
    MapDialogRect(dialog, &rect);
    return rect.right;
}

int DialogUnitToPixelY(HWND dialog, int value)
{
    RECT rect = {0, 0, 0, value};
    MapDialogRect(dialog, &rect);
    return rect.bottom;
}

static std::wstring ReadText(HWND dialog, int controlId)
{
    wchar_t buffer[256] = {0};
    GetDlgItemTextW(dialog, controlId, buffer, static_cast<int>(sizeof(buffer) / sizeof(buffer[0])));
    return std::wstring(buffer);
}

static void SetText(HWND dialog, int controlId, const wchar_t *text)
{
    SetDlgItemTextW(dialog, controlId, text == nullptr ? L"" : text);
}

static void SetStatus(HWND dialog, const std::wstring &text)
{
    SetText(dialog, IDC_AMS_STATUS_TEXT, text.c_str());
}

static void SetControlVisible(HWND dialog, int controlId, bool visible)
{
    HWND control = GetDlgItem(dialog, controlId);
    if (control != nullptr) {
        ShowWindow(control, visible ? SW_SHOW : SW_HIDE);
    }
}

static void SetPasswordMask(HWND dialog, bool enabled)
{
    HWND edit = GetDlgItem(dialog, IDC_AMS_CARD_PASSWORD);
    SendMessageW(edit, EM_SETPASSWORDCHAR, enabled ? L'*' : 0, 0);
    InvalidateRect(edit, nullptr, TRUE);
}

static void ClearInputs(HWND dialog)
{
    SetText(dialog, IDC_AMS_CARD_NAME, L"");
    SetText(dialog, IDC_AMS_CARD_PASSWORD, L"");
    SetText(dialog, IDC_AMS_CARD_MONEY, L"");
}

static void ClearListColumns(HWND listHandle)
{
    int column = Header_GetItemCount(ListView_GetHeader(listHandle));
    while (column-- > 0) {
        ListView_DeleteColumn(listHandle, column);
    }
}

static void PrepareList(HWND listHandle)
{
    ListView_DeleteAllItems(listHandle);
    ClearListColumns(listHandle);
}

static void AddColumn(HWND listHandle, int index, int width, const wchar_t *title)
{
    LVCOLUMNW column = {0};
    column.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT;
    column.fmt = LVCFMT_LEFT;
    column.cx = width;
    column.pszText = const_cast<LPWSTR>(title);
    ListView_InsertColumn(listHandle, index, &column);
}

static void AddRow(HWND listHandle, int row, const std::vector<std::wstring> &values)
{
    LVITEMW item = {0};
    if (values.empty()) {
        return;
    }

    item.mask = LVIF_TEXT;
    item.iItem = row;
    item.pszText = const_cast<LPWSTR>(values[0].c_str());
    ListView_InsertItem(listHandle, &item);

    for (size_t i = 1; i < values.size(); ++i) {
        ListView_SetItemText(listHandle, row, static_cast<int>(i), const_cast<LPWSTR>(values[i].c_str()));
    }
}

static void ShowMessageRow(HWND listHandle, const wchar_t *message)
{
    PrepareList(listHandle);
    AddColumn(listHandle, 0, 700, L"提示");
    AddRow(listHandle, 0, {message});
}

static void ShowCard(HWND listHandle, const Card &card)
{
    PrepareList(listHandle);
    AddColumn(listHandle, 0, 150, L"卡号");
    AddColumn(listHandle, 1, 100, L"状态");
    AddColumn(listHandle, 2, 100, L"余额");
    AddColumn(listHandle, 3, 120, L"累计消费");
    AddColumn(listHandle, 4, 100, L"使用次数");
    AddColumn(listHandle, 5, 190, L"最后使用时间");

    AddRow(listHandle, 0, {
        GuiUtf8ToWide(card.aCardName),
        GuiCardStatusText(card.nStatus),
        GuiFormatMoneyFromCent(card.nBalanceCent),
        GuiFormatMoneyFromCent(card.nTotalUseCent),
        std::to_wstring(card.nUseCount),
        GuiFormatTime(card.tLast)
    });
}

static void ShowSettle(HWND listHandle, const SettleInfo &info)
{
    PrepareList(listHandle);
    AddColumn(listHandle, 0, 150, L"卡号");
    AddColumn(listHandle, 1, 190, L"上机时间");
    AddColumn(listHandle, 2, 190, L"下机时间");
    AddColumn(listHandle, 3, 100, L"消费金额");
    AddColumn(listHandle, 4, 100, L"余额");
    AddRow(listHandle, 0, {
        GuiUtf8ToWide(info.aCardName),
        GuiFormatTime(info.tStart),
        GuiFormatTime(info.tEnd),
        GuiFormatMoneyFromCent(info.nAmountCent),
        GuiFormatMoneyFromCent(info.nBalanceCent)
    });
}

static void ShowLogon(HWND listHandle, const LogonInfo &info)
{
    PrepareList(listHandle);
    AddColumn(listHandle, 0, 150, L"卡号");
    AddColumn(listHandle, 1, 100, L"余额");
    AddColumn(listHandle, 2, 190, L"上机时间");
    AddRow(listHandle, 0, {
        GuiUtf8ToWide(info.aCardName),
        GuiFormatMoneyFromCent(info.nBalanceCent),
        GuiFormatTime(info.tStart)
    });
}

static void ShowMoney(HWND listHandle, const Card &card, const Money &money, const wchar_t *title)
{
    PrepareList(listHandle);
    AddColumn(listHandle, 0, 150, L"卡号");
    AddColumn(listHandle, 1, 120, title);
    AddColumn(listHandle, 2, 120, L"当前余额");
    AddRow(listHandle, 0, {
        GuiUtf8ToWide(card.aCardName),
        GuiFormatMoneyFromCent(money.nMoneyCent),
        GuiFormatMoneyFromCent(card.nBalanceCent)
    });
}

static void ShowStatistics(HWND listHandle, const BillingStatistics &statistics)
{
    PrepareList(listHandle);
    AddColumn(listHandle, 0, 160, L"项目");
    AddColumn(listHandle, 1, 160, L"金额");
    AddRow(listHandle, 0, {L"总营业额", GuiFormatMoneyFromCent(statistics.totalAmountCent)});
    for (int i = 0; i < 12; ++i) {
        AddRow(listHandle, i + 1, {
            std::to_wstring(statistics.year) + L"年" + std::to_wstring(i + 1) + L"月",
            GuiFormatMoneyFromCent(statistics.monthlyAmountCent[i])
        });
    }
}

static std::wstring ErrorText(BizResult result)
{
    if (result == BIZ_ERR_WRONG_PASSWORD) {
        return L"账号、卡号或密码错误。";
    }
    if (result == BIZ_ERR_SYSTEM) {
        return L"系统内部错误，或当前登录角色没有权限执行该操作。";
    }
    return GuiUtf8ToWide(bizGetMessage(result));
}

static void SetNavButton(HWND dialog, int controlId, const wchar_t *text, bool visible)
{
    SetText(dialog, controlId, text);
    SetControlVisible(dialog, controlId, visible);
}

static void ConfigureInput(HWND dialog,
                           const wchar_t *label1,
                           bool showInput1,
                           const wchar_t *label2,
                           bool showInput2,
                           const wchar_t *label3,
                           bool showInput3,
                           bool passwordMask)
{
    SetText(dialog, IDC_AMS_LABEL_1, label1);
    SetText(dialog, IDC_AMS_LABEL_2, label2);
    SetText(dialog, IDC_AMS_LABEL_3, label3);

    SetControlVisible(dialog, IDC_AMS_LABEL_1, showInput1);
    SetControlVisible(dialog, IDC_AMS_CARD_NAME, showInput1);
    SetControlVisible(dialog, IDC_AMS_LABEL_2, showInput2);
    SetControlVisible(dialog, IDC_AMS_CARD_PASSWORD, showInput2);
    SetControlVisible(dialog, IDC_AMS_LABEL_3, showInput3);
    SetControlVisible(dialog, IDC_AMS_CARD_MONEY, showInput3);
    SetPasswordMask(dialog, passwordMask);
}

static void SwitchMode(HWND dialog, GuiState *state, GuiMode mode)
{
    state->mode = mode;
    ClearInputs(dialog);
    PrepareList(state->listHandle);
    SetControlVisible(dialog, IDC_AMS_SUBMIT, true);

    switch (mode) {
    case GuiMode::AuthAdmin:
        SetWindowTextW(dialog, L"计费管理系统 - 登录");
        SetNavButton(dialog, IDC_AMS_NAV_ADD_CARD, L"管理员登录", true);
        SetNavButton(dialog, IDC_AMS_NAV_QUERY_CARD, L"用户登录", true);
        SetNavButton(dialog, IDC_AMS_NAV_LOGON, L"用户注册", true);
        SetNavButton(dialog, IDC_AMS_NAV_SETTLE, L"退出", true);
        SetNavButton(dialog, IDC_AMS_NAV_RECHARGE, L"", false);
        SetNavButton(dialog, IDC_AMS_NAV_REFUND, L"", false);
        SetNavButton(dialog, IDC_AMS_NAV_CANCEL_CARD, L"", false);
        SetNavButton(dialog, IDC_AMS_NAV_BILLING, L"", false);
        SetNavButton(dialog, IDC_AMS_NAV_STAT, L"", false);
        SetNavButton(dialog, IDC_AMS_NAV_EXIT, L"", false);
        ConfigureInput(dialog, L"管理员账号", true, L"管理员密码", true, L"", false, true);
        SetText(dialog, IDC_AMS_SUBMIT, L"登录");
        SetStatus(dialog, L"");
        break;
    case GuiMode::AuthUserLogin:
        ConfigureInput(dialog, L"卡号", true, L"密码", true, L"", false, true);
        SetText(dialog, IDC_AMS_SUBMIT, L"登录");
        SetStatus(dialog, L"用户登录：输入卡号和密码。");
        break;
    case GuiMode::AuthRegister:
        ConfigureInput(dialog, L"注册卡号", true, L"注册密码", true, L"", false, true);
        SetText(dialog, IDC_AMS_SUBMIT, L"注册");
        SetStatus(dialog, L"用户注册：默认赠送 100 元新手礼包。");
        break;
    case GuiMode::AdminQuery:
    case GuiMode::AdminStop:
        SetWindowTextW(dialog, L"计费管理系统 - 管理员后台");
        SetNavButton(dialog, IDC_AMS_NAV_ADD_CARD, L"查询卡", true);
        SetNavButton(dialog, IDC_AMS_NAV_QUERY_CARD, L"下机", true);
        SetNavButton(dialog, IDC_AMS_NAV_LOGON, L"充值", true);
        SetNavButton(dialog, IDC_AMS_NAV_SETTLE, L"退费", true);
        SetNavButton(dialog, IDC_AMS_NAV_RECHARGE, L"营业额统计", true);
        SetNavButton(dialog, IDC_AMS_NAV_REFUND, L"退出登录", true);
        SetNavButton(dialog, IDC_AMS_NAV_CANCEL_CARD, L"", false);
        SetNavButton(dialog, IDC_AMS_NAV_BILLING, L"", false);
        SetNavButton(dialog, IDC_AMS_NAV_STAT, L"", false);
        SetNavButton(dialog, IDC_AMS_NAV_EXIT, L"", false);
        ConfigureInput(dialog, L"卡号", true, L"", false, L"", false, false);
        SetText(dialog, IDC_AMS_SUBMIT, mode == GuiMode::AdminQuery ? L"查询" : L"下机");
        SetStatus(dialog, mode == GuiMode::AdminQuery ? L"管理员：查询任意卡，不需要卡密码。" : L"管理员：下机只需要卡号。");
        break;
    case GuiMode::AdminRecharge:
    case GuiMode::AdminRefund:
        ConfigureInput(dialog, L"卡号", true, L"", false, mode == GuiMode::AdminRecharge ? L"充值金额(元)" : L"退费金额(元)", true, false);
        SetText(dialog, IDC_AMS_SUBMIT, mode == GuiMode::AdminRecharge ? L"充值" : L"退费");
        SetStatus(dialog, L"管理员资金操作：只需要卡号和金额，不需要卡密码。");
        break;
    case GuiMode::AdminStatistics:
        ConfigureInput(dialog, L"年份(YYYY)", true, L"", false, L"", false, false);
        SetText(dialog, IDC_AMS_SUBMIT, L"统计");
        SetStatus(dialog, L"管理员：营业额统计。");
        break;
    case GuiMode::UserBalance:
    case GuiMode::UserStart:
    case GuiMode::UserStop:
        SetWindowTextW(dialog, L"计费管理系统 - 用户中心");
        SetNavButton(dialog, IDC_AMS_NAV_ADD_CARD, L"查余额", true);
        SetNavButton(dialog, IDC_AMS_NAV_QUERY_CARD, L"上机", true);
        SetNavButton(dialog, IDC_AMS_NAV_LOGON, L"下机", true);
        SetNavButton(dialog, IDC_AMS_NAV_SETTLE, L"充值", true);
        SetNavButton(dialog, IDC_AMS_NAV_RECHARGE, L"退费", true);
        SetNavButton(dialog, IDC_AMS_NAV_REFUND, L"注销卡", true);
        SetNavButton(dialog, IDC_AMS_NAV_CANCEL_CARD, L"退出登录", true);
        SetNavButton(dialog, IDC_AMS_NAV_BILLING, L"", false);
        SetNavButton(dialog, IDC_AMS_NAV_STAT, L"", false);
        SetNavButton(dialog, IDC_AMS_NAV_EXIT, L"", false);
        ConfigureInput(dialog, L"", false, L"", false, L"", false, false);
        SetText(dialog, IDC_AMS_SUBMIT, mode == GuiMode::UserBalance ? L"查询余额" : (mode == GuiMode::UserStart ? L"上机" : L"下机"));
        SetStatus(dialog, std::wstring(L"用户：当前登录卡号 ") + GuiUtf8ToWide(state->session.cardName) + L"，默认作用于当前卡。");
        break;
    case GuiMode::UserRecharge:
    case GuiMode::UserRefund:
        ConfigureInput(dialog, mode == GuiMode::UserRecharge ? L"充值金额(元)" : L"退费金额(元)", true, L"", false, L"", false, false);
        SetText(dialog, IDC_AMS_SUBMIT, mode == GuiMode::UserRecharge ? L"充值" : L"退费");
        SetStatus(dialog, L"用户资金操作：默认作用于当前卡。");
        break;
    case GuiMode::UserCancel:
        ConfigureInput(dialog, L"确认卡号", true, L"确认密码", true, L"", false, true);
        SetText(dialog, IDC_AMS_SUBMIT, L"注销");
        SetStatus(dialog, L"注销卡需要再次输入卡号和密码确认。");
        break;
    }
}

static void LogoutToLogin(HWND dialog, GuiState *state)
{
    bizLogout(&state->session);
    SwitchMode(dialog, state, GuiMode::AuthAdmin);
    ShowMessageRow(state->listHandle, L"已退出登录。");
}

static void ExecuteSubmit(HWND dialog, GuiState *state)
{
    std::string text1 = GuiWideToUtf8(ReadText(dialog, IDC_AMS_CARD_NAME));
    std::string text2 = GuiWideToUtf8(ReadText(dialog, IDC_AMS_CARD_PASSWORD));
    std::string text3 = GuiWideToUtf8(ReadText(dialog, IDC_AMS_CARD_MONEY));
    BizResult result = BIZ_OK;

    switch (state->mode) {
    case GuiMode::AuthAdmin:
        result = bizAdminLogin(text1.c_str(), text2.c_str(), &state->session);
        if (result == BIZ_OK) {
            SwitchMode(dialog, state, GuiMode::AdminQuery);
            ShowMessageRow(state->listHandle, L"管理员登录成功。");
        } else {
            SetStatus(dialog, ErrorText(result));
        }
        break;
    case GuiMode::AuthUserLogin:
        result = bizUserLogin(text1.c_str(), text2.c_str(), &state->session);
        if (result == BIZ_OK) {
            SwitchMode(dialog, state, GuiMode::UserBalance);
            ShowMessageRow(state->listHandle, L"用户登录成功。");
        } else {
            SetStatus(dialog, ErrorText(result));
        }
        break;
    case GuiMode::AuthRegister: {
        Card card = {};
        result = bizUserRegister(text1.c_str(), text2.c_str(), &card);
        if (result == BIZ_OK) {
            MessageBoxW(dialog, L"注册成功！你已获得 100 元新手礼包。", L"Promotion", MB_OK | MB_ICONINFORMATION);
            ShowCard(state->listHandle, card);
            SetStatus(dialog, L"注册成功，可直接使用用户登录。");
        } else {
            SetStatus(dialog, ErrorText(result));
        }
        break;
    }
    case GuiMode::AdminQuery: {
        Card card = {};
        result = bizAdminQueryCard(&state->session, text1.c_str(), &card);
        if (result == BIZ_OK) {
            ShowCard(state->listHandle, card);
            SetStatus(dialog, L"查询完成。");
        } else {
            SetStatus(dialog, ErrorText(result));
        }
        break;
    }
    case GuiMode::AdminStop: {
        SettleInfo info = {};
        result = bizAdminStopBilling(&state->session, text1.c_str(), time(nullptr), &info);
        if (result == BIZ_OK) {
            ShowSettle(state->listHandle, info);
            SetStatus(dialog, L"管理员下机成功。");
        } else {
            SetStatus(dialog, ErrorText(result));
        }
        break;
    }
    case GuiMode::AdminRecharge:
    case GuiMode::AdminRefund: {
        Money money = {};
        Card card = {};
        if (state->mode == GuiMode::AdminRecharge) {
            result = bizAdminRecharge(&state->session, text1.c_str(), text3.c_str(), &money, &card);
        } else {
            result = bizAdminRefundByAmount(&state->session, text1.c_str(), text3.c_str(), &money, &card);
        }
        if (result == BIZ_OK) {
            ShowMoney(state->listHandle, card, money, state->mode == GuiMode::AdminRecharge ? L"充值金额" : L"退费金额");
            SetStatus(dialog, state->mode == GuiMode::AdminRecharge ? L"管理员充值成功。" : L"管理员退费成功。");
        } else {
            SetStatus(dialog, ErrorText(result));
        }
        break;
    }
    case GuiMode::AdminStatistics: {
        BillingStatistics statistics = {};
        result = bizAdminGetBillingStatistics(&state->session, text1.c_str(), &statistics);
        if (result == BIZ_OK) {
            ShowStatistics(state->listHandle, statistics);
            SetStatus(dialog, L"营业额统计完成。");
        } else {
            SetStatus(dialog, ErrorText(result));
        }
        break;
    }
    case GuiMode::UserBalance: {
        Card card = {};
        result = bizUserQueryBalance(&state->session, &card);
        if (result == BIZ_OK) {
            ShowCard(state->listHandle, card);
            SetStatus(dialog, L"余额查询完成。");
        } else {
            SetStatus(dialog, ErrorText(result));
        }
        break;
    }
    case GuiMode::UserStart: {
        LogonInfo info = {};
        result = bizUserStartBilling(&state->session, time(nullptr), &info);
        if (result == BIZ_OK) {
            ShowLogon(state->listHandle, info);
            SetStatus(dialog, L"上机成功。");
        } else {
            SetStatus(dialog, ErrorText(result));
        }
        break;
    }
    case GuiMode::UserStop: {
        SettleInfo info = {};
        result = bizUserStopBilling(&state->session, time(nullptr), &info);
        if (result == BIZ_OK) {
            ShowSettle(state->listHandle, info);
            SetStatus(dialog, L"下机成功。");
        } else {
            SetStatus(dialog, ErrorText(result));
        }
        break;
    }
    case GuiMode::UserRecharge:
    case GuiMode::UserRefund: {
        Money money = {};
        Card card = {};
        if (state->mode == GuiMode::UserRecharge) {
            result = bizUserRecharge(&state->session, text1.c_str(), &money, &card);
        } else {
            result = bizUserRefundByAmount(&state->session, text1.c_str(), &money, &card);
        }
        if (result == BIZ_OK) {
            ShowMoney(state->listHandle, card, money, state->mode == GuiMode::UserRecharge ? L"充值金额" : L"退费金额");
            SetStatus(dialog, state->mode == GuiMode::UserRecharge ? L"充值成功。" : L"退费成功。");
        } else {
            SetStatus(dialog, ErrorText(result));
        }
        break;
    }
    case GuiMode::UserCancel: {
        Money money = {};
        Card card = {};
        result = bizUserCancelCardWithPassword(&state->session, text1.c_str(), text2.c_str(), &money, &card);
        if (result == BIZ_OK) {
            ShowMoney(state->listHandle, card, money, L"退款金额");
            MessageBoxW(dialog, L"注销卡成功，当前用户已退出登录。", L"注销卡", MB_OK | MB_ICONINFORMATION);
            bizLogout(&state->session);
            SwitchMode(dialog, state, GuiMode::AuthAdmin);
        } else {
            SetStatus(dialog, ErrorText(result));
        }
        break;
    }
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
        SwitchMode(dialog, initState, GuiMode::AuthAdmin);
        return TRUE;
    }
    case WM_COMMAND:
        if (state == nullptr) {
            return FALSE;
        }

        switch (LOWORD(wParam)) {
        case IDC_AMS_SUBMIT:
            ExecuteSubmit(dialog, state);
            return TRUE;
        case IDC_AMS_NAV_ADD_CARD:
            if (bizIsAdminSession(&state->session)) {
                SwitchMode(dialog, state, GuiMode::AdminQuery);
            } else if (bizIsUserSession(&state->session)) {
                SwitchMode(dialog, state, GuiMode::UserBalance);
            } else {
                SwitchMode(dialog, state, GuiMode::AuthAdmin);
            }
            return TRUE;
        case IDC_AMS_NAV_QUERY_CARD:
            if (bizIsAdminSession(&state->session)) {
                SwitchMode(dialog, state, GuiMode::AdminStop);
            } else if (bizIsUserSession(&state->session)) {
                SwitchMode(dialog, state, GuiMode::UserStart);
            } else {
                SwitchMode(dialog, state, GuiMode::AuthUserLogin);
            }
            return TRUE;
        case IDC_AMS_NAV_LOGON:
            if (bizIsAdminSession(&state->session)) {
                SwitchMode(dialog, state, GuiMode::AdminRecharge);
            } else if (bizIsUserSession(&state->session)) {
                SwitchMode(dialog, state, GuiMode::UserStop);
            } else {
                SwitchMode(dialog, state, GuiMode::AuthRegister);
            }
            return TRUE;
        case IDC_AMS_NAV_SETTLE:
            if (bizIsAdminSession(&state->session)) {
                SwitchMode(dialog, state, GuiMode::AdminRefund);
            } else if (bizIsUserSession(&state->session)) {
                SwitchMode(dialog, state, GuiMode::UserRecharge);
            } else {
                EndDialog(dialog, 0);
            }
            return TRUE;
        case IDC_AMS_NAV_RECHARGE:
            if (bizIsAdminSession(&state->session)) {
                SwitchMode(dialog, state, GuiMode::AdminStatistics);
            } else if (bizIsUserSession(&state->session)) {
                SwitchMode(dialog, state, GuiMode::UserRefund);
            }
            return TRUE;
        case IDC_AMS_NAV_REFUND:
            if (bizIsAdminSession(&state->session)) {
                LogoutToLogin(dialog, state);
            } else if (bizIsUserSession(&state->session)) {
                SwitchMode(dialog, state, GuiMode::UserCancel);
            }
            return TRUE;
        case IDC_AMS_NAV_CANCEL_CARD:
            if (bizIsUserSession(&state->session)) {
                LogoutToLogin(dialog, state);
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
