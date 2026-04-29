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


void GuiExecuteSubmit(HWND dialog, GuiState *state)
{
    std::string text1 = GuiWideToUtf8(ReadText(dialog, IDC_AMS_CARD_NAME));
    std::string text2 = GuiWideToUtf8(ReadText(dialog, IDC_AMS_CARD_PASSWORD));
    std::string text3 = GuiWideToUtf8(ReadText(dialog, IDC_AMS_CARD_MONEY));
    BizResult result = BIZ_OK;

    switch (state->mode) {
    case GuiMode::AuthAdmin:
        result = bizAdminLogin(text1.c_str(), text2.c_str(), &state->session);
        if (result == BIZ_OK) {
            GuiSwitchMode(dialog, state, GuiMode::AdminQuery);
            GuiShowMessageRow(state->listHandle, L"管理员登录成功。");
        } else {
            SetStatus(dialog, ErrorText(result));
        }
        break;
    case GuiMode::AuthUserLogin:
        result = bizUserLogin(text1.c_str(), text2.c_str(), &state->session);
        if (result == BIZ_OK) {
            GuiSwitchMode(dialog, state, GuiMode::UserBalance);
            GuiShowMessageRow(state->listHandle, L"用户登录成功。");
        } else {
            SetStatus(dialog, ErrorText(result));
        }
        break;
    case GuiMode::AuthRegister: {
        Card card = {};
        result = bizUserRegister(text1.c_str(), text2.c_str(), &card);
        if (result == BIZ_OK) {
            MessageBoxW(dialog, L"注册成功！你已获得 100 元新手礼包。", L"Promotion", MB_OK | MB_ICONINFORMATION);
            GuiShowCard(state->listHandle, card);
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
            GuiShowCard(state->listHandle, card);
            SetStatus(dialog, L"查询完成。");
        } else {
            SetStatus(dialog, ErrorText(result));
        }
        break;
    }
    case GuiMode::AdminFuzzyQuery: {
        Card *cards = nullptr;
        size_t actualCount = 0;
        size_t requiredCount = 0;

        result = bizAdminQueryCardsByKeyword(&state->session,
                                             text1.c_str(),
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
                                                 text1.c_str(),
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
        break;
    }
    case GuiMode::AdminAdvancedQuery: {
        CardQueryOption option = {};
        Card *cards = nullptr;
        size_t count = 0;
        result = BuildAdvancedQueryOption(text1, text2, text3, &option);
        if (result == BIZ_OK) {
            result = bizAdminQueryCardsAdvanced(&state->session, &option, &cards, &count);
        }
        if (result == BIZ_OK) {
            GuiShowCards(state->listHandle, cards, count);
            SetStatus(dialog,
                L"高级查询完成。筛选=" + GuiUtf8ToWide(bizGetCardQueryFilterText(option.filterType)) +
                L"；排序=" + GuiUtf8ToWide(bizGetCardQuerySortText(option.sortType)) +
                L"；结果数=" + std::to_wstring(count) + L"。");
            bizFreeCardQueryResult(cards);
        } else {
            SetStatus(dialog, ErrorText(result));
        }
        break;
    }
    case GuiMode::AdminStop: {
        SettleInfo info = {};
        result = bizAdminStopBilling(&state->session, text1.c_str(), time(nullptr), &info);
        if (result == BIZ_OK) {
            GuiShowSettle(state->listHandle, info);
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
            GuiShowMoney(state->listHandle, card, money, state->mode == GuiMode::AdminRecharge ? L"充值金额" : L"退费金额");
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
            GuiShowStatistics(state->listHandle, statistics);
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
            GuiShowCard(state->listHandle, card);
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
            GuiShowLogon(state->listHandle, info);
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
            GuiShowSettle(state->listHandle, info);
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
            GuiShowMoney(state->listHandle, card, money, state->mode == GuiMode::UserRecharge ? L"充值金额" : L"退费金额");
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
            GuiShowMoney(state->listHandle, card, money, L"退款金额");
            MessageBoxW(dialog, L"注销卡成功，当前用户已退出登录。", L"注销卡", MB_OK | MB_ICONINFORMATION);
            bizLogout(&state->session);
            GuiSwitchMode(dialog, state, GuiMode::AuthAdmin);
        } else {
            SetStatus(dialog, ErrorText(result));
        }
        break;
    }
    }
}

