#ifndef GUI_MAIN_WINDOW_CORE_H
#define GUI_MAIN_WINDOW_CORE_H

#include <windows.h>

#include "business.h"

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

int DialogUnitToPixelX(HWND dialog, int value);
int DialogUnitToPixelY(HWND dialog, int value);
INT_PTR RunMainGuiDialog(HINSTANCE instanceHandle, int showCommand);

#endif
