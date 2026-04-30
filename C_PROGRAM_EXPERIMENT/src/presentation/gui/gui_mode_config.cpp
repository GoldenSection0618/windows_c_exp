/*
 * 文件：gui_mode_config.cpp
 * 作用：集中配置不同 GuiMode 下的导航按钮、输入框、提交按钮和状态提示。
 * 边界：本文件只描述界面状态，不调用业务层 API，也不处理提交事件。
 */
#include "gui_main_window_internal.h"

#include "gui_resource.h"
#include "gui_utils.h"

#include <string>

static void SetText(HWND dialog, int controlId, const wchar_t *text)
{
    SetDlgItemTextW(dialog, controlId, text == nullptr ? L"" : text);
}

static void SetStatus(HWND dialog, const std::wstring &text)
{
    SetText(dialog, IDC_AMS_STATUS_TEXT, text.c_str());
}

static void SetControlVisible(HWND dialog, int controlId, bool visible)
{
    HWND control = GetDlgItem(dialog, controlId);
    if (control != nullptr) {
        ShowWindow(control, visible ? SW_SHOW : SW_HIDE);
    }
}

static void SetPasswordMask(HWND dialog, bool enabled)
{
    HWND edit = GetDlgItem(dialog, IDC_AMS_CARD_PASSWORD);
    SendMessageW(edit, EM_SETPASSWORDCHAR, enabled ? L'*' : 0, 0);
    InvalidateRect(edit, nullptr, TRUE);
}

static void ClearInputs(HWND dialog)
{
    SetText(dialog, IDC_AMS_CARD_NAME, L"");
    SetText(dialog, IDC_AMS_CARD_PASSWORD, L"");
    SetText(dialog, IDC_AMS_CARD_MONEY, L"");
}

static void SetNavButton(HWND dialog, int controlId, const wchar_t *text, bool visible)
{
    SetText(dialog, controlId, text);
    SetControlVisible(dialog, controlId, visible);
}

/* 单个导航按钮的显示配置。 */
struct NavButtonConfig {
    int controlId;
    const wchar_t *text;
    bool visible;
};

/* 三个通用输入框的标签、显隐和密码掩码配置。 */
struct InputConfig {
    const wchar_t *label1;
    bool showInput1;
    const wchar_t *label2;
    bool showInput2;
    const wchar_t *label3;
    bool showInput3;
    bool passwordMask;
};

/* GuiMode 的完整界面配置，用于避免 handler 中散落控件状态逻辑。 */
struct ModeConfig {
    GuiMode mode;
    const wchar_t *windowTitle;
    const NavButtonConfig *navButtons;
    InputConfig input;
    const wchar_t *submitText;
    const wchar_t *statusText;
    bool appendCurrentCard;
};

static const NavButtonConfig kAuthNav[] = {
    {IDC_AMS_NAV_ADD_CARD, L"管理员登录", true},
    {IDC_AMS_NAV_QUERY_CARD, L"用户登录", true},
    {IDC_AMS_NAV_LOGON, L"用户注册", true},
    {IDC_AMS_NAV_SETTLE, L"退出", true},
    {IDC_AMS_NAV_RECHARGE, L"", false},
    {IDC_AMS_NAV_REFUND, L"", false},
    {IDC_AMS_NAV_CANCEL_CARD, L"", false},
    {IDC_AMS_NAV_BILLING, L"", false},
    {IDC_AMS_NAV_STAT, L"", false},
    {IDC_AMS_NAV_EXIT, L"", false},
    {0, nullptr, false},
};

static const NavButtonConfig kAdminNav[] = {
    {IDC_AMS_NAV_ADD_CARD, L"查询卡", true},
    {IDC_AMS_NAV_QUERY_CARD, L"模糊查询", true},
    {IDC_AMS_NAV_LOGON, L"高级查询", true},
    {IDC_AMS_NAV_SETTLE, L"下机", true},
    {IDC_AMS_NAV_RECHARGE, L"充值", true},
    {IDC_AMS_NAV_REFUND, L"退费", true},
    {IDC_AMS_NAV_CANCEL_CARD, L"营业额统计", true},
    {IDC_AMS_NAV_BILLING, L"文件维护", true},
    {IDC_AMS_NAV_STAT, L"退出登录", true},
    {IDC_AMS_NAV_EXIT, L"", false},
    {0, nullptr, false},
};

static const NavButtonConfig kUserNav[] = {
    {IDC_AMS_NAV_ADD_CARD, L"查余额", true},
    {IDC_AMS_NAV_QUERY_CARD, L"上机", true},
    {IDC_AMS_NAV_LOGON, L"下机", true},
    {IDC_AMS_NAV_SETTLE, L"充值", true},
    {IDC_AMS_NAV_RECHARGE, L"退费", true},
    {IDC_AMS_NAV_REFUND, L"注销卡", true},
    {IDC_AMS_NAV_CANCEL_CARD, L"退出登录", true},
    {IDC_AMS_NAV_BILLING, L"", false},
    {IDC_AMS_NAV_STAT, L"", false},
    {IDC_AMS_NAV_EXIT, L"", false},
    {0, nullptr, false},
};

/* 每个 GuiMode 对应一组界面配置，切换模式时统一应用。 */
static const ModeConfig kModeConfigs[] = {
    {GuiMode::AuthAdmin, L"计费管理系统 - 登录", kAuthNav, {L"管理员账号", true, L"管理员密码", true, L"", false, true}, L"登录", L"", false},
    {GuiMode::AuthUserLogin, L"计费管理系统 - 登录", kAuthNav, {L"卡号", true, L"密码", true, L"", false, true}, L"登录", L"用户登录：输入卡号和密码。", false},
    {GuiMode::AuthRegister, L"计费管理系统 - 登录", kAuthNav, {L"注册卡号", true, L"注册密码", true, L"", false, true}, L"注册", L"用户注册：默认赠送 100 元新手礼包。", false},
    {GuiMode::AdminQuery, L"计费管理系统 - 管理员后台", kAdminNav, {L"卡号", true, L"", false, L"", false, false}, L"查询", L"管理员：查询任意卡，不需要卡密码。", false},
    {GuiMode::AdminFuzzyQuery, L"计费管理系统 - 管理员后台", kAdminNav, {L"卡号关键字", true, L"", false, L"", false, false}, L"模糊查询", L"管理员：输入卡号关键字，查询所有包含该关键字的卡。", false},
    {GuiMode::AdminAdvancedQuery, L"计费管理系统 - 管理员后台", kAdminNav, {L"筛选编号(1~5)", true, L"排序编号(1~5)", true, L"低余额阈值(元)", true, false}, L"高级查询", L"筛选：1全部/2未上机/3上机/4注销/5低余额；排序：1余额升/2余额降/3次数降/4消费降/5最后使用降。", false},
    {GuiMode::AdminStop, L"计费管理系统 - 管理员后台", kAdminNav, {L"卡号", true, L"", false, L"", false, false}, L"下机", L"管理员：下机只需要卡号。", false},
    {GuiMode::AdminRecharge, L"计费管理系统 - 管理员后台", kAdminNav, {L"卡号", true, L"", false, L"充值金额(元)", true, false}, L"充值", L"管理员资金操作：只需要卡号和金额，不需要卡密码。", false},
    {GuiMode::AdminRefund, L"计费管理系统 - 管理员后台", kAdminNav, {L"卡号", true, L"", false, L"退费金额(元)", true, false}, L"退费", L"管理员资金操作：只需要卡号和金额，不需要卡密码。", false},
    {GuiMode::AdminStatistics, L"计费管理系统 - 管理员后台", kAdminNav, {L"年月(YYYY-MM)", true, L"", false, L"", false, false}, L"统计", L"管理员：按月统计营业额，例如 2026-04。", false},
    {GuiMode::AdminFileMaintenance, L"计费管理系统 - 管理员后台", kAdminNav, {L"操作编号(1~4)", true, L"备份路径", true, L"确认恢复(YES)", true, false}, L"执行维护", L"操作：1健康检查 / 2导出报告 / 3手动备份 / 4恢复备份；恢复时需填写备份路径并在第三栏输入 YES。", false},
    {GuiMode::UserBalance, L"计费管理系统 - 用户中心", kUserNav, {L"", false, L"", false, L"", false, false}, L"查询余额", L"用户：当前登录卡号 ", true},
    {GuiMode::UserStart, L"计费管理系统 - 用户中心", kUserNav, {L"", false, L"", false, L"", false, false}, L"上机", L"用户：当前登录卡号 ", true},
    {GuiMode::UserStop, L"计费管理系统 - 用户中心", kUserNav, {L"", false, L"", false, L"", false, false}, L"下机", L"用户：当前登录卡号 ", true},
    {GuiMode::UserRecharge, L"计费管理系统 - 用户中心", kUserNav, {L"充值金额(元)", true, L"", false, L"", false, false}, L"充值", L"用户资金操作：默认作用于当前卡。", false},
    {GuiMode::UserRefund, L"计费管理系统 - 用户中心", kUserNav, {L"退费金额(元)", true, L"", false, L"", false, false}, L"退费", L"用户资金操作：默认作用于当前卡。", false},
    {GuiMode::UserCancel, L"计费管理系统 - 用户中心", kUserNav, {L"确认卡号", true, L"确认密码", true, L"", false, true}, L"注销", L"注销卡需要再次输入卡号和密码确认。", false},
};

static const ModeConfig *FindModeConfig(GuiMode mode)
{
    for (const ModeConfig &config : kModeConfigs) {
        if (config.mode == mode) {
            return &config;
        }
    }
    return nullptr;
}

static void ApplyNavButtons(HWND dialog, const NavButtonConfig *navButtons)
{
    if (navButtons == nullptr) {
        return;
    }
    for (const NavButtonConfig *button = navButtons; button->controlId != 0; ++button) {
        SetNavButton(dialog, button->controlId, button->text, button->visible);
    }
}

/* 设置三个通用输入框的标签、显隐和密码掩码。 */
static void ConfigureInput(HWND dialog,
                           const wchar_t *label1,
                           bool showInput1,
                           const wchar_t *label2,
                           bool showInput2,
                           const wchar_t *label3,
                           bool showInput3,
                           bool passwordMask)
{
    SetText(dialog, IDC_AMS_LABEL_1, label1);
    SetText(dialog, IDC_AMS_LABEL_2, label2);
    SetText(dialog, IDC_AMS_LABEL_3, label3);

    SetControlVisible(dialog, IDC_AMS_LABEL_1, showInput1);
    SetControlVisible(dialog, IDC_AMS_CARD_NAME, showInput1);
    SetControlVisible(dialog, IDC_AMS_LABEL_2, showInput2);
    SetControlVisible(dialog, IDC_AMS_CARD_PASSWORD, showInput2);
    SetControlVisible(dialog, IDC_AMS_LABEL_3, showInput3);
    SetControlVisible(dialog, IDC_AMS_CARD_MONEY, showInput3);
    SetPasswordMask(dialog, passwordMask);
}

/*
 * 功能：切换 GUI 当前模式并应用对应界面配置。
 * 边界：只更新控件状态和提示文本，不执行任何业务操作。
 */
void GuiSwitchMode(HWND dialog, GuiState *state, GuiMode mode)
{
    const ModeConfig *config = FindModeConfig(mode);
    if (config == nullptr) {
        return;
    }

    state->mode = mode;
    ClearInputs(dialog);
    GuiPrepareList(state->listHandle);
    SetControlVisible(dialog, IDC_AMS_SUBMIT, true);

    SetWindowTextW(dialog, config->windowTitle);
    ApplyNavButtons(dialog, config->navButtons);
    ConfigureInput(dialog,
                   config->input.label1,
                   config->input.showInput1,
                   config->input.label2,
                   config->input.showInput2,
                   config->input.label3,
                   config->input.showInput3,
                   config->input.passwordMask);
    SetText(dialog, IDC_AMS_SUBMIT, config->submitText);
    if (config->appendCurrentCard) {
        SetStatus(dialog, std::wstring(config->statusText) + GuiUtf8ToWide(state->session.cardName) + L"，默认作用于当前卡。");
    } else {
        SetStatus(dialog, config->statusText);
    }
}
