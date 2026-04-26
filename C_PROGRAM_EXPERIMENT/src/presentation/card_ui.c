#include "card_ui.h"

#include "business.h"
#include "card_view.h"
#include "common.h"
#include "menu.h"

#include <stdlib.h>
#include <stdio.h>
#include <time.h>

static const char *getStartBillingMessage(BizResult result)
{
    switch (result) {
    case BIZ_ERR_CARD_NOT_FOUND:
    case BIZ_ERR_WRONG_PASSWORD:
        return "卡号或密码错误！";
    case BIZ_ERR_CARD_UNAVAILABLE:
        return "该卡正在使用！";
    case BIZ_ERR_CARD_CANCELED_FOR_START:
        return "该卡已注销！";
    case BIZ_ERR_BALANCE_NOT_ENOUGH:
        return "余额不足！";
    default:
        return bizGetMessage(result);
    }
}

static const char *getStopBillingMessage(BizResult result)
{
    switch (result) {
    case BIZ_ERR_CARD_NOT_FOUND:
    case BIZ_ERR_WRONG_PASSWORD:
        return "卡号或密码错误！";
    default:
        return bizGetMessage(result);
    }
}

static const char *getRechargeMessage(BizResult result)
{
    switch (result) {
    case BIZ_ERR_CARD_NOT_FOUND:
    case BIZ_ERR_WRONG_PASSWORD:
        return "卡号或密码错误！";
    default:
        return bizGetMessage(result);
    }
}

static const char *getRefundMessage(BizResult result)
{
    switch (result) {
    case BIZ_ERR_CARD_NOT_FOUND:
    case BIZ_ERR_WRONG_PASSWORD:
        return "卡号或密码错误！";
    case BIZ_ERR_CARD_STATUS_INVALID_FOR_REFUND:
        return "该卡正在上机，不能退费！";
    case BIZ_ERR_CARD_CANCELED_FOR_REFUND:
        return "已注销卡不能退费！";
    case BIZ_ERR_BALANCE_NOT_ENOUGH:
        return "余额不足，不能退费！";
    default:
        return bizGetMessage(result);
    }
}

static const char *getBillingQueryMessage(BizResult result)
{
    return bizGetMessage(result);
}

static const char *getCancelCardMessage(BizResult result)
{
    switch (result) {
    case BIZ_ERR_CARD_NOT_FOUND:
    case BIZ_ERR_WRONG_PASSWORD:
        return "卡号或密码错误！";
    case BIZ_ERR_CARD_STATUS_INVALID_FOR_CANCEL:
        return "该卡正在上机，不能注销！";
    case BIZ_ERR_CARD_CANCELED_FOR_CANCEL:
        return "该卡已注销，不能重复注销！";
    default:
        return bizGetMessage(result);
    }
}

void handleAddCardInteraction(void)
{
    char cardName[INPUT_BUF_SIZE];
    char password[INPUT_BUF_SIZE];
    char amountText[INPUT_BUF_SIZE];
    Card card;
    BizResult result = BIZ_OK;

    if (readTextInput("请输入卡号（1~18位）：", cardName, sizeof(cardName)) != 0) {
        printf("%s\n", bizGetMessage(BIZ_ERR_INVALID_CARD_NAME));
        return;
    }

    if (readTextInput("请输入密码（1~8位）：", password, sizeof(password)) != 0) {
        printf("%s\n", bizGetMessage(BIZ_ERR_INVALID_PASSWORD));
        return;
    }

    if (readTextInput("请输入开卡金额（元）：", amountText, sizeof(amountText)) != 0) {
        printf("%s\n", bizGetMessage(BIZ_ERR_INVALID_AMOUNT));
        return;
    }

    result = bizAddCard(cardName, password, amountText, &card);
    if (result != BIZ_OK) {
        printf("%s\n", bizGetMessage(result));
        return;
    }

    printf("添加卡成功！\n");
    viewShowCardSummary(&card);
}

void handleQueryCardInteraction(void)
{
    char queryText[INPUT_BUF_SIZE];
    Card card;
    Card *cards = NULL;
    size_t matchCount = 0;
    size_t actualCount = 0;
    int queryMode = 0;
    BizResult result = BIZ_OK;

    printf("1. 精确查询\n");
    printf("2. 模糊查询\n");
    if (readChoiceInput("请选择查询方式：", &queryMode) != 0) {
        printf("查询方式输入格式错误，请输入数字编号（1~2）。\n");
        return;
    }

    switch (queryMode) {
    case 1:
        if (readTextInput("请输入卡号（1~18位）：", queryText, sizeof(queryText)) != 0) {
            printf("%s\n", bizGetMessage(BIZ_ERR_INVALID_CARD_NAME));
            return;
        }

        result = bizQueryCard(queryText, &card);
        if (result != BIZ_OK) {
            printf("%s\n", bizGetMessage(result));
            return;
        }

        viewShowQueryCardDetails(&card);
        break;
    case 2:
        if (readTextInput("请输入查询关键字（1~18位）：", queryText, sizeof(queryText)) != 0) {
            printf("%s\n", bizGetMessage(BIZ_ERR_INVALID_CARD_NAME));
            return;
        }

        result = bizQueryCardsByKeyword(queryText, NULL, 0, &actualCount, &matchCount);
        if (result != BIZ_OK) {
            printf("%s\n", bizGetMessage(result));
            return;
        }

        cards = (Card *)malloc(matchCount * sizeof(Card));
        if (cards == NULL) {
            printf("%s\n", bizGetMessage(BIZ_ERR_NO_MEMORY));
            return;
        }

        result = bizQueryCardsByKeyword(queryText, cards, matchCount, &actualCount, &matchCount);
        if (result != BIZ_OK) {
            free(cards);
            printf("%s\n", bizGetMessage(result));
            return;
        }

        viewShowFuzzyQueryResults(queryText, cards, actualCount);
        free(cards);
        break;
    default:
        printf("无效查询方式，请输入 1~2。\n");
        return;
    }
}

void handleStartBillingInteraction(void)
{
    char cardName[INPUT_BUF_SIZE];
    char password[INPUT_BUF_SIZE];
    LogonInfo logonInfo;
    BizResult result = BIZ_OK;
    time_t requestTime = (time_t)0;

    if (readTextInput("请输入卡号（1~18位）：", cardName, sizeof(cardName)) != 0) {
        printf("%s\n", bizGetMessage(BIZ_ERR_INVALID_CARD_NAME));
        return;
    }

    if (readTextInput("请输入密码（1~8位）：", password, sizeof(password)) != 0) {
        printf("%s\n", bizGetMessage(BIZ_ERR_INVALID_PASSWORD));
        return;
    }

    requestTime = time(NULL);
    result = bizStartBilling(cardName, password, requestTime, &logonInfo);
    if (result != BIZ_OK) {
        printf("上机失败！\n");
        printf("%s\n", getStartBillingMessage(result));
        return;
    }

    printf("上机成功！\n");
    viewShowLogonInfo(&logonInfo);
}

void handleStopBillingInteraction(void)
{
    char cardName[INPUT_BUF_SIZE];
    char password[INPUT_BUF_SIZE];
    SettleInfo settleInfo;
    BizResult result = BIZ_OK;
    time_t requestTime = (time_t)0;

    if (readTextInput("请输入卡号（1~18位）：", cardName, sizeof(cardName)) != 0) {
        printf("%s\n", bizGetMessage(BIZ_ERR_INVALID_CARD_NAME));
        return;
    }

    if (readTextInput("请输入密码（1~8位）：", password, sizeof(password)) != 0) {
        printf("%s\n", bizGetMessage(BIZ_ERR_INVALID_PASSWORD));
        return;
    }

    requestTime = time(NULL);
    result = bizStopBilling(cardName, password, requestTime, &settleInfo);
    if (result != BIZ_OK) {
        printf("下机失败！\n");
        printf("%s\n", getStopBillingMessage(result));
        return;
    }

    printf("下机成功！\n");
    viewShowSettleInfo(&settleInfo);
}

void handleRechargeInteraction(void)
{
    char cardName[INPUT_BUF_SIZE];
    char password[INPUT_BUF_SIZE];
    char amountText[INPUT_BUF_SIZE];
    Money rechargeRecord;
    Card updatedCard;
    BizResult result = BIZ_OK;

    if (readTextInput("请输入卡号（1~18位）：", cardName, sizeof(cardName)) != 0) {
        printf("%s\n", bizGetMessage(BIZ_ERR_INVALID_CARD_NAME));
        return;
    }

    if (readTextInput("请输入密码（1~8位）：", password, sizeof(password)) != 0) {
        printf("%s\n", bizGetMessage(BIZ_ERR_INVALID_PASSWORD));
        return;
    }

    if (readTextInput("请输入充值金额（元）：", amountText, sizeof(amountText)) != 0) {
        printf("%s\n", bizGetMessage(BIZ_ERR_INVALID_AMOUNT));
        return;
    }

    result = bizRecharge(cardName, password, amountText, &rechargeRecord, &updatedCard);
    if (result != BIZ_OK) {
        printf("充值失败！\n");
        printf("%s\n", getRechargeMessage(result));
        return;
    }

    printf("充值成功！\n");
    viewShowRechargeInfo(&updatedCard, rechargeRecord.nMoneyCent);
}

void handleRefundInteraction(void)
{
    char cardName[INPUT_BUF_SIZE];
    char password[INPUT_BUF_SIZE];
    Money refundRecord;
    Card updatedCard;
    BizResult result = BIZ_OK;

    if (readTextInput("请输入卡号（1~18位）：", cardName, sizeof(cardName)) != 0) {
        printf("%s\n", bizGetMessage(BIZ_ERR_INVALID_CARD_NAME));
        return;
    }

    if (readTextInput("请输入密码（1~8位）：", password, sizeof(password)) != 0) {
        printf("%s\n", bizGetMessage(BIZ_ERR_INVALID_PASSWORD));
        return;
    }

    result = bizRefund(cardName, password, &refundRecord, &updatedCard);
    if (result != BIZ_OK) {
        printf("退费失败！\n");
        printf("%s\n", getRefundMessage(result));
        return;
    }

    printf("退费成功！\n");
    viewShowRefundInfo(&updatedCard, refundRecord.nMoneyCent);
}

void handleCancelCardInteraction(void)
{
    char cardName[INPUT_BUF_SIZE];
    char password[INPUT_BUF_SIZE];
    Money refundRecord;
    Card updatedCard;
    BizResult result = BIZ_OK;

    if (readTextInput("请输入卡号（1~18位）：", cardName, sizeof(cardName)) != 0) {
        printf("%s\n", bizGetMessage(BIZ_ERR_INVALID_CARD_NAME));
        return;
    }

    if (readTextInput("请输入密码（1~8位）：", password, sizeof(password)) != 0) {
        printf("%s\n", bizGetMessage(BIZ_ERR_INVALID_PASSWORD));
        return;
    }

    result = bizCancelCard(cardName, password, &refundRecord, &updatedCard);
    if (result != BIZ_OK) {
        printf("注销卡失败！\n");
        printf("%s\n", getCancelCardMessage(result));
        return;
    }

    printf("注销卡成功！\n");
    viewShowCancelCardInfo(&updatedCard, refundRecord.nMoneyCent);
}

void handleBillingQueryInteraction(void)
{
    char cardName[INPUT_BUF_SIZE];
    char startText[INPUT_BUF_SIZE];
    char endText[INPUT_BUF_SIZE];
    BillingQueryResult resultSet;
    int featureChoice = 0;
    int queryMode = 0;
    BizResult result = BIZ_OK;

    resultSet.items = NULL;
    resultSet.count = 0;

    printf("1. 按卡号查询消费记录\n");
    if (readChoiceInput("请选择统计功能：", &featureChoice) != 0) {
        printf("统计功能输入格式错误，请输入数字编号（1）。\n");
        return;
    }
    if (featureChoice != 1) {
        printf("无效统计功能编号，请输入 1。\n");
        return;
    }

    if (readTextInput("请输入卡号（1~18位）：", cardName, sizeof(cardName)) != 0) {
        printf("%s\n", bizGetMessage(BIZ_ERR_INVALID_CARD_NAME));
        return;
    }

    printf("1. 查询全部\n");
    printf("2. 限定时间段\n");
    if (readChoiceInput("请选择查询方式：", &queryMode) != 0) {
        printf("查询方式输入格式错误，请输入数字编号（1~2）。\n");
        return;
    }

    switch (queryMode) {
    case 1:
        result = bizQueryBillingsByCardName(cardName, &resultSet);
        break;
    case 2:
        if (readTextInput("请输入开始时间（YYYY-MM-DD HH:MM:SS）：", startText, sizeof(startText)) != 0) {
            printf("%s\n", bizGetMessage(BIZ_ERR_INVALID_TIME_RANGE));
            return;
        }
        if (readTextInput("请输入结束时间（YYYY-MM-DD HH:MM:SS）：", endText, sizeof(endText)) != 0) {
            printf("%s\n", bizGetMessage(BIZ_ERR_INVALID_TIME_RANGE));
            return;
        }
        result = bizQueryBillingsByCardNameAndRange(cardName, startText, endText, &resultSet);
        break;
    default:
        printf("无效查询方式，请输入 1~2。\n");
        return;
    }

    if (result != BIZ_OK) {
        printf("%s\n", getBillingQueryMessage(result));
        return;
    }

    viewShowBillingRecords(resultSet.items, resultSet.count);
    bizFreeBillingQueryResult(&resultSet);
}
