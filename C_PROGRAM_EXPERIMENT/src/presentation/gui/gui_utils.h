#ifndef GUI_UTILS_H
#define GUI_UTILS_H

#include <stdint.h>
#include <string>
#include <time.h>

std::wstring GuiUtf8ToWide(const char *text);
std::string GuiWideToUtf8(const std::wstring &text);
std::wstring GuiFormatMoneyFromCent(int32_t amountCent);
std::wstring GuiFormatTime(time_t value);
std::wstring GuiCardStatusText(int status);
std::wstring GuiBillingStatusText(int status);

#endif
