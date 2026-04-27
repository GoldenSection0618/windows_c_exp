#ifndef GUI_MAIN_WINDOW_CORE_H
#define GUI_MAIN_WINDOW_CORE_H

#include <windows.h>

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
    HWND navBackgroundHandle;
    bool layoutInitialized;
};

int DialogUnitToPixelX(HWND dialog, int value);
int DialogUnitToPixelY(HWND dialog, int value);
INT_PTR RunMainGuiDialog(HINSTANCE instanceHandle, int showCommand);

#endif
