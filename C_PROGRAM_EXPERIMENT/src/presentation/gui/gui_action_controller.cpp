#include "gui_main_window_internal.h"

#include "gui_action_utils.h"
#include "gui_auth_actions.h"
#include "gui_admin_actions.h"
#include "gui_user_actions.h"
#include "gui_resource.h"
#include "gui_utils.h"

#include <string>

void GuiExecuteSubmit(HWND dialog, GuiState *state)
{
    std::string text1 = GuiWideToUtf8(GuiReadText(dialog, IDC_AMS_CARD_NAME));
    std::string text2 = GuiWideToUtf8(GuiReadText(dialog, IDC_AMS_CARD_PASSWORD));
    std::string text3 = GuiWideToUtf8(GuiReadText(dialog, IDC_AMS_CARD_MONEY));

    switch (state->mode) {
    case GuiMode::AuthAdmin:
        GuiHandleAdminLogin(dialog, state, text1, text2);
        break;
    case GuiMode::AuthUserLogin:
        GuiHandleUserLogin(dialog, state, text1, text2);
        break;
    case GuiMode::AuthRegister:
        GuiHandleRegister(dialog, state, text1, text2);
        break;
    case GuiMode::AdminQuery:
        GuiHandleAdminQuery(dialog, state, text1);
        break;
    case GuiMode::AdminFuzzyQuery:
        GuiHandleAdminFuzzyQuery(dialog, state, text1);
        break;
    case GuiMode::AdminAdvancedQuery:
        GuiHandleAdminAdvancedQuery(dialog, state, text1, text2, text3);
        break;
    case GuiMode::AdminStop:
        GuiHandleAdminStop(dialog, state, text1);
        break;
    case GuiMode::AdminRecharge:
    case GuiMode::AdminRefund:
        GuiHandleAdminMoney(dialog, state, text1, text3);
        break;
    case GuiMode::AdminStatistics:
        GuiHandleAdminStatistics(dialog, state, text1);
        break;
    case GuiMode::AdminFileMaintenance:
        GuiHandleAdminFileMaintenance(dialog, state, text1, text2, text3);
        break;
    case GuiMode::UserBalance:
        GuiHandleUserBalance(dialog, state);
        break;
    case GuiMode::UserStart:
        GuiHandleUserStart(dialog, state);
        break;
    case GuiMode::UserStop:
        GuiHandleUserStop(dialog, state);
        break;
    case GuiMode::UserRecharge:
    case GuiMode::UserRefund:
        GuiHandleUserMoney(dialog, state, text1);
        break;
    case GuiMode::UserCancel:
        GuiHandleUserCancel(dialog, state, text1, text2);
        break;
    }
}
