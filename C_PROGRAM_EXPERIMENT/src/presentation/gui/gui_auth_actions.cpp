#include "gui_auth_actions.h"

#include "gui_action_utils.h"

void GuiLogoutToLogin(HWND dialog, GuiState *state)
{
    bizLogout(&state->session);
    GuiSwitchMode(dialog, state, GuiMode::AuthAdmin);
    GuiShowMessageRow(state->listHandle, L"已退出登录。");
}

void GuiHandleAdminLogin(HWND dialog, GuiState *state, const std::string &account, const std::string &password)
{
    BizResult result = bizAdminLogin(account.c_str(), password.c_str(), &state->session);
    if (result == BIZ_OK) {
        GuiSwitchMode(dialog, state, GuiMode::AdminQuery);
        GuiShowMessageRow(state->listHandle, L"管理员登录成功。");
    } else {
        GuiSetStatus(dialog, GuiErrorText(result));
    }
}

void GuiHandleUserLogin(HWND dialog, GuiState *state, const std::string &cardName, const std::string &password)
{
    BizResult result = bizUserLogin(cardName.c_str(), password.c_str(), &state->session);
    if (result == BIZ_OK) {
        GuiSwitchMode(dialog, state, GuiMode::UserBalance);
        GuiShowMessageRow(state->listHandle, L"用户登录成功。");
    } else {
        GuiSetStatus(dialog, GuiErrorText(result));
    }
}

void GuiHandleRegister(HWND dialog, GuiState *state, const std::string &cardName, const std::string &password)
{
    Card card = {};
    BizResult result = bizUserRegister(cardName.c_str(), password.c_str(), &card);
    if (result == BIZ_OK) {
        MessageBoxW(dialog, L"注册成功！你已获得 100 元新手礼包。", L"Promotion", MB_OK | MB_ICONINFORMATION);
        GuiShowCard(state->listHandle, card);
        GuiSetStatus(dialog, L"注册成功，可直接使用用户登录。");
    } else {
        GuiSetStatus(dialog, GuiErrorText(result));
    }
}
