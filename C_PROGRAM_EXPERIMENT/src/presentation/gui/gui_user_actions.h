#ifndef GUI_USER_ACTIONS_H
#define GUI_USER_ACTIONS_H

#include "gui_main_window_internal.h"

#include <windows.h>

#include <string>

void GuiHandleUserBalance(HWND dialog, GuiState *state);
void GuiHandleUserStart(HWND dialog, GuiState *state);
void GuiHandleUserStop(HWND dialog, GuiState *state);
void GuiHandleUserMoney(HWND dialog, GuiState *state, const std::string &amountText);
void GuiHandleUserCancel(HWND dialog, GuiState *state, const std::string &cardName, const std::string &password);

#endif
