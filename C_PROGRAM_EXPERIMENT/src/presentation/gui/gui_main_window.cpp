#include "gui_main_window.h"

#include "business.h"
#include "gui_resource.h"
#include "gui_utils.h"

#include <commctrl.h>
#include <time.h>

#include <array>
#include <string>
#include <vector>

#pragma comment(lib, "Comctl32.lib")

enum class GuiFeature {
    AddCard,
    QueryCard,
    Logon,
    Settle,
    Recharge,
    Refund,
    CancelCard,
    Billing,
    Statistics
};

struct GuiState {
    GuiFeature feature;
    HWND listHandle;
};

static std::wstring ReadControlText(HWND dialog, int controlId)
{
    wchar_t buffer[256] = {0};
    GetDlgItemTextW(dialog, controlId, buffer, static_cast<int>(sizeof(buffer) / sizeof(buffer[0])));
    return std::wstring(buffer);
}

static void SetStatusText(HWND dialog, const std::wstring &text)
{
    SetDlgItemTextW(dialog, IDC_AMS_STATUS_TEXT, text.c_str());
}

static void SetLabels(HWND dialog, const wchar_t *label1, const wchar_t *label2, const wchar_t *label3)
{
    SetDlgItemTextW(dialog, IDC_AMS_LABEL_1, label1 == nullptr ? L"" : label1);
    SetDlgItemTextW(dialog, IDC_AMS_LABEL_2, label2 == nullptr ? L"" : label2);
    SetDlgItemTextW(dialog, IDC_AMS_LABEL_3, label3 == nullptr ? L"" : label3);
}

static void SetEditVisible(HWND dialog, int controlId, bool visible)
{
    ShowWindow(GetDlgItem(dialog, controlId), visible ? SW_SHOW : SW_HIDE);
}

static void SetLabelVisible(HWND dialog, int controlId, bool visible)
{
    ShowWindow(GetDlgItem(dialog, controlId), visible ? SW_SHOW : SW_HIDE);
}

static void ConfigurePasswordMask(HWND dialog, bool enabled)
{
    HWND passwordEdit = GetDlgItem(dialog, IDC_AMS_CARD_PASSWORD);
    SendMessageW(passwordEdit, EM_SETPASSWORDCHAR, enabled ? L'*' : 0, 0);
    InvalidateRect(passwordEdit, nullptr, TRUE);
}

static void ResetInput(HWND dialog)
{
    SetDlgItemTextW(dialog, IDC_AMS_CARD_NAME, L"");
    SetDlgItemTextW(dialog, IDC_AMS_CARD_PASSWORD, L"");
    SetDlgItemTextW(dialog, IDC_AMS_CARD_MONEY, L"");
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

static void AddListColumn(HWND listHandle, int index, int width, const wchar_t *title)
{
    LVCOLUMNW column = {0};
    column.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT;
    column.fmt = LVCFMT_LEFT;
    column.cx = width;
    column.pszText = const_cast<LPWSTR>(title);
    ListView_InsertColumn(listHandle, index, &column);
}

static void AddListRow(HWND listHandle, int row, const std::vector<std::wstring> &values)
{
    LVITEMW item = {0};
    item.mask = LVIF_TEXT;
    item.iItem = row;
    item.pszText = const_cast<LPWSTR>(values[0].c_str());
    ListView_InsertItem(listHandle, &item);

    for (size_t i = 1; i < values.size(); ++i) {
        ListView_SetItemText(listHandle, row, static_cast<int>(i), const_cast<LPWSTR>(values[i].c_str()));
    }
}

static void ShowCardQueryResult(HWND listHandle, const std::vector<Card> &cards)
{
    int row = 0;
    PrepareList(listHandle);
    AddListColumn(listHandle, 0, 160, L"卡号");
    AddListColumn(listHandle, 1, 100, L"状态");
    AddListColumn(listHandle, 2, 100, L"余额");
    AddListColumn(listHandle, 3, 120, L"累计消费");
    AddListColumn(listHandle, 4, 100, L"使用次数");
    AddListColumn(listHandle, 5, 210, L"最后使用时间");

    for (const Card &card : cards) {
        AddListRow(listHandle, row++, {
            GuiUtf8ToWide(card.aCardName),
            GuiCardStatusText(card.nStatus),
            GuiFormatMoneyFromCent(card.nBalanceCent),
            GuiFormatMoneyFromCent(card.nTotalUseCent),
            std::to_wstring(card.nUseCount),
            GuiFormatTime(card.tLast)
        });
    }
}

static void ShowSingleResult(HWND listHandle, const std::vector<std::wstring> &columns, const std::vector<std::wstring> &values)
{
    int i = 0;
    PrepareList(listHandle);
    for (i = 0; i < static_cast<int>(columns.size()); ++i) {
        AddListColumn(listHandle, i, 180, columns[i].c_str());
    }
    AddListRow(listHandle, 0, values);
}

static void ShowBillingResults(HWND listHandle, const Billing *items, size_t count)
{
    int row = 0;
    size_t i = 0;

    PrepareList(listHandle);
    AddListColumn(listHandle, 0, 160, L"卡号");
    AddListColumn(listHandle, 1, 190, L"上机时间");
    AddListColumn(listHandle, 2, 190, L"下机时间");
    AddListColumn(listHandle, 3, 100, L"消费金额");
    AddListColumn(listHandle, 4, 90, L"状态");

    for (i = 0; i < count; ++i) {
        AddListRow(listHandle, row++, {
            GuiUtf8ToWide(items[i].aCardName),
            GuiFormatTime(items[i].tStart),
            GuiFormatTime(items[i].tEnd),
            GuiFormatMoneyFromCent(items[i].nAmountCent),
            GuiBillingStatusText(items[i].nStatus)
        });
    }
}

static void SwitchFeature(HWND dialog, GuiState *state, GuiFeature feature)
{
    state->feature = feature;
    ResetInput(dialog);

    switch (feature) {
    case GuiFeature::AddCard:
        SetLabels(dialog, L"卡号", L"密码", L"初始金额(元)");
        SetEditVisible(dialog, IDC_AMS_CARD_NAME, true);
        SetEditVisible(dialog, IDC_AMS_CARD_PASSWORD, true);
        SetEditVisible(dialog, IDC_AMS_CARD_MONEY, true);
        SetLabelVisible(dialog, IDC_AMS_LABEL_1, true);
        SetLabelVisible(dialog, IDC_AMS_LABEL_2, true);
        SetLabelVisible(dialog, IDC_AMS_LABEL_3, true);
        ConfigurePasswordMask(dialog, true);
        SetStatusText(dialog, L"添加卡：输入卡号、密码、初始金额。\n");
        break;
    case GuiFeature::QueryCard:
        SetLabels(dialog, L"卡号关键字", L"", L"");
        SetEditVisible(dialog, IDC_AMS_CARD_NAME, true);
        SetEditVisible(dialog, IDC_AMS_CARD_PASSWORD, false);
        SetEditVisible(dialog, IDC_AMS_CARD_MONEY, false);
        SetLabelVisible(dialog, IDC_AMS_LABEL_1, true);
        SetLabelVisible(dialog, IDC_AMS_LABEL_2, false);
        SetLabelVisible(dialog, IDC_AMS_LABEL_3, false);
        ConfigurePasswordMask(dialog, false);
        SetStatusText(dialog, L"查询卡：支持关键字查询。\n");
        break;
    case GuiFeature::Logon:
        SetLabels(dialog, L"卡号", L"密码", L"");
        SetEditVisible(dialog, IDC_AMS_CARD_NAME, true);
        SetEditVisible(dialog, IDC_AMS_CARD_PASSWORD, true);
        SetEditVisible(dialog, IDC_AMS_CARD_MONEY, false);
        SetLabelVisible(dialog, IDC_AMS_LABEL_1, true);
        SetLabelVisible(dialog, IDC_AMS_LABEL_2, true);
        SetLabelVisible(dialog, IDC_AMS_LABEL_3, false);
        ConfigurePasswordMask(dialog, true);
        SetStatusText(dialog, L"上机：输入卡号与密码。\n");
        break;
    case GuiFeature::Settle:
        SetLabels(dialog, L"卡号", L"密码", L"");
        SetEditVisible(dialog, IDC_AMS_CARD_NAME, true);
        SetEditVisible(dialog, IDC_AMS_CARD_PASSWORD, true);
        SetEditVisible(dialog, IDC_AMS_CARD_MONEY, false);
        SetLabelVisible(dialog, IDC_AMS_LABEL_1, true);
        SetLabelVisible(dialog, IDC_AMS_LABEL_2, true);
        SetLabelVisible(dialog, IDC_AMS_LABEL_3, false);
        ConfigurePasswordMask(dialog, true);
        SetStatusText(dialog, L"下机：输入卡号与密码。\n");
        break;
    case GuiFeature::Recharge:
        SetLabels(dialog, L"卡号", L"密码", L"充值金额(元)");
        SetEditVisible(dialog, IDC_AMS_CARD_NAME, true);
        SetEditVisible(dialog, IDC_AMS_CARD_PASSWORD, true);
        SetEditVisible(dialog, IDC_AMS_CARD_MONEY, true);
        SetLabelVisible(dialog, IDC_AMS_LABEL_1, true);
        SetLabelVisible(dialog, IDC_AMS_LABEL_2, true);
        SetLabelVisible(dialog, IDC_AMS_LABEL_3, true);
        ConfigurePasswordMask(dialog, true);
        SetStatusText(dialog, L"充值：输入卡号、密码、金额。\n");
        break;
    case GuiFeature::Refund:
        SetLabels(dialog, L"卡号", L"密码", L"退费金额(元)");
        SetEditVisible(dialog, IDC_AMS_CARD_NAME, true);
        SetEditVisible(dialog, IDC_AMS_CARD_PASSWORD, true);
        SetEditVisible(dialog, IDC_AMS_CARD_MONEY, true);
        SetLabelVisible(dialog, IDC_AMS_LABEL_1, true);
        SetLabelVisible(dialog, IDC_AMS_LABEL_2, true);
        SetLabelVisible(dialog, IDC_AMS_LABEL_3, true);
        ConfigurePasswordMask(dialog, true);
        SetStatusText(dialog, L"退费：输入卡号、密码、金额。\n");
        break;
    case GuiFeature::CancelCard:
        SetLabels(dialog, L"卡号", L"密码", L"");
        SetEditVisible(dialog, IDC_AMS_CARD_NAME, true);
        SetEditVisible(dialog, IDC_AMS_CARD_PASSWORD, true);
        SetEditVisible(dialog, IDC_AMS_CARD_MONEY, false);
        SetLabelVisible(dialog, IDC_AMS_LABEL_1, true);
        SetLabelVisible(dialog, IDC_AMS_LABEL_2, true);
        SetLabelVisible(dialog, IDC_AMS_LABEL_3, false);
        ConfigurePasswordMask(dialog, true);
        SetStatusText(dialog, L"注销卡：输入卡号与密码。\n");
        break;
    case GuiFeature::Billing:
        SetLabels(dialog, L"卡号", L"", L"");
        SetEditVisible(dialog, IDC_AMS_CARD_NAME, true);
        SetEditVisible(dialog, IDC_AMS_CARD_PASSWORD, false);
        SetEditVisible(dialog, IDC_AMS_CARD_MONEY, false);
        SetLabelVisible(dialog, IDC_AMS_LABEL_1, true);
        SetLabelVisible(dialog, IDC_AMS_LABEL_2, false);
        SetLabelVisible(dialog, IDC_AMS_LABEL_3, false);
        ConfigurePasswordMask(dialog, false);
        SetStatusText(dialog, L"消费记录：输入卡号查询。\n");
        break;
    case GuiFeature::Statistics:
        SetLabels(dialog, L"年份(YYYY)", L"", L"");
        SetEditVisible(dialog, IDC_AMS_CARD_NAME, true);
        SetEditVisible(dialog, IDC_AMS_CARD_PASSWORD, false);
        SetEditVisible(dialog, IDC_AMS_CARD_MONEY, false);
        SetLabelVisible(dialog, IDC_AMS_LABEL_1, true);
        SetLabelVisible(dialog, IDC_AMS_LABEL_2, false);
        SetLabelVisible(dialog, IDC_AMS_LABEL_3, false);
        ConfigurePasswordMask(dialog, false);
        SetStatusText(dialog, L"营业额统计：输入年份。\n");
        break;
    }

    PrepareList(state->listHandle);
}

static void ShowError(HWND dialog, BizResult result)
{
    SetStatusText(dialog, GuiUtf8ToWide(bizGetMessage(result)));
}

static void ExecuteAddCard(HWND dialog, GuiState *state)
{
    Card card = {};
    std::string cardName = GuiWideToUtf8(ReadControlText(dialog, IDC_AMS_CARD_NAME));
    std::string password = GuiWideToUtf8(ReadControlText(dialog, IDC_AMS_CARD_PASSWORD));
    std::string amount = GuiWideToUtf8(ReadControlText(dialog, IDC_AMS_CARD_MONEY));
    BizResult result = bizAddCard(cardName.c_str(), password.c_str(), amount.c_str(), &card);

    if (result != BIZ_OK) {
        ShowError(dialog, result);
        return;
    }

    ShowSingleResult(state->listHandle,
        {L"卡号", L"状态", L"余额"},
        {GuiUtf8ToWide(card.aCardName), GuiCardStatusText(card.nStatus), GuiFormatMoneyFromCent(card.nBalanceCent)});
    SetStatusText(dialog, L"添加卡成功。\n");
}

static void ExecuteQueryCard(HWND dialog, GuiState *state)
{
    std::string keyword = GuiWideToUtf8(ReadControlText(dialog, IDC_AMS_CARD_NAME));
    size_t actual = 0;
    size_t required = 0;
    BizResult result = bizQueryCardsByKeyword(keyword.c_str(), nullptr, 0, &actual, &required);

    if (result != BIZ_OK) {
        ShowError(dialog, result);
        return;
    }

    std::vector<Card> cards(required);
    result = bizQueryCardsByKeyword(keyword.c_str(), cards.data(), cards.size(), &actual, &required);
    if (result != BIZ_OK) {
        ShowError(dialog, result);
        return;
    }

    cards.resize(actual);
    ShowCardQueryResult(state->listHandle, cards);
    SetStatusText(dialog, L"查询完成。\n");
}

static void ExecuteLogon(HWND dialog, GuiState *state)
{
    LogonInfo info = {};
    std::string cardName = GuiWideToUtf8(ReadControlText(dialog, IDC_AMS_CARD_NAME));
    std::string password = GuiWideToUtf8(ReadControlText(dialog, IDC_AMS_CARD_PASSWORD));
    BizResult result = bizStartBilling(cardName.c_str(), password.c_str(), time(nullptr), &info);

    if (result != BIZ_OK) {
        ShowError(dialog, result);
        return;
    }

    ShowSingleResult(state->listHandle,
        {L"卡号", L"余额", L"上机时间"},
        {GuiUtf8ToWide(info.aCardName), GuiFormatMoneyFromCent(info.nBalanceCent), GuiFormatTime(info.tStart)});
    SetStatusText(dialog, L"上机成功。\n");
}

static void ExecuteSettle(HWND dialog, GuiState *state)
{
    SettleInfo info = {};
    std::string cardName = GuiWideToUtf8(ReadControlText(dialog, IDC_AMS_CARD_NAME));
    std::string password = GuiWideToUtf8(ReadControlText(dialog, IDC_AMS_CARD_PASSWORD));
    BizResult result = bizStopBilling(cardName.c_str(), password.c_str(), time(nullptr), &info);

    if (result != BIZ_OK) {
        ShowError(dialog, result);
        return;
    }

    ShowSingleResult(state->listHandle,
        {L"卡号", L"上机时间", L"下机时间", L"消费金额", L"余额"},
        {GuiUtf8ToWide(info.aCardName), GuiFormatTime(info.tStart), GuiFormatTime(info.tEnd),
            GuiFormatMoneyFromCent(info.nAmountCent), GuiFormatMoneyFromCent(info.nBalanceCent)});
    SetStatusText(dialog, L"下机成功。\n");
}

static void ExecuteRecharge(HWND dialog, GuiState *state)
{
    Money money = {};
    Card card = {};
    std::string cardName = GuiWideToUtf8(ReadControlText(dialog, IDC_AMS_CARD_NAME));
    std::string password = GuiWideToUtf8(ReadControlText(dialog, IDC_AMS_CARD_PASSWORD));
    std::string amount = GuiWideToUtf8(ReadControlText(dialog, IDC_AMS_CARD_MONEY));
    BizResult result = bizRecharge(cardName.c_str(), password.c_str(), amount.c_str(), &money, &card);

    if (result != BIZ_OK) {
        ShowError(dialog, result);
        return;
    }

    ShowSingleResult(state->listHandle,
        {L"卡号", L"充值金额", L"当前余额"},
        {GuiUtf8ToWide(card.aCardName), GuiFormatMoneyFromCent(money.nMoneyCent), GuiFormatMoneyFromCent(card.nBalanceCent)});
    SetStatusText(dialog, L"充值成功。\n");
}

static void ExecuteRefund(HWND dialog, GuiState *state)
{
    Money money = {};
    Card card = {};
    std::string cardName = GuiWideToUtf8(ReadControlText(dialog, IDC_AMS_CARD_NAME));
    std::string password = GuiWideToUtf8(ReadControlText(dialog, IDC_AMS_CARD_PASSWORD));
    std::string amount = GuiWideToUtf8(ReadControlText(dialog, IDC_AMS_CARD_MONEY));
    BizResult result = bizRefundByAmount(cardName.c_str(), password.c_str(), amount.c_str(), &money, &card);

    if (result != BIZ_OK) {
        ShowError(dialog, result);
        return;
    }

    ShowSingleResult(state->listHandle,
        {L"卡号", L"退费金额", L"当前余额"},
        {GuiUtf8ToWide(card.aCardName), GuiFormatMoneyFromCent(money.nMoneyCent), GuiFormatMoneyFromCent(card.nBalanceCent)});
    SetStatusText(dialog, L"退费成功。\n");
}

static void ExecuteCancelCard(HWND dialog, GuiState *state)
{
    Money money = {};
    Card card = {};
    std::string cardName = GuiWideToUtf8(ReadControlText(dialog, IDC_AMS_CARD_NAME));
    std::string password = GuiWideToUtf8(ReadControlText(dialog, IDC_AMS_CARD_PASSWORD));
    BizResult result = bizCancelCard(cardName.c_str(), password.c_str(), &money, &card);

    if (result != BIZ_OK) {
        ShowError(dialog, result);
        return;
    }

    ShowSingleResult(state->listHandle,
        {L"卡号", L"退款金额"},
        {GuiUtf8ToWide(card.aCardName), GuiFormatMoneyFromCent(money.nMoneyCent)});
    SetStatusText(dialog, L"注销卡成功。\n");
}

static void ExecuteBilling(HWND dialog, GuiState *state)
{
    BillingQueryResult resultSet = {};
    std::string cardName = GuiWideToUtf8(ReadControlText(dialog, IDC_AMS_CARD_NAME));
    BizResult result = bizQueryBillingsByCardName(cardName.c_str(), &resultSet);

    if (result != BIZ_OK) {
        ShowError(dialog, result);
        return;
    }

    ShowBillingResults(state->listHandle, resultSet.items, resultSet.count);
    bizFreeBillingQueryResult(&resultSet);
    SetStatusText(dialog, L"消费记录查询完成。\n");
}

static void ExecuteStatistics(HWND dialog, GuiState *state)
{
    BillingStatistics statistics = {};
    std::string year = GuiWideToUtf8(ReadControlText(dialog, IDC_AMS_CARD_NAME));
    BizResult result = bizGetBillingStatistics(year.c_str(), &statistics);

    if (result != BIZ_OK) {
        ShowError(dialog, result);
        return;
    }

    PrepareList(state->listHandle);
    AddListColumn(state->listHandle, 0, 220, L"项目");
    AddListColumn(state->listHandle, 1, 220, L"金额(元)");

    AddListRow(state->listHandle, 0, {L"总营业额", GuiFormatMoneyFromCent(statistics.totalAmountCent)});
    for (int month = 0; month < 12; ++month) {
        AddListRow(state->listHandle, month + 1,
            {std::to_wstring(statistics.year) + L"年" + std::to_wstring(month + 1) + L"月",
             GuiFormatMoneyFromCent(statistics.monthlyAmountCent[month])});
    }

    SetStatusText(dialog, L"营业额统计完成。\n");
}

static void ExecuteCurrentFeature(HWND dialog, GuiState *state)
{
    switch (state->feature) {
    case GuiFeature::AddCard:
        ExecuteAddCard(dialog, state);
        break;
    case GuiFeature::QueryCard:
        ExecuteQueryCard(dialog, state);
        break;
    case GuiFeature::Logon:
        ExecuteLogon(dialog, state);
        break;
    case GuiFeature::Settle:
        ExecuteSettle(dialog, state);
        break;
    case GuiFeature::Recharge:
        ExecuteRecharge(dialog, state);
        break;
    case GuiFeature::Refund:
        ExecuteRefund(dialog, state);
        break;
    case GuiFeature::CancelCard:
        ExecuteCancelCard(dialog, state);
        break;
    case GuiFeature::Billing:
        ExecuteBilling(dialog, state);
        break;
    case GuiFeature::Statistics:
        ExecuteStatistics(dialog, state);
        break;
    }
}

static INT_PTR CALLBACK MainDialogProc(HWND dialog, UINT message, WPARAM wParam, LPARAM lParam)
{
    GuiState *state = reinterpret_cast<GuiState *>(GetWindowLongPtrW(dialog, GWLP_USERDATA));

    switch (message) {
    case WM_INITDIALOG: {
        GuiState *initState = reinterpret_cast<GuiState *>(lParam);
        HWND listHandle = GetDlgItem(dialog, IDC_AMS_RESULT_LIST);
        HFONT fontHandle = CreateFontW(-18, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
            CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Microsoft YaHei UI");

        initState->listHandle = listHandle;
        SetWindowLongPtrW(dialog, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(initState));
        SendMessageW(dialog, WM_SETFONT, reinterpret_cast<WPARAM>(fontHandle), TRUE);

        ListView_SetExtendedListViewStyle(listHandle, LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES | LVS_EX_DOUBLEBUFFER);
        SwitchFeature(dialog, initState, GuiFeature::AddCard);
        return TRUE;
    }
    case WM_COMMAND:
        if (state == nullptr) {
            return FALSE;
        }
        switch (LOWORD(wParam)) {
        case IDC_AMS_NAV_ADD_CARD:
            SwitchFeature(dialog, state, GuiFeature::AddCard);
            return TRUE;
        case IDC_AMS_NAV_QUERY_CARD:
            SwitchFeature(dialog, state, GuiFeature::QueryCard);
            return TRUE;
        case IDC_AMS_NAV_LOGON:
            SwitchFeature(dialog, state, GuiFeature::Logon);
            return TRUE;
        case IDC_AMS_NAV_SETTLE:
            SwitchFeature(dialog, state, GuiFeature::Settle);
            return TRUE;
        case IDC_AMS_NAV_RECHARGE:
            SwitchFeature(dialog, state, GuiFeature::Recharge);
            return TRUE;
        case IDC_AMS_NAV_REFUND:
            SwitchFeature(dialog, state, GuiFeature::Refund);
            return TRUE;
        case IDC_AMS_NAV_CANCEL_CARD:
            SwitchFeature(dialog, state, GuiFeature::CancelCard);
            return TRUE;
        case IDC_AMS_NAV_BILLING:
            SwitchFeature(dialog, state, GuiFeature::Billing);
            return TRUE;
        case IDC_AMS_NAV_STAT:
            SwitchFeature(dialog, state, GuiFeature::Statistics);
            return TRUE;
        case IDC_AMS_NAV_EXIT:
            EndDialog(dialog, 0);
            return TRUE;
        case IDC_AMS_SUBMIT:
            ExecuteCurrentFeature(dialog, state);
            return TRUE;
        default:
            break;
        }
        break;
    case WM_CLOSE:
        EndDialog(dialog, 0);
        return TRUE;
    default:
        break;
    }

    return FALSE;
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
