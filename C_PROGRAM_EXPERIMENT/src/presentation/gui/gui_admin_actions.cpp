#include "gui_admin_actions.h"

#include "gui_action_utils.h"

#include "card_file_maintenance.h"
#include "card_query.h"
#include "card_validator.h"
#include "gui_utils.h"

#include <ctime>
#include <cstdlib>

static BizResult BuildAdvancedQueryOption(const std::string &filterText,
                                          const std::string &sortText,
                                          const std::string &limitText,
                                          CardQueryOption *option)
{
    int filter = GuiParseInt(filterText, CARD_QUERY_FILTER_ALL);
    int sort = GuiParseInt(sortText, CARD_QUERY_SORT_BALANCE_DESC);
    int32_t limitCent = 0;

    if (option == nullptr) {
        return BIZ_ERR_SYSTEM;
    }
    if (filter < CARD_QUERY_FILTER_ALL || filter > CARD_QUERY_FILTER_LOW_BALANCE ||
        sort < CARD_QUERY_SORT_BALANCE_ASC || sort > CARD_QUERY_SORT_LAST_USE_DESC) {
        return BIZ_ERR_SYSTEM;
    }

    option->filterType = static_cast<CardQueryFilterType>(filter);
    option->sortType = static_cast<CardQuerySortType>(sort);
    option->lowBalanceLimitCent = 0;

    if (option->filterType == CARD_QUERY_FILTER_LOW_BALANCE) {
        if (limitText.empty() || validatorParseMoneyToCent(limitText.c_str(), &limitCent) != MONEY_PARSE_OK || limitCent <= 0) {
            return BIZ_ERR_INVALID_AMOUNT;
        }
        option->lowBalanceLimitCent = limitCent;
    }

    return BIZ_OK;
}

void GuiHandleAdminQuery(HWND dialog, GuiState *state, const std::string &cardName)
{
    Card card = {};
    BizResult result = bizAdminQueryCard(&state->session, cardName.c_str(), &card);
    if (result == BIZ_OK) {
        GuiShowCard(state->listHandle, card);
        GuiSetStatus(dialog, L"查询完成。");
    } else {
        GuiSetStatus(dialog, GuiErrorText(result));
    }
}

void GuiHandleAdminFuzzyQuery(HWND dialog, GuiState *state, const std::string &keyword)
{
    Card *cards = nullptr;
    size_t actualCount = 0;
    size_t requiredCount = 0;

    BizResult result = bizAdminQueryCardsByKeyword(&state->session,
                                                   keyword.c_str(),
                                                   nullptr,
                                                   0,
                                                   &actualCount,
                                                   &requiredCount);
    if (result == BIZ_OK) {
        cards = static_cast<Card *>(malloc(requiredCount * sizeof(Card)));
        if (cards == nullptr) {
            result = BIZ_ERR_NO_MEMORY;
        }
    }
    if (result == BIZ_OK) {
        result = bizAdminQueryCardsByKeyword(&state->session,
                                             keyword.c_str(),
                                             cards,
                                             requiredCount,
                                             &actualCount,
                                             &requiredCount);
    }
    if (result == BIZ_OK) {
        GuiShowCards(state->listHandle, cards, actualCount);
        GuiSetStatus(dialog, L"模糊查询完成。结果数=" + std::to_wstring(actualCount) + L"。");
    } else {
        GuiSetStatus(dialog, GuiErrorText(result));
    }
    if (cards != nullptr) {
        free(cards);
    }
}

void GuiHandleAdminAdvancedQuery(HWND dialog,
                                 GuiState *state,
                                 const std::string &filterText,
                                 const std::string &sortText,
                                 const std::string &limitText)
{
    CardQueryOption option = {};
    Card *cards = nullptr;
    size_t count = 0;
    BizResult result = BuildAdvancedQueryOption(filterText, sortText, limitText, &option);
    if (result == BIZ_OK) {
        result = bizAdminQueryCardsAdvanced(&state->session, &option, &cards, &count);
    }
    if (result == BIZ_OK) {
        GuiShowCards(state->listHandle, cards, count);
        GuiSetStatus(dialog,
            L"高级查询完成。筛选=" + GuiUtf8ToWide(bizGetCardQueryFilterText(option.filterType)) +
            L"；排序=" + GuiUtf8ToWide(bizGetCardQuerySortText(option.sortType)) +
            L"；结果数=" + std::to_wstring(count) + L"。");
    } else {
        GuiSetStatus(dialog, GuiErrorText(result));
    }
    if (cards != nullptr) {
        bizFreeCardQueryResult(cards);
    }
}

void GuiHandleAdminStop(HWND dialog, GuiState *state, const std::string &cardName)
{
    SettleInfo info = {};
    BizResult result = bizAdminStopBilling(&state->session, cardName.c_str(), time(nullptr), &info);
    if (result == BIZ_OK) {
        GuiShowSettle(state->listHandle, info);
        GuiSetStatus(dialog, L"管理员下机成功。");
    } else {
        GuiSetStatus(dialog, GuiErrorText(result));
    }
}

void GuiHandleAdminMoney(HWND dialog, GuiState *state, const std::string &cardName, const std::string &amountText)
{
    Money money = {};
    Card card = {};
    BizResult result = BIZ_OK;
    if (state->mode == GuiMode::AdminRecharge) {
        result = bizAdminRecharge(&state->session, cardName.c_str(), amountText.c_str(), &money, &card);
    } else {
        result = bizAdminRefundByAmount(&state->session, cardName.c_str(), amountText.c_str(), &money, &card);
    }
    if (result == BIZ_OK) {
        GuiShowMoney(state->listHandle, card, money, state->mode == GuiMode::AdminRecharge ? L"充值金额" : L"退费金额");
        GuiSetStatus(dialog, state->mode == GuiMode::AdminRecharge ? L"管理员充值成功。" : L"管理员退费成功。");
    } else {
        GuiSetStatus(dialog, GuiErrorText(result));
    }
}

void GuiHandleAdminStatistics(HWND dialog, GuiState *state, const std::string &yearMonthText)
{
    BillingStatistics statistics = {};
    BizResult result = bizAdminGetBillingStatistics(&state->session, yearMonthText.c_str(), &statistics);
    if (result == BIZ_OK) {
        GuiShowStatistics(state->listHandle, statistics);
        GuiSetStatus(dialog, L"营业额统计完成。");
    } else {
        GuiSetStatus(dialog, GuiErrorText(result));
    }
}

void GuiHandleAdminFileMaintenance(HWND dialog, GuiState *state, const std::string &operationText, const std::string &backupPathText, const std::string &confirmText)
{
    int op = GuiParseInt(operationText, 0);
    if (op < 1 || op > 4) {
        GuiSetStatus(dialog, L"文件维护操作编号无效，请输入 1~4。");
        return;
    }

    CardFileMaintenanceResult result;
    BizResult bizRet;

    if (op == 1) {
        bizRet = bizCheckCardFileHealth(&result);
        if (bizRet == BIZ_OK) {
            GuiShowCardFileHealthResult(state->listHandle, result);
            GuiSetStatus(dialog, L"健康检查完成，异常记录已隔离到 data/cards_error.txt。");
            bizFreeCardFileMaintenanceResult(&result);
        } else {
            GuiSetStatus(dialog, GuiErrorText(bizRet));
        }
    } else if (op == 2) {
        bizRet = bizCheckCardFileHealth(&result);
        if (bizRet == BIZ_OK) {
            BizResult reportRet = bizExportCardFileHealthReport(&result);
            GuiShowCardFileHealthResult(state->listHandle, result);
            if (reportRet == BIZ_OK) {
                GuiSetStatus(dialog, L"健康检查报告已导出到 data/cards_health_report.txt。");
            } else {
                GuiSetStatus(dialog, GuiErrorText(reportRet));
            }
            bizFreeCardFileMaintenanceResult(&result);
        } else {
            GuiSetStatus(dialog, GuiErrorText(bizRet));
        }
    } else if (op == 3) {
        char backupPath[256];
        bizRet = bizBackupCardFile(backupPath, sizeof(backupPath));
        if (bizRet == BIZ_OK) {
            GuiPrepareList(state->listHandle);
            GuiShowMessageRow(state->listHandle, GuiUtf8ToWide(("备份路径：" + std::string(backupPath)).c_str()).c_str());
            GuiSetStatus(dialog, L"备份成功。");
        } else {
            GuiSetStatus(dialog, GuiErrorText(bizRet));
        }
    } else if (op == 4) {
        if (backupPathText.empty()) {
            GuiSetStatus(dialog, L"恢复失败：请填写备份路径。");
            return;
        }
        if (confirmText != "YES") {
            GuiSetStatus(dialog, L"恢复已取消：第三栏必须输入 YES。");
            return;
        }
        bizRet = bizRecoverCardFile(backupPathText.c_str());
        if (bizRet == BIZ_OK) {
            GuiSetStatus(dialog, L"恢复成功。");
        } else {
            GuiSetStatus(dialog, GuiErrorText(bizRet));
        }
    }
}
