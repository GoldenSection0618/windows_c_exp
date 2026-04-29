#ifndef GUI_ADMIN_ACTIONS_H
#define GUI_ADMIN_ACTIONS_H

#include "gui_main_window_internal.h"

#include <windows.h>

#include <string>

void GuiHandleAdminQuery(HWND dialog, GuiState *state, const std::string &cardName);
void GuiHandleAdminFuzzyQuery(HWND dialog, GuiState *state, const std::string &keyword);
void GuiHandleAdminAdvancedQuery(HWND dialog,
                                 GuiState *state,
                                 const std::string &filterText,
                                 const std::string &sortText,
                                 const std::string &limitText);
void GuiHandleAdminStop(HWND dialog, GuiState *state, const std::string &cardName);
void GuiHandleAdminMoney(HWND dialog, GuiState *state, const std::string &cardName, const std::string &amountText);
void GuiHandleAdminStatistics(HWND dialog, GuiState *state, const std::string &yearMonthText);
void GuiHandleAdminFileMaintenance(HWND dialog,
                                   GuiState *state,
                                   const std::string &operationText,
                                   const std::string &backupPathText,
                                   const std::string &confirmText);

#endif
