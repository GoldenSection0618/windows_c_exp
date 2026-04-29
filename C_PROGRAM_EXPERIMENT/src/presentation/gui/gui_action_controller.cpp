#include "gui_main_window_internal.h"

#include "card_query.h"
#include "card_validator.h"
#include "gui_resource.h"
#include "gui_utils.h"

#include <windows.h>

#include <ctime>
#include <cstdlib>
#include <string>

static std::wstring ReadText(HWND dialog, int controlId)
{
    wchar_t buffer[256] = {0};
    GetDlgItemTextW(dialog, controlId, buffer, static_cast<int>(sizeof(buffer) / sizeof(buffer[0])));
    return std::wstring(buffer);
}


static void SetStatus(HWND dialog, const std::wstring &text)
{
    SetDlgItemTextW(dialog, IDC_AMS_STATUS_TEXT, text.c_str());
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


static int ParseGuiInt(const std::string &text, int defaultValue)
{
    char *endptr = nullptr;
    long value = 0;

    if (text.empty()) {
        return defaultValue;
    }

    value = strtol(text.c_str(), &endptr, 10);
    if (endptr == text.c_str() || *endptr != '\0') {
        return -1;
    }
    return static_cast<int>(value);
}


static BizResult BuildAdvancedQueryOption(const std::string &filterText,
                                          const std::string &sortText,
                                          const std::string &limitText,
                                          CardQueryOption *option)
{
    int filter = ParseGuiInt(filterText, CARD_QUERY_FILTER_ALL);
    int sort = ParseGuiInt(sortText, CARD_QUERY_SORT_BALANCE_DESC);
    int32_t limitCent = 0;

    if (option == nullptr) {
        return BIZ_ERR_SYSTEM;
    }
    if (filter < CARD_QUERY_FILTER_ALL || filter > CARD_QUERY_FILTER_LOW_BALANCE ||
        sort < CARD_QUERY_SORT_BALANCE_ASC || sort > CARD_QUERY_SORT_LAST_USE_DESC) {
        return BIZ_ERR_SYSTEM;
    }

    option->filterType = static_cast<CardQueryFilterType>(filter);
    option->sortType = static_cast<CardQuerySortType>(sort);
    option->lowBalanceLimitCent = 0;

    if (option->filterType == CARD_QUERY_FILTER_LOW_BALANCE) {
        if (limitText.empty() || validatorParseMoneyToCent(limitText.c_str(), &limitCent) != MONEY_PARSE_OK || limitCent <= 0) {
            return BIZ_ERR_INVALID_AMOUNT;
        }
        option->lowBalanceLimitCent = limitCent;
    }

    return BIZ_OK;
}


void GuiLogoutToLogin(HWND dialog, GuiState *state)
{
    bizLogout(&state->session);
    GuiSwitchMode(dialog, state, GuiMode::AuthAdmin);
    GuiShowMessageRow(state->listHandle, L"已退出登录。");
}


static void HandleAdminLogin(HWND dialog, GuiState *state, const std::string &account, const std::string &password)
{
    BizResult result = bizAdminLogin(account.c_str(), password.c_str(), &state->session);
    if (result == BIZ_OK) {
        GuiSwitchMode(dialog, state, GuiMode::AdminQuery);
        GuiShowMessageRow(state->listHandle, L"管理员登录成功。");
    } else {
        SetStatus(dialog, ErrorText(result));
    }
}


static void HandleUserLogin(HWND dialog, GuiState *state, const std::string &cardName, const std::string &password)
{
    BizResult result = bizUserLogin(cardName.c_str(), password.c_str(), &state->session);
    if (result == BIZ_OK) {
        GuiSwitchMode(dialog, state, GuiMode::UserBalance);
        GuiShowMessageRow(state->listHandle, L"用户登录成功。");
    } else {
        SetStatus(dialog, ErrorText(result));
    }
}


static void HandleRegister(HWND dialog, GuiState *state, const std::string &cardName, const std::string &password)
{
    Card card = {};
    BizResult result = bizUserRegister(cardName.c_str(), password.c_str(), &card);
    if (result == BIZ_OK) {
        MessageBoxW(dialog, L"注册成功！你已获得 100 元新手礼包。", L"Promotion", MB_OK | MB_ICONINFORMATION);
        GuiShowCard(state->listHandle, card);
        SetStatus(dialog, L"注册成功，可直接使用用户登录。");
    } else {
        SetStatus(dialog, ErrorText(result));
    }
}


static void HandleAdminQuery(HWND dialog, GuiState *state, const std::string &cardName)
{
    Card card = {};
    BizResult result = bizAdminQueryCard(&state->session, cardName.c_str(), &card);
    if (result == BIZ_OK) {
        GuiShowCard(state->listHandle, card);
        SetStatus(dialog, L"查询完成。");
    } else {
        SetStatus(dialog, ErrorText(result));
    }
}


static void HandleAdminFuzzyQuery(HWND dialog, GuiState *state, const std::string &keyword)
{
    Card *cards = nullptr;
    size_t actualCount = 0;
    size_t requiredCount = 0;

    BizResult result = bizAdminQueryCardsByKeyword(&state->session,
                                                   keyword.c_str(),
                                                   nullptr,
                                                   0,
                                                   &actualCount,
                                                   &requiredCount);
    if (result == BIZ_OK) {
        cards = static_cast<Card *>(malloc(requiredCount * sizeof(Card)));
        if (cards == nullptr) {
            result = BIZ_ERR_NO_MEMORY;
        }
    }
    if (result == BIZ_OK) {
        result = bizAdminQueryCardsByKeyword(&state->session,
                                             keyword.c_str(),
                                             cards,
                                             requiredCount,
                                             &actualCount,
                                             &requiredCount);
    }
    if (result == BIZ_OK) {
        GuiShowCards(state->listHandle, cards, actualCount);
        SetStatus(dialog, L"模糊查询完成。结果数=" + std::to_wstring(actualCount) + L"。");
    } else {
        SetStatus(dialog, ErrorText(result));
    }
    if (cards != nullptr) {
        free(cards);
    }
}


static void HandleAdminAdvancedQuery(HWND dialog,
                                     GuiState *state,
                                     const std::string &filterText,
                                     const std::string &sortText,
                                     const std::string &limitText)
{
    CardQueryOption option = {};
    Card *cards = nullptr;
    size_t count = 0;
    BizResult result = BuildAdvancedQueryOption(filterText, sortText, limitText, &option);
    if (result == BIZ_OK) {
        result = bizAdminQueryCardsAdvanced(&state->session, &option, &cards, &count);
    }
    if (result == BIZ_OK) {
        GuiShowCards(state->listHandle, cards, count);
        SetStatus(dialog,
            L"高级查询完成。筛选=" + GuiUtf8ToWide(bizGetCardQueryFilterText(option.filterType)) +
            L"；排序=" + GuiUtf8ToWide(bizGetCardQuerySortText(option.sortType)) +
            L"；结果数=" + std::to_wstring(count) + L"。");
    } else {
        SetStatus(dialog, ErrorText(result));
    }
    if (cards != nullptr) {
        bizFreeCardQueryResult(cards);
    }
}


static void HandleAdminStop(HWND dialog, GuiState *state, const std::string &cardName)
{
    SettleInfo info = {};
    BizResult result = bizAdminStopBilling(&state->session, cardName.c_str(), time(nullptr), &info);
    if (result == BIZ_OK) {
        GuiShowSettle(state->listHandle, info);
        SetStatus(dialog, L"管理员下机成功。");
    } else {
        SetStatus(dialog, ErrorText(result));
    }
}


static void HandleAdminMoney(HWND dialog, GuiState *state, const std::string &cardName, const std::string &amountText)
{
    Money money = {};
    Card card = {};
    BizResult result = BIZ_OK;
    if (state->mode == GuiMode::AdminRecharge) {
        result = bizAdminRecharge(&state->session, cardName.c_str(), amountText.c_str(), &money, &card);
    } else {
        result = bizAdminRefundByAmount(&state->session, cardName.c_str(), amountText.c_str(), &money, &card);
    }
    if (result == BIZ_OK) {
        GuiShowMoney(state->listHandle, card, money, state->mode == GuiMode::AdminRecharge ? L"充值金额" : L"退费金额");
        SetStatus(dialog, state->mode == GuiMode::AdminRecharge ? L"管理员充值成功。" : L"管理员退费成功。");
    } else {
        SetStatus(dialog, ErrorText(result));
    }
}


static void HandleAdminStatistics(HWND dialog, GuiState *state, const std::string &yearMonthText)
{
    BillingStatistics statistics = {};
    BizResult result = bizAdminGetBillingStatistics(&state->session, yearMonthText.c_str(), &statistics);
    if (result == BIZ_OK) {
        GuiShowStatistics(state->listHandle, statistics);
        SetStatus(dialog, L"营业额统计完成。");
    } else {
        SetStatus(dialog, ErrorText(result));
    }
}


static void HandleUserBalance(HWND dialog, GuiState *state)
{
    Card card = {};
    BizResult result = bizUserQueryBalance(&state->session, &card);
    if (result == BIZ_OK) {
        GuiShowCard(state->listHandle, card);
        SetStatus(dialog, L"余额查询完成。");
    } else {
        SetStatus(dialog, ErrorText(result));
    }
}


static void HandleUserStart(HWND dialog, GuiState *state)
{
    LogonInfo info = {};
    BizResult result = bizUserStartBilling(&state->session, time(nullptr), &info);
    if (result == BIZ_OK) {
        GuiShowLogon(state->listHandle, info);
        SetStatus(dialog, L"上机成功。");
    } else {
        SetStatus(dialog, ErrorText(result));
    }
}


static void HandleUserStop(HWND dialog, GuiState *state)
{
    SettleInfo info = {};
    BizResult result = bizUserStopBilling(&state->session, time(nullptr), &info);
    if (result == BIZ_OK) {
        GuiShowSettle(state->listHandle, info);
        SetStatus(dialog, L"下机成功。");
    } else {
        SetStatus(dialog, ErrorText(result));
    }
}


static void HandleUserMoney(HWND dialog, GuiState *state, const std::string &amountText)
{
    Money money = {};
    Card card = {};
    BizResult result = BIZ_OK;
    if (state->mode == GuiMode::UserRecharge) {
        result = bizUserRecharge(&state->session, amountText.c_str(), &money, &card);
    } else {
        result = bizUserRefundByAmount(&state->session, amountText.c_str(), &money, &card);
    }
    if (result == BIZ_OK) {
        GuiShowMoney(state->listHandle, card, money, state->mode == GuiMode::UserRecharge ? L"充值金额" : L"退费金额");
        SetStatus(dialog, state->mode == GuiMode::UserRecharge ? L"充值成功。" : L"退费成功。");
    } else {
        SetStatus(dialog, ErrorText(result));
    }
}


static void HandleUserCancel(HWND dialog, GuiState *state, const std::string &cardName, const std::string &password)
{
    Money money = {};
    Card card = {};
    BizResult result = bizUserCancelCardWithPassword(&state->session, cardName.c_str(), password.c_str(), &money, &card);
    if (result == BIZ_OK) {
        GuiShowMoney(state->listHandle, card, money, L"退款金额");
        MessageBoxW(dialog, L"注销卡成功，当前用户已退出登录。", L"注销卡", MB_OK | MB_ICONINFORMATION);
        bizLogout(&state->session);
        GuiSwitchMode(dialog, state, GuiMode::AuthAdmin);
    } else {
        SetStatus(dialog, ErrorText(result));
    }
}


void GuiExecuteSubmit(HWND dialog, GuiState *state)
{
    std::string text1 = GuiWideToUtf8(ReadText(dialog, IDC_AMS_CARD_NAME));
    std::string text2 = GuiWideToUtf8(ReadText(dialog, IDC_AMS_CARD_PASSWORD));
    std::string text3 = GuiWideToUtf8(ReadText(dialog, IDC_AMS_CARD_MONEY));

    switch (state->mode) {
    case GuiMode::AuthAdmin:
        HandleAdminLogin(dialog, state, text1, text2);
        break;
    case GuiMode::AuthUserLogin:
        HandleUserLogin(dialog, state, text1, text2);
        break;
    case GuiMode::AuthRegister:
        HandleRegister(dialog, state, text1, text2);
        break;
    case GuiMode::AdminQuery:
        HandleAdminQuery(dialog, state, text1);
        break;
    case GuiMode::AdminFuzzyQuery:
        HandleAdminFuzzyQuery(dialog, state, text1);
        break;
    case GuiMode::AdminAdvancedQuery:
        HandleAdminAdvancedQuery(dialog, state, text1, text2, text3);
        break;
    case GuiMode::AdminStop:
        HandleAdminStop(dialog, state, text1);
        break;
    case GuiMode::AdminRecharge:
    case GuiMode::AdminRefund:
        HandleAdminMoney(dialog, state, text1, text3);
        break;
    case GuiMode::AdminStatistics:
        HandleAdminStatistics(dialog, state, text1);
        break;
    case GuiMode::UserBalance:
        HandleUserBalance(dialog, state);
        break;
    case GuiMode::UserStart:
        HandleUserStart(dialog, state);
        break;
    case GuiMode::UserStop:
        HandleUserStop(dialog, state);
        break;
    case GuiMode::UserRecharge:
    case GuiMode::UserRefund:
        HandleUserMoney(dialog, state, text1);
        break;
    case GuiMode::UserCancel:
        HandleUserCancel(dialog, state, text1, text2);
        break;
    }
}

