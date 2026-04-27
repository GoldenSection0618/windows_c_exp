#include "card_view.h"

#include <stdio.h>
#include <string.h>
#include <time.h>

static const char *cardStatusText(int status)
{
    switch (status) {
    case CARD_STATUS_OFFLINE:
        return "未上机";
    case CARD_STATUS_ONLINE:
        return "正在上机";
    case CARD_STATUS_CANCELED:
        return "已注销";
    case CARD_STATUS_INVALID:
        return "失效";
    default:
        return "未知";
    }
}

static void formatMoney(int32_t cent, char *buffer, size_t size)
{
    int32_t value = cent;
    int32_t yuan = 0;
    int32_t rest = 0;

    if (buffer == NULL || size == 0) {
        return;
    }

    if (value < 0) {
        value = -value;
    }

    yuan = value / 100;
    rest = value % 100;
    if (cent < 0) {
        snprintf(buffer, size, "-%d.%02d", yuan, rest);
    } else {
        snprintf(buffer, size, "%d.%02d", yuan, rest);
    }
}

static void formatTime(time_t value, char *buffer, size_t size)
{
    struct tm *local = NULL;

    if (buffer == NULL || size == 0) {
        return;
    }
    if (value == 0) {
        snprintf(buffer, size, "-");
        return;
    }

    local = localtime(&value);
    if (local == NULL || strftime(buffer, size, "%Y-%m-%d %H:%M:%S", local) == 0) {
        snprintf(buffer, size, "-");
    }
}

static int utf8Width(const char *text)
{
    const unsigned char *p = (const unsigned char *)text;
    int width = 0;

    if (text == NULL) {
        return 0;
    }

    while (*p != '\0') {
        if ((*p & 0x80) == 0) {
            width++;
            p++;
        } else if ((*p & 0xE0) == 0xC0) {
            width += 2;
            p += (p[1] == '\0') ? 1 : 2;
        } else if ((*p & 0xF0) == 0xE0) {
            width += 2;
            p += (p[1] == '\0' || p[2] == '\0') ? 1 : 3;
        } else if ((*p & 0xF8) == 0xF0) {
            width += 2;
            p += (p[1] == '\0' || p[2] == '\0' || p[3] == '\0') ? 1 : 4;
        } else {
            width += 2;
            p++;
        }
    }

    return width;
}

static void printPad(int count)
{
    while (count-- > 0) {
        putchar(' ');
    }
}

static void printLeft(const char *text, int width)
{
    const char *safeText = text == NULL ? "" : text;
    int pad = width - utf8Width(safeText);

    fputs(safeText, stdout);
    if (pad > 0) {
        printPad(pad);
    }
}

static void printRight(const char *text, int width)
{
    const char *safeText = text == NULL ? "" : text;
    int pad = width - (int)strlen(safeText);

    if (pad > 0) {
        printPad(pad);
    }
    fputs(safeText, stdout);
}

static void printRuleCell(int width)
{
    putchar('+');
    while (width-- > 0) {
        putchar('-');
    }
}

static void printRule(void)
{
    printRuleCell(20);
    printRuleCell(10);
    printRuleCell(12);
    printRuleCell(12);
    printRuleCell(10);
    printRuleCell(21);
    printf("+\n");
}

static void printHeader(void)
{
    printRule();
    printf("| "); printLeft("卡号", 18);
    printf(" | "); printLeft("状态", 8);
    printf(" | "); printLeft("余额", 10);
    printf(" | "); printLeft("累计消费", 10);
    printf(" | "); printLeft("次数", 8);
    printf(" | "); printLeft("最后使用时间", 19);
    printf(" |\n");
    printRule();
}

static void printRow(const Card *card)
{
    char balance[32];
    char totalUse[32];
    char useCount[16];
    char lastTime[32];

    formatMoney(card->nBalanceCent, balance, sizeof(balance));
    formatMoney(card->nTotalUseCent, totalUse, sizeof(totalUse));
    snprintf(useCount, sizeof(useCount), "%d", card->nUseCount);
    formatTime(card->tLast, lastTime, sizeof(lastTime));

    printf("| "); printLeft(card->aCardName, 18);
    printf(" | "); printLeft(cardStatusText(card->nStatus), 8);
    printf(" | "); printRight(balance, 10);
    printf(" | "); printRight(totalUse, 10);
    printf(" | "); printRight(useCount, 8);
    printf(" | "); printLeft(lastTime, 19);
    printf(" |\n");
}

void viewShowAdvancedQueryPage(const Card *cards,
                               size_t count,
                               size_t pageIndex,
                               size_t pageSize,
                               const CardQueryOption *option)
{
    size_t totalPages = 0;
    size_t start = 0;
    size_t end = 0;
    size_t i = 0;

    if (cards == NULL || count == 0 || pageSize == 0 || option == NULL) {
        return;
    }

    totalPages = (count + pageSize - 1) / pageSize;
    if (pageIndex >= totalPages) {
        pageIndex = totalPages - 1;
    }

    start = pageIndex * pageSize;
    end = start + pageSize;
    if (end > count) {
        end = count;
    }

    printf("筛选条件：%s；排序方式：%s；共 %zu 条；第 %zu/%zu 页。\n",
           bizGetCardQueryFilterText(option->filterType),
           bizGetCardQuerySortText(option->sortType),
           count,
           pageIndex + 1,
           totalPages);
    printHeader();
    for (i = start; i < end; i++) {
        printRow(&cards[i]);
    }
    printRule();
}
