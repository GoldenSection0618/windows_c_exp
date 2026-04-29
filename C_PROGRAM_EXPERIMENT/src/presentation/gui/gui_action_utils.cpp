#include "gui_action_utils.h"

#include "gui_resource.h"
#include "gui_utils.h"

#include <cstdlib>

std::wstring GuiReadText(HWND dialog, int controlId)
{
    wchar_t buffer[256] = {0};
    GetDlgItemTextW(dialog, controlId, buffer, static_cast<int>(sizeof(buffer) / sizeof(buffer[0])));
    return std::wstring(buffer);
}

void GuiSetStatus(HWND dialog, const std::wstring &text)
{
    SetDlgItemTextW(dialog, IDC_AMS_STATUS_TEXT, text.c_str());
}

std::wstring GuiErrorText(BizResult result)
{
    if (result == BIZ_ERR_WRONG_PASSWORD) {
        return L"账号、卡号或密码错误。";
    }
    if (result == BIZ_ERR_SYSTEM) {
        return L"系统内部错误，或当前登录角色没有权限执行该操作。";
    }
    return GuiUtf8ToWide(bizGetMessage(result));
}

int GuiParseInt(const std::string &text, int defaultValue)
{
    char *endptr = nullptr;
    long value = 0;

    if (text.empty()) {
        return defaultValue;
    }

    value = strtol(text.c_str(), &endptr, 10);
    if (endptr == text.c_str() || *endptr != '\0') {
        return -1;
    }
    return static_cast<int>(value);
}
