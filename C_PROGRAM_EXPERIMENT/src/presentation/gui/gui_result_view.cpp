/*
 * 文件：gui_result_view.cpp
 * 作用：负责将业务层返回的数据渲染到 Win32 ListView 表格中。
 * 边界：本文件只做展示格式化，不发起业务调用，也不修改业务数据。
 */
#include "gui_main_window_internal.h"

#include "gui_utils.h"

#include <commctrl.h>
#include <string>
#include <vector>

/* 清空 ListView 的全部列定义，供不同结果类型重新建表头。 */
static void ClearListColumns(HWND listHandle)
{
    int column = Header_GetItemCount(ListView_GetHeader(listHandle));
    while (column-- > 0) {
        ListView_DeleteColumn(listHandle, column);
    }
}

/* 清空 ListView 的行和列，恢复为可重新渲染状态。 */
void GuiPrepareList(HWND listHandle)
{
    ListView_DeleteAllItems(listHandle);
    ClearListColumns(listHandle);
}

static void AddColumn(HWND listHandle, int index, int width, const wchar_t *title)
{
    LVCOLUMNW column = {0};
    column.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT;
    column.fmt = LVCFMT_LEFT;
    column.cx = width;
    column.pszText = const_cast<LPWSTR>(title);
    ListView_InsertColumn(listHandle, index, &column);
}

static void AddRow(HWND listHandle, int row, const std::vector<std::wstring> &values)
{
    LVITEMW item = {0};
    if (values.empty()) {
        return;
    }

    item.mask = LVIF_TEXT;
    item.iItem = row;
    item.pszText = const_cast<LPWSTR>(values[0].c_str());
    ListView_InsertItem(listHandle, &item);

    for (size_t i = 1; i < values.size(); ++i) {
        ListView_SetItemText(listHandle, row, static_cast<int>(i), const_cast<LPWSTR>(values[i].c_str()));
    }
}

/* 用单列表格显示提示信息或错误信息。 */
void GuiShowMessageRow(HWND listHandle, const wchar_t *message)
{
    GuiPrepareList(listHandle);
    AddColumn(listHandle, 0, 700, L"提示");
    AddRow(listHandle, 0, {message});
}

/* 显示单张卡的核心状态、余额和使用信息。 */
void GuiShowCard(HWND listHandle, const Card &card)
{
    GuiPrepareList(listHandle);
    AddColumn(listHandle, 0, 150, L"卡号");
    AddColumn(listHandle, 1, 100, L"状态");
    AddColumn(listHandle, 2, 100, L"余额");
    AddColumn(listHandle, 3, 120, L"累计消费");
    AddColumn(listHandle, 4, 100, L"使用次数");
    AddColumn(listHandle, 5, 190, L"最后使用时间");

    AddRow(listHandle, 0, {
        GuiUtf8ToWide(card.aCardName),
        GuiCardStatusText(card.nStatus),
        GuiFormatMoneyFromCent(card.nBalanceCent),
        GuiFormatMoneyFromCent(card.nTotalUseCent),
        std::to_wstring(card.nUseCount),
        GuiFormatTime(card.tLast)
    });
}

/* 显示卡列表，供模糊查询和高级查询复用。 */
void GuiShowCards(HWND listHandle, const Card *cards, size_t count)
{
    GuiPrepareList(listHandle);
    AddColumn(listHandle, 0, 150, L"卡号");
    AddColumn(listHandle, 1, 100, L"状态");
    AddColumn(listHandle, 2, 100, L"余额");
    AddColumn(listHandle, 3, 120, L"累计消费");
    AddColumn(listHandle, 4, 100, L"使用次数");
    AddColumn(listHandle, 5, 190, L"最后使用时间");

    if (cards == nullptr) {
        return;
    }

    for (size_t i = 0; i < count; ++i) {
        AddRow(listHandle, static_cast<int>(i), {
            GuiUtf8ToWide(cards[i].aCardName),
            GuiCardStatusText(cards[i].nStatus),
            GuiFormatMoneyFromCent(cards[i].nBalanceCent),
            GuiFormatMoneyFromCent(cards[i].nTotalUseCent),
            std::to_wstring(cards[i].nUseCount),
            GuiFormatTime(cards[i].tLast)
        });
    }
}

/* 显示下机结算结果。 */
void GuiShowSettle(HWND listHandle, const SettleInfo &info)
{
    GuiPrepareList(listHandle);
    AddColumn(listHandle, 0, 150, L"卡号");
    AddColumn(listHandle, 1, 190, L"上机时间");
    AddColumn(listHandle, 2, 190, L"下机时间");
    AddColumn(listHandle, 3, 100, L"消费金额");
    AddColumn(listHandle, 4, 100, L"余额");
    AddRow(listHandle, 0, {
        GuiUtf8ToWide(info.aCardName),
        GuiFormatTime(info.tStart),
        GuiFormatTime(info.tEnd),
        GuiFormatMoneyFromCent(info.nAmountCent),
        GuiFormatMoneyFromCent(info.nBalanceCent)
    });
}

/* 显示上机成功结果。 */
void GuiShowLogon(HWND listHandle, const LogonInfo &info)
{
    GuiPrepareList(listHandle);
    AddColumn(listHandle, 0, 150, L"卡号");
    AddColumn(listHandle, 1, 100, L"余额");
    AddColumn(listHandle, 2, 190, L"上机时间");
    AddRow(listHandle, 0, {
        GuiUtf8ToWide(info.aCardName),
        GuiFormatMoneyFromCent(info.nBalanceCent),
        GuiFormatTime(info.tStart)
    });
}

/* 显示充值或退费结果，title 决定第二列表头。 */
void GuiShowMoney(HWND listHandle, const Card &card, const Money &money, const wchar_t *title)
{
    GuiPrepareList(listHandle);
    AddColumn(listHandle, 0, 150, L"卡号");
    AddColumn(listHandle, 1, 120, title);
    AddColumn(listHandle, 2, 120, L"当前余额");
    AddRow(listHandle, 0, {
        GuiUtf8ToWide(card.aCardName),
        GuiFormatMoneyFromCent(money.nMoneyCent),
        GuiFormatMoneyFromCent(card.nBalanceCent)
    });
}

/* 显示 YYYY-MM 月营业额统计结果。 */
void GuiShowStatistics(HWND listHandle, const BillingStatistics &statistics)
{
    GuiPrepareList(listHandle);
    AddColumn(listHandle, 0, 160, L"统计月份");
    AddColumn(listHandle, 1, 160, L"月营业额");
    AddRow(listHandle, 0, {
        std::to_wstring(statistics.year) + L"年" + std::to_wstring(statistics.month) + L"月",
        GuiFormatMoneyFromCent(statistics.totalAmountCent)
    });
}

/* 显示 cards.txt 健康检查摘要和前 20 条异常。 */
void GuiShowCardFileHealthResult(HWND listHandle, const CardFileMaintenanceResult &result)
{
    GuiPrepareList(listHandle);
    AddColumn(listHandle, 0, 400, L"健康检查明细");

    std::vector<std::wstring> lines;
    lines.push_back(L"cards.txt 健康检查结果");
    lines.push_back(L"扫描行数：" + std::to_wstring(result.report.totalLines));
    lines.push_back(L"有效记录：" + std::to_wstring(result.report.validLines));
    lines.push_back(L"异常记录：" + std::to_wstring(result.report.invalidLines));
    lines.push_back(L"重复卡号：" + std::to_wstring(result.report.duplicateCards));
    lines.push_back(L"超长记录：" + std::to_wstring(result.report.lineTooLongCount));
    lines.push_back(L"异常记录文件：data/cards_error.txt");

    size_t limit = (result.issueCount > 20) ? 20 : result.issueCount;
    for (size_t i = 0; i < limit; i++) {
        const CardFileIssue &issue = result.issues[i];
        std::wstring line = L"Line " + std::to_wstring(issue.lineNo) + L" | ";
        std::string typeStr = dataGetCardFileIssueTypeName(issue.type);
        line += GuiUtf8ToWide(typeStr.c_str()) + L" | " + GuiUtf8ToWide(issue.reason);
        lines.push_back(line);
    }

    if (result.issueCount > 20) {
        lines.push_back(L"...(更多异常略)");
    }

    for (size_t i = 0; i < lines.size(); i++) {
        AddRow(listHandle, i, {lines[i]});
    }
}


