#include "gui_utils.h"

#include "model.h"

#include <windows.h>

#include <cwchar>

std::wstring GuiUtf8ToWide(const char *text)
{
    int length = 0;
    std::wstring result;

    if (text == nullptr) {
        return L"";
    }

    length = MultiByteToWideChar(CP_UTF8, 0, text, -1, nullptr, 0);
    if (length <= 1) {
        return L"";
    }

    result.resize((size_t)length - 1);
    MultiByteToWideChar(CP_UTF8, 0, text, -1, &result[0], length);
    return result;
}

std::string GuiWideToUtf8(const std::wstring &text)
{
    int length = 0;
    std::string result;

    if (text.empty()) {
        return std::string();
    }

    length = WideCharToMultiByte(CP_UTF8, 0, text.c_str(), -1, nullptr, 0, nullptr, nullptr);
    if (length <= 1) {
        return std::string();
    }

    result.resize((size_t)length - 1);
    WideCharToMultiByte(CP_UTF8, 0, text.c_str(), -1, &result[0], length, nullptr, nullptr);
    return result;
}

std::wstring GuiFormatMoneyFromCent(int32_t amountCent)
{
    int32_t absCent = amountCent < 0 ? -amountCent : amountCent;
    int32_t yuan = absCent / 100;
    int32_t cent = absCent % 100;
    wchar_t buffer[64] = {0};

    if (amountCent < 0) {
        swprintf_s(buffer, L"-%d.%02d", yuan, cent);
    } else {
        swprintf_s(buffer, L"%d.%02d", yuan, cent);
    }

    return buffer;
}

std::wstring GuiFormatTime(time_t value)
{
    struct tm localValue = {0};
    wchar_t buffer[64] = {0};

    if (value == (time_t)0) {
        return L"-";
    }

    if (localtime_s(&localValue, &value) != 0) {
        return L"-";
    }

    if (wcsftime(buffer, sizeof(buffer) / sizeof(buffer[0]), L"%Y-%m-%d %H:%M:%S", &localValue) == 0) {
        return L"-";
    }

    return buffer;
}

std::wstring GuiCardStatusText(int status)
{
    switch (status) {
    case CARD_STATUS_OFFLINE:
        return L"未上机";
    case CARD_STATUS_ONLINE:
        return L"正在上机";
    case CARD_STATUS_CANCELED:
        return L"已注销";
    case CARD_STATUS_INVALID:
        return L"失效";
    default:
        return L"未知";
    }
}

std::wstring GuiBillingStatusText(int status)
{
    if (status == 1) {
        return L"已结算";
    }
    if (status == 0) {
        return L"未结算";
    }
    return L"未知";
}
