#include "gui_main_window_internal.h"

#include "gui_utils.h"

#include <commctrl.h>
#include <string>
#include <vector>

static void ClearListColumns(HWND listHandle)
{
    int column = Header_GetItemCount(ListView_GetHeader(listHandle));
    while (column-- > 0) {
        ListView_DeleteColumn(listHandle, column);
    }
}


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


void GuiShowMessageRow(HWND listHandle, const wchar_t *message)
{
    GuiPrepareList(listHandle);
    AddColumn(listHandle, 0, 700, L"提示");
    AddRow(listHandle, 0, {message});
}


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


void GuiShowStatistics(HWND listHandle, const BillingStatistics &statistics)
{
    GuiPrepareList(listHandle);
    AddColumn(listHandle, 0, 160, L"项目");
    AddColumn(listHandle, 1, 160, L"金额");
    AddRow(listHandle, 0, {L"总营业额", GuiFormatMoneyFromCent(statistics.totalAmountCent)});
    for (int i = 0; i < 12; ++i) {
        AddRow(listHandle, i + 1, {
            std::to_wstring(statistics.year) + L"年" + std::to_wstring(i + 1) + L"月",
            GuiFormatMoneyFromCent(statistics.monthlyAmountCent[i])
        });
    }
}


