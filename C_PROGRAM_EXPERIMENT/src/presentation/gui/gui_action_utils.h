#ifndef GUI_ACTION_UTILS_H
#define GUI_ACTION_UTILS_H

#include "business.h"

#include <windows.h>

#include <string>

std::wstring GuiReadText(HWND dialog, int controlId);
void GuiSetStatus(HWND dialog, const std::wstring &text);
std::wstring GuiErrorText(BizResult result);
int GuiParseInt(const std::string &text, int defaultValue);

#endif
