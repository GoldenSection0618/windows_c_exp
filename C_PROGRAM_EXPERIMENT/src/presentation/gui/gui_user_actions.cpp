#include "gui_user_actions.h"

#include "gui_action_utils.h"

#include <ctime>

void GuiHandleUserBalance(HWND dialog, GuiState *state)
{
    Card card = {};
    BizResult result = bizUserQueryBalance(&state->session, &card);
    if (result == BIZ_OK) {
        GuiShowCard(state->listHandle, card);
        GuiSetStatus(dialog, L"余额查询完成。");
    } else {
        GuiSetStatus(dialog, GuiErrorText(result));
    }
}

void GuiHandleUserStart(HWND dialog, GuiState *state)
{
    LogonInfo info = {};
    BizResult result = bizUserStartBilling(&state->session, time(nullptr), &info);
    if (result == BIZ_OK) {
        GuiShowLogon(state->listHandle, info);
        GuiSetStatus(dialog, L"上机成功。");
    } else {
        GuiSetStatus(dialog, GuiErrorText(result));
    }
}

void GuiHandleUserStop(HWND dialog, GuiState *state)
{
    SettleInfo info = {};
    BizResult result = bizUserStopBilling(&state->session, time(nullptr), &info);
    if (result == BIZ_OK) {
        GuiShowSettle(state->listHandle, info);
        GuiSetStatus(dialog, L"下机成功。");
    } else {
        GuiSetStatus(dialog, GuiErrorText(result));
    }
}

void GuiHandleUserMoney(HWND dialog, GuiState *state, const std::string &amountText)
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
        GuiSetStatus(dialog, state->mode == GuiMode::UserRecharge ? L"充值成功。" : L"退费成功。");
    } else {
        GuiSetStatus(dialog, GuiErrorText(result));
    }
}

void GuiHandleUserCancel(HWND dialog, GuiState *state, const std::string &cardName, const std::string &password)
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
        GuiSetStatus(dialog, GuiErrorText(result));
    }
}
