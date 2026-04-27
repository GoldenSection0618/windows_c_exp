#ifndef GUI_MAIN_WINDOW_INTERNAL_H
#define GUI_MAIN_WINDOW_INTERNAL_H

#include "business.h"

#include <windows.h>

enum class GuiMode {
    AuthAdmin,
    AuthUserLogin,
    AuthRegister,
    AdminQuery,
    AdminStop,
    AdminRecharge,
    AdminRefund,
    AdminStatistics,
    UserBalance,
    UserStart,
    UserStop,
    UserRecharge,
    UserRefund,
    UserCancel
};

struct GuiState {
    LoginSession session;
    GuiMode mode;
    HWND listHandle;
};

void GuiPrepareList(HWND listHandle);
void GuiShowMessageRow(HWND listHandle, const wchar_t *message);
void GuiShowCard(HWND listHandle, const Card &card);
void GuiShowMoney(HWND listHandle, const Card &card, const Money &money, const wchar_t *title);
void GuiShowSettle(HWND listHandle, const SettleInfo &info);
void GuiShowLogon(HWND listHandle, const LogonInfo &info);
void GuiShowStatistics(HWND listHandle, const BillingStatistics &statistics);

void GuiSwitchMode(HWND dialog, GuiState *state, GuiMode mode);
void GuiExecuteSubmit(HWND dialog, GuiState *state);
void GuiLogoutToLogin(HWND dialog, GuiState *state);

#endif
