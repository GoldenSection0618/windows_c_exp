#ifndef GUI_AUTH_ACTIONS_H
#define GUI_AUTH_ACTIONS_H

#include "gui_main_window_internal.h"

#include <windows.h>

#include <string>

void GuiHandleAdminLogin(HWND dialog, GuiState *state, const std::string &account, const std::string &password);
void GuiHandleUserLogin(HWND dialog, GuiState *state, const std::string &cardName, const std::string &password);
void GuiHandleRegister(HWND dialog, GuiState *state, const std::string &cardName, const std::string &password);

#endif
