#include "card_view.h"

#include <stdio.h>
#include <string.h>
#include <time.h>

static const char *cardStatusToText(int status)
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

static const char *billingStatusToText(int status)
{
    switch (status) {
    case 0:
        return "未结算";
    case 1:
        return "已结算";
    default:
        return "未知";
    }
}

static void formatMoneyFromCent(int32_t amountCent, char *buffer, size_t size);

static void formatTimeString(time_t ts, char *buffer, size_t size)
{
    struct tm *local = NULL;

    if (buffer == NULL || size == 0) {
        return;
    }

    if (ts == 0) {
        snprintf(buffer, size, "-");
        return;
    }

    local = localtime(&ts);
    if (local == NULL) {
        snprintf(buffer, size, "-");
        return;
    }

    if (strftime(buffer, size, "%Y-%m-%d %H:%M:%S", local) == 0) {
        snprintf(buffer, size, "-");
    }
}

static int utf8DisplayWidth(const char *text)
{
    const unsigned char *p = (const unsigned char *)text;
    int width = 0;

    if (text == NULL) {
        return 0;
    }

    while (*p != '\0') {
        if ((*p & 0x80) == 0) {
            width += 1;
            p++;
            continue;
        }

        if ((*p & 0xE0) == 0xC0) {
            p += (p[1] == '\0') ? 1 : 2;
        } else if ((*p & 0xF0) == 0xE0) {
            p += (p[1] == '\0' || p[2] == '\0') ? 1 : 3;
        } else if ((*p & 0xF8) == 0xF0) {
            p += (p[1] == '\0' || p[2] == '\0' || p[3] == '\0') ? 1 : 4;
        } else {
            p += 1;
        }
        width += 2;
    }

    return width;
}

static void printSpaces(int count)
{
    int i = 0;

    for (i = 0; i < count; i++) {
        putchar(' ');
    }
}

static void printCellLeftUtf8(const char *text, int width)
{
    int pad = 0;
    const char *safeText = text == NULL ? "" : text;

    fputs(safeText, stdout);
    pad = width - utf8DisplayWidth(safeText);
    if (pad > 0) {
        printSpaces(pad);
    }
}

static void printCellRightAscii(const char *text, int width)
{
    int pad = 0;
    const char *safeText = text == NULL ? "" : text;

    pad = width - (int)strlen(safeText);
    if (pad > 0) {
        printSpaces(pad);
    }
    fputs(safeText, stdout);
}

static void printCellRule(int width)
{
    putchar('+');
    while (width-- > 0) {
        putchar('-');
    }
}

static void printQueryTableHeader(int cardColWidth,
                                  int statusColWidth,
                                  int balanceColWidth,
                                  int totalUseColWidth,
                                  int useCountColWidth,
                                  int lastUseTimeColWidth)
{
    printCellRule(cardColWidth + 2);
    printCellRule(statusColWidth + 2);
    printCellRule(balanceColWidth + 2);
    printCellRule(totalUseColWidth + 2);
    printCellRule(useCountColWidth + 2);
    printCellRule(lastUseTimeColWidth + 2);
    printf("+\n");

    printf("| ");
    printCellLeftUtf8("卡号", cardColWidth);
    printf(" | ");
    printCellLeftUtf8("卡状态", statusColWidth);
    printf(" | ");
    printCellLeftUtf8("余额", balanceColWidth);
    printf(" | ");
    printCellLeftUtf8("累计使用", totalUseColWidth);
    printf(" | ");
    printCellLeftUtf8("使用次数", useCountColWidth);
    printf(" | ");
    printCellLeftUtf8("最后使用时间", lastUseTimeColWidth);
    printf(" |\n");

    printCellRule(cardColWidth + 2);
    printCellRule(statusColWidth + 2);
    printCellRule(balanceColWidth + 2);
    printCellRule(totalUseColWidth + 2);
    printCellRule(useCountColWidth + 2);
    printCellRule(lastUseTimeColWidth + 2);
    printf("+\n");
}

static void printQueryTableRow(const Card *card,
                               int cardColWidth,
                               int statusColWidth,
                               int balanceColWidth,
                               int totalUseColWidth,
                               int useCountColWidth,
                               int lastUseTimeColWidth)
{
    char lastUseBuf[32];
    char balanceBuf[32];
    char totalUseBuf[32];
    char useCountBuf[16];

    formatTimeString(card->tLast, lastUseBuf, sizeof(lastUseBuf));
    formatMoneyFromCent(card->nBalanceCent, balanceBuf, sizeof(balanceBuf));
    formatMoneyFromCent(card->nTotalUseCent, totalUseBuf, sizeof(totalUseBuf));
    snprintf(useCountBuf, sizeof(useCountBuf), "%d", card->nUseCount);

    printf("| ");
    printCellLeftUtf8(card->aCardName, cardColWidth);
    printf(" | ");
    printCellLeftUtf8(cardStatusToText(card->nStatus), statusColWidth);
    printf(" | ");
    printCellRightAscii(balanceBuf, balanceColWidth);
    printf(" | ");
    printCellRightAscii(totalUseBuf, totalUseColWidth);
    printf(" | ");
    printCellRightAscii(useCountBuf, useCountColWidth);
    printf(" | ");
    printCellLeftUtf8(lastUseBuf, lastUseTimeColWidth);
    printf(" |\n");
}

static void printQueryTableFooter(int cardColWidth,
                                  int statusColWidth,
                                  int balanceColWidth,
                                  int totalUseColWidth,
                                  int useCountColWidth,
                                  int lastUseTimeColWidth)
{
    printCellRule(cardColWidth + 2);
    printCellRule(statusColWidth + 2);
    printCellRule(balanceColWidth + 2);
    printCellRule(totalUseColWidth + 2);
    printCellRule(useCountColWidth + 2);
    printCellRule(lastUseTimeColWidth + 2);
    printf("+\n");
}

static void formatMoneyFromCent(int32_t amountCent, char *buffer, size_t size)
{
    int32_t absCent = amountCent;
    int32_t yuan = 0;
    int32_t cent = 0;

    if (buffer == NULL || size == 0) {
        return;
    }

    if (amountCent < 0) {
        absCent = -amountCent;
    }

    yuan = absCent / 100;
    cent = absCent % 100;

    if (amountCent < 0) {
        snprintf(buffer, size, "-%d.%02d", yuan, cent);
    } else {
        snprintf(buffer, size, "%d.%02d", yuan, cent);
    }
}

void viewShowCardSummary(const Card *card)
{
    const int cardColWidth = 18;
    const int pwdColWidth = 8;
    const int statusColWidth = 8;
    const int balanceColWidth = 10;
    char balanceBuf[32];

    if (card == NULL) {
        return;
    }

    formatMoneyFromCent(card->nBalanceCent, balanceBuf, sizeof(balanceBuf));

    printCellRule(cardColWidth + 2);
    printCellRule(pwdColWidth + 2);
    printCellRule(statusColWidth + 2);
    printCellRule(balanceColWidth + 2);
    printf("+\n");

    printf("| ");
    printCellLeftUtf8("卡号", cardColWidth);
    printf(" | ");
    printCellLeftUtf8("密码", pwdColWidth);
    printf(" | ");
    printCellLeftUtf8("状态", statusColWidth);
    printf(" | ");
    printCellLeftUtf8("余额", balanceColWidth);
    printf(" |\n");

    printCellRule(cardColWidth + 2);
    printCellRule(pwdColWidth + 2);
    printCellRule(statusColWidth + 2);
    printCellRule(balanceColWidth + 2);
    printf("+\n");

    printf("| ");
    printCellLeftUtf8(card->aCardName, cardColWidth);
    printf(" | ");
    printCellLeftUtf8(card->aPwd, pwdColWidth);
    printf(" | ");
    printCellLeftUtf8(cardStatusToText(card->nStatus), statusColWidth);
    printf(" | ");
    printCellRightAscii(balanceBuf, balanceColWidth);
    printf(" |\n");

    printCellRule(cardColWidth + 2);
    printCellRule(pwdColWidth + 2);
    printCellRule(statusColWidth + 2);
    printCellRule(balanceColWidth + 2);
    printf("+\n");
}

void viewShowQueryCardDetails(const Card *card)
{
    const int cardColWidth = 18;
    const int statusColWidth = 8;
    const int balanceColWidth = 10;
    const int totalUseColWidth = 10;
    const int useCountColWidth = 8;
    const int lastUseTimeColWidth = 19;
    if (card == NULL) {
        return;
    }

    printf("查询结果：\n");
    printQueryTableHeader(cardColWidth,
                          statusColWidth,
                          balanceColWidth,
                          totalUseColWidth,
                          useCountColWidth,
                          lastUseTimeColWidth);
    printQueryTableRow(card,
                       cardColWidth,
                       statusColWidth,
                       balanceColWidth,
                       totalUseColWidth,
                       useCountColWidth,
                       lastUseTimeColWidth);
    printQueryTableFooter(cardColWidth,
                          statusColWidth,
                          balanceColWidth,
                          totalUseColWidth,
                          useCountColWidth,
                          lastUseTimeColWidth);
}

void viewShowFuzzyQueryResults(const char *keyword, const Card *cards, size_t count)
{
    const int cardColWidth = 18;
    const int statusColWidth = 8;
    const int balanceColWidth = 10;
    const int totalUseColWidth = 10;
    const int useCountColWidth = 8;
    const int lastUseTimeColWidth = 19;
    size_t index = 0;

    if (keyword == NULL || cards == NULL || count == 0) {
        return;
    }

    printf("查询关键字：%s\n", keyword);
    printf("匹配结果数：%zu\n", count);
    printQueryTableHeader(cardColWidth,
                          statusColWidth,
                          balanceColWidth,
                          totalUseColWidth,
                          useCountColWidth,
                          lastUseTimeColWidth);

    for (index = 0; index < count; index++) {
        printQueryTableRow(&cards[index],
                           cardColWidth,
                           statusColWidth,
                           balanceColWidth,
                           totalUseColWidth,
                           useCountColWidth,
                           lastUseTimeColWidth);
    }

    printQueryTableFooter(cardColWidth,
                          statusColWidth,
                          balanceColWidth,
                          totalUseColWidth,
                          useCountColWidth,
                          lastUseTimeColWidth);
}

void viewShowLogonInfo(const LogonInfo *logonInfo)
{
    const int cardColWidth = 18;
    const int balanceColWidth = 10;
    const int startTimeColWidth = 19;
    char balanceBuf[32];
    char startTimeBuf[32];

    if (logonInfo == NULL) {
        return;
    }

    formatMoneyFromCent(logonInfo->nBalanceCent, balanceBuf, sizeof(balanceBuf));
    formatTimeString(logonInfo->tStart, startTimeBuf, sizeof(startTimeBuf));

    printCellRule(cardColWidth + 2);
    printCellRule(balanceColWidth + 2);
    printCellRule(startTimeColWidth + 2);
    printf("+\n");

    printf("| ");
    printCellLeftUtf8("卡号", cardColWidth);
    printf(" | ");
    printCellLeftUtf8("余额", balanceColWidth);
    printf(" | ");
    printCellLeftUtf8("上机时间", startTimeColWidth);
    printf(" |\n");

    printCellRule(cardColWidth + 2);
    printCellRule(balanceColWidth + 2);
    printCellRule(startTimeColWidth + 2);
    printf("+\n");

    printf("| ");
    printCellLeftUtf8(logonInfo->aCardName, cardColWidth);
    printf(" | ");
    printCellRightAscii(balanceBuf, balanceColWidth);
    printf(" | ");
    printCellLeftUtf8(startTimeBuf, startTimeColWidth);
    printf(" |\n");

    printCellRule(cardColWidth + 2);
    printCellRule(balanceColWidth + 2);
    printCellRule(startTimeColWidth + 2);
    printf("+\n");
}

void viewShowSettleInfo(const SettleInfo *settleInfo)
{
    const int cardColWidth = 18;
    const int startTimeColWidth = 19;
    const int endTimeColWidth = 19;
    const int amountColWidth = 10;
    const int balanceColWidth = 10;
    char startTimeBuf[32];
    char endTimeBuf[32];
    char amountBuf[32];
    char balanceBuf[32];

    if (settleInfo == NULL) {
        return;
    }

    formatTimeString(settleInfo->tStart, startTimeBuf, sizeof(startTimeBuf));
    formatTimeString(settleInfo->tEnd, endTimeBuf, sizeof(endTimeBuf));
    formatMoneyFromCent(settleInfo->nAmountCent, amountBuf, sizeof(amountBuf));
    formatMoneyFromCent(settleInfo->nBalanceCent, balanceBuf, sizeof(balanceBuf));

    printCellRule(cardColWidth + 2);
    printCellRule(startTimeColWidth + 2);
    printCellRule(endTimeColWidth + 2);
    printCellRule(amountColWidth + 2);
    printCellRule(balanceColWidth + 2);
    printf("+\n");

    printf("| ");
    printCellLeftUtf8("卡号", cardColWidth);
    printf(" | ");
    printCellLeftUtf8("上机时间", startTimeColWidth);
    printf(" | ");
    printCellLeftUtf8("下机时间", endTimeColWidth);
    printf(" | ");
    printCellLeftUtf8("消费金额", amountColWidth);
    printf(" | ");
    printCellLeftUtf8("结算后余额", balanceColWidth);
    printf(" |\n");

    printCellRule(cardColWidth + 2);
    printCellRule(startTimeColWidth + 2);
    printCellRule(endTimeColWidth + 2);
    printCellRule(amountColWidth + 2);
    printCellRule(balanceColWidth + 2);
    printf("+\n");

    printf("| ");
    printCellLeftUtf8(settleInfo->aCardName, cardColWidth);
    printf(" | ");
    printCellLeftUtf8(startTimeBuf, startTimeColWidth);
    printf(" | ");
    printCellLeftUtf8(endTimeBuf, endTimeColWidth);
    printf(" | ");
    printCellRightAscii(amountBuf, amountColWidth);
    printf(" | ");
    printCellRightAscii(balanceBuf, balanceColWidth);
    printf(" |\n");

    printCellRule(cardColWidth + 2);
    printCellRule(startTimeColWidth + 2);
    printCellRule(endTimeColWidth + 2);
    printCellRule(amountColWidth + 2);
    printCellRule(balanceColWidth + 2);
    printf("+\n");
}

void viewShowRechargeInfo(const Card *card, int32_t rechargeAmountCent)
{
    const int cardColWidth = 18;
    const int amountColWidth = 10;
    const int balanceColWidth = 10;
    char amountBuf[32];
    char balanceBuf[32];

    if (card == NULL) {
        return;
    }

    formatMoneyFromCent(rechargeAmountCent, amountBuf, sizeof(amountBuf));
    formatMoneyFromCent(card->nBalanceCent, balanceBuf, sizeof(balanceBuf));

    printCellRule(cardColWidth + 2);
    printCellRule(amountColWidth + 2);
    printCellRule(balanceColWidth + 2);
    printf("+\n");

    printf("| ");
    printCellLeftUtf8("卡号", cardColWidth);
    printf(" | ");
    printCellLeftUtf8("充值金额", amountColWidth);
    printf(" | ");
    printCellLeftUtf8("当前余额", balanceColWidth);
    printf(" |\n");

    printCellRule(cardColWidth + 2);
    printCellRule(amountColWidth + 2);
    printCellRule(balanceColWidth + 2);
    printf("+\n");

    printf("| ");
    printCellLeftUtf8(card->aCardName, cardColWidth);
    printf(" | ");
    printCellRightAscii(amountBuf, amountColWidth);
    printf(" | ");
    printCellRightAscii(balanceBuf, balanceColWidth);
    printf(" |\n");

    printCellRule(cardColWidth + 2);
    printCellRule(amountColWidth + 2);
    printCellRule(balanceColWidth + 2);
    printf("+\n");
}

void viewShowRefundInfo(const Card *card, int32_t refundAmountCent)
{
    const int cardColWidth = 18;
    const int amountColWidth = 10;
    const int balanceColWidth = 10;
    char amountBuf[32];
    char balanceBuf[32];

    if (card == NULL) {
        return;
    }

    formatMoneyFromCent(refundAmountCent, amountBuf, sizeof(amountBuf));
    formatMoneyFromCent(card->nBalanceCent, balanceBuf, sizeof(balanceBuf));

    printCellRule(cardColWidth + 2);
    printCellRule(amountColWidth + 2);
    printCellRule(balanceColWidth + 2);
    printf("+\n");

    printf("| ");
    printCellLeftUtf8("卡号", cardColWidth);
    printf(" | ");
    printCellLeftUtf8("退费金额", amountColWidth);
    printf(" | ");
    printCellLeftUtf8("当前余额", balanceColWidth);
    printf(" |\n");

    printCellRule(cardColWidth + 2);
    printCellRule(amountColWidth + 2);
    printCellRule(balanceColWidth + 2);
    printf("+\n");

    printf("| ");
    printCellLeftUtf8(card->aCardName, cardColWidth);
    printf(" | ");
    printCellRightAscii(amountBuf, amountColWidth);
    printf(" | ");
    printCellRightAscii(balanceBuf, balanceColWidth);
    printf(" |\n");

    printCellRule(cardColWidth + 2);
    printCellRule(amountColWidth + 2);
    printCellRule(balanceColWidth + 2);
    printf("+\n");
}

void viewShowCancelCardInfo(const Card *card, int32_t refundAmountCent)
{
    const int cardColWidth = 18;
    const int amountColWidth = 10;
    char amountBuf[32];

    if (card == NULL) {
        return;
    }

    formatMoneyFromCent(refundAmountCent, amountBuf, sizeof(amountBuf));

    printCellRule(cardColWidth + 2);
    printCellRule(amountColWidth + 2);
    printf("+\n");

    printf("| ");
    printCellLeftUtf8("卡号", cardColWidth);
    printf(" | ");
    printCellLeftUtf8("退款金额", amountColWidth);
    printf(" |\n");

    printCellRule(cardColWidth + 2);
    printCellRule(amountColWidth + 2);
    printf("+\n");

    printf("| ");
    printCellLeftUtf8(card->aCardName, cardColWidth);
    printf(" | ");
    printCellRightAscii(amountBuf, amountColWidth);
    printf(" |\n");

    printCellRule(cardColWidth + 2);
    printCellRule(amountColWidth + 2);
    printf("+\n");
}

void viewShowBillingRecords(const Billing *items, size_t count)
{
    const int cardColWidth = 18;
    const int startTimeColWidth = 19;
    const int endTimeColWidth = 19;
    const int amountColWidth = 10;
    const int statusColWidth = 8;
    size_t index = 0;

    if (items == NULL || count == 0) {
        return;
    }

    printf("查询结果：\n");
    printCellRule(cardColWidth + 2);
    printCellRule(startTimeColWidth + 2);
    printCellRule(endTimeColWidth + 2);
    printCellRule(amountColWidth + 2);
    printCellRule(statusColWidth + 2);
    printf("+\n");

    printf("| ");
    printCellLeftUtf8("卡号", cardColWidth);
    printf(" | ");
    printCellLeftUtf8("上机时间", startTimeColWidth);
    printf(" | ");
    printCellLeftUtf8("下机时间", endTimeColWidth);
    printf(" | ");
    printCellLeftUtf8("消费金额", amountColWidth);
    printf(" | ");
    printCellLeftUtf8("结算状态", statusColWidth);
    printf(" |\n");

    printCellRule(cardColWidth + 2);
    printCellRule(startTimeColWidth + 2);
    printCellRule(endTimeColWidth + 2);
    printCellRule(amountColWidth + 2);
    printCellRule(statusColWidth + 2);
    printf("+\n");

    for (index = 0; index < count; index++) {
        char startTimeBuf[32];
        char endTimeBuf[32];
        char amountBuf[32];

        formatTimeString(items[index].tStart, startTimeBuf, sizeof(startTimeBuf));
        formatTimeString(items[index].tEnd, endTimeBuf, sizeof(endTimeBuf));
        if (items[index].tEnd == (time_t)0) {
            snprintf(endTimeBuf, sizeof(endTimeBuf), "--");
        }
        formatMoneyFromCent(items[index].nAmountCent, amountBuf, sizeof(amountBuf));

        printf("| ");
        printCellLeftUtf8(items[index].aCardName, cardColWidth);
        printf(" | ");
        printCellLeftUtf8(startTimeBuf, startTimeColWidth);
        printf(" | ");
        printCellLeftUtf8(endTimeBuf, endTimeColWidth);
        printf(" | ");
        printCellRightAscii(amountBuf, amountColWidth);
        printf(" | ");
        printCellLeftUtf8(billingStatusToText(items[index].nStatus), statusColWidth);
        printf(" |\n");
    }

    printCellRule(cardColWidth + 2);
    printCellRule(startTimeColWidth + 2);
    printCellRule(endTimeColWidth + 2);
    printCellRule(amountColWidth + 2);
    printCellRule(statusColWidth + 2);
    printf("+\n");
}
