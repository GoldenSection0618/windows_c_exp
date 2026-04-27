#include "business.h"
#include "billing_query_repository.h"
#include "billing_repository.h"
#include "billing_rule.h"
#include "billing_storage_file.h"
#include "card_repository.h"
#include "card_storage_file.h"
#include "card_validator.h"
#include "common.h"
#include "money_storage_file.h"
#include "operation_log.h"
#include "time_validator.h"

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <limits.h>
#include <string.h>
#include <time.h>

static BizResult mapDataResult(DataResult result)
{
    switch (result) {
    case DATA_OK:
        return BIZ_OK;
    case DATA_ERR_DUPLICATE:
        return BIZ_ERR_DUPLICATE_CARD;
    case DATA_ERR_NO_MEMORY:
        return BIZ_ERR_NO_MEMORY;
    case DATA_ERR_FILE_OPEN:
        return BIZ_ERR_FILE_OPEN;
    case DATA_ERR_FILE_NOT_FOUND:
        return BIZ_ERR_FILE_NOT_FOUND;
    case DATA_ERR_RECORD_FORMAT:
        return BIZ_ERR_RECORD_FORMAT;
    case DATA_ERR_TIME_PARSE:
        return BIZ_ERR_TIME_PARSE;
    default:
        return BIZ_ERR_SYSTEM;
    }
}

static BizResult prepareFuzzyQueryKeyword(const char *keywordInput,
                                          char *keyword,
                                          size_t keywordSize)
{
    int readResult = 0;

    if (validatorNormalizeInput(keywordInput, keyword, keywordSize) != 0 ||
        !validatorIsValidCardName(keyword)) {
        return BIZ_ERR_INVALID_CARD_NAME;
    }

    readResult = dataLoadCards();
    if (readResult < 0) {
        return mapDataResult((DataResult)readResult);
    }

    return BIZ_OK;
}

static int isStartBillingAllowedStatus(int status)
{
    return status == CARD_STATUS_OFFLINE;
}

static int isStopBillingAllowedStatus(int status)
{
    return status == CARD_STATUS_ONLINE;
}

static int isRefundAllowedStatus(int status)
{
    return status == CARD_STATUS_OFFLINE;
}

static int isCancelAllowedStatus(int status)
{
    return status == CARD_STATUS_OFFLINE;
}

static BizResult parseStatisticsYear(const char *yearInput, int *year)
{
    char yearText[INPUT_BUF_SIZE];
    char *endptr = NULL;
    long parsedYear = 0;

    if (year == NULL) {
        return BIZ_ERR_SYSTEM;
    }

    if (validatorNormalizeInput(yearInput, yearText, sizeof(yearText)) != 0 || yearText[0] == '\0') {
        return BIZ_ERR_INVALID_TIME_RANGE;
    }

    errno = 0;
    parsedYear = strtol(yearText, &endptr, 10);
    if (errno != 0 || endptr == yearText || *endptr != '\0') {
        return BIZ_ERR_INVALID_TIME_RANGE;
    }

    if (parsedYear < 1970 || parsedYear > 9999) {
        return BIZ_ERR_INVALID_TIME_RANGE;
    }

    *year = (int)parsedYear;
    return BIZ_OK;
}

static void clearSession(LoginSession *session)
{
    if (session == NULL) {
        return;
    }

    memset(session, 0, sizeof(*session));
    session->role = LOGIN_ROLE_NONE;
    session->loggedIn = 0;
}

static int isAdminSessionValid(const LoginSession *session)
{
    return session != NULL && session->loggedIn != 0 && session->role == LOGIN_ROLE_ADMIN;
}

static int isUserSessionValid(const LoginSession *session)
{
    return session != NULL && session->loggedIn != 0 && session->role == LOGIN_ROLE_USER;
}

static BizResult getCardPasswordByName(const char *cardNameInput, char *passwordBuffer, size_t passwordBufferSize)
{
    Card card;
    BizResult result = BIZ_OK;

    if (passwordBuffer == NULL || passwordBufferSize == 0) {
        return BIZ_ERR_SYSTEM;
    }

    result = bizQueryCard(cardNameInput, &card);
    if (result != BIZ_OK) {
        return result;
    }

    if (snprintf(passwordBuffer, passwordBufferSize, "%s", card.aPwd) < 0) {
        return BIZ_ERR_SYSTEM;
    }

    return BIZ_OK;
}

void bizInitSession(LoginSession *session)
{
    clearSession(session);
}

void bizLogout(LoginSession *session)
{
    clearSession(session);
}

int bizIsAdminSession(const LoginSession *session)
{
    return isAdminSessionValid(session);
}

int bizIsUserSession(const LoginSession *session)
{
    return isUserSessionValid(session);
}

BizResult bizAdminLogin(const char *accountInput, const char *passwordInput, LoginSession *session)
{
    char account[INPUT_BUF_SIZE];
    char password[INPUT_BUF_SIZE];

    if (session == NULL) {
        return BIZ_ERR_SYSTEM;
    }

    if (validatorNormalizeInput(accountInput, account, sizeof(account)) != 0 ||
        validatorNormalizeInput(passwordInput, password, sizeof(password)) != 0) {
        return BIZ_ERR_WRONG_PASSWORD;
    }

    if (strcmp(account, "root") != 0 || strcmp(password, "root") != 0) {
        return BIZ_ERR_WRONG_PASSWORD;
    }

    clearSession(session);
    session->role = LOGIN_ROLE_ADMIN;
    session->loggedIn = 1;
    return BIZ_OK;
}

BizResult bizUserRegister(const char *cardNameInput, const char *passwordInput, Card *createdCard)
{
    return bizAddCard(cardNameInput, passwordInput, "100", createdCard);
}

BizResult bizUserLogin(const char *cardNameInput, const char *passwordInput, LoginSession *session)
{
    char password[INPUT_BUF_SIZE];
    Card card;
    BizResult result = BIZ_OK;

    if (session == NULL) {
        return BIZ_ERR_SYSTEM;
    }

    if (validatorNormalizeInput(passwordInput, password, sizeof(password)) != 0 ||
        !validatorIsValidPassword(password)) {
        return BIZ_ERR_WRONG_PASSWORD;
    }

    result = bizQueryCard(cardNameInput, &card);
    if (result != BIZ_OK) {
        return result;
    }

    if (strcmp(card.aPwd, password) != 0) {
        return BIZ_ERR_WRONG_PASSWORD;
    }

    clearSession(session);
    session->role = LOGIN_ROLE_USER;
    session->loggedIn = 1;
    snprintf(session->cardName, sizeof(session->cardName), "%s", card.aCardName);
    snprintf(session->password, sizeof(session->password), "%s", card.aPwd);
    return BIZ_OK;
}

BizResult bizAdminQueryCard(const LoginSession *session, const char *cardNameInput, Card *queriedCard)
{
    if (!isAdminSessionValid(session)) {
        return BIZ_ERR_SYSTEM;
    }

    return bizQueryCard(cardNameInput, queriedCard);
}

BizResult bizAdminStopBilling(const LoginSession *session,
                              const char *cardNameInput,
                              time_t requestTime,
                              SettleInfo *settleInfo)
{
    char password[INPUT_BUF_SIZE];
    BizResult result = BIZ_OK;

    if (!isAdminSessionValid(session)) {
        return BIZ_ERR_SYSTEM;
    }

    result = getCardPasswordByName(cardNameInput, password, sizeof(password));
    if (result != BIZ_OK) {
        return result;
    }

    return bizStopBilling(cardNameInput, password, requestTime, settleInfo);
}

BizResult bizAdminRecharge(const LoginSession *session,
                           const char *cardNameInput,
                           const char *amountInput,
                           Money *rechargeRecord,
                           Card *updatedCard)
{
    char password[INPUT_BUF_SIZE];
    BizResult result = BIZ_OK;

    if (!isAdminSessionValid(session)) {
        return BIZ_ERR_SYSTEM;
    }

    result = getCardPasswordByName(cardNameInput, password, sizeof(password));
    if (result != BIZ_OK) {
        return result;
    }

    return bizRecharge(cardNameInput, password, amountInput, rechargeRecord, updatedCard);
}

BizResult bizAdminRefundByAmount(const LoginSession *session,
                                 const char *cardNameInput,
                                 const char *amountInput,
                                 Money *refundRecord,
                                 Card *updatedCard)
{
    char password[INPUT_BUF_SIZE];
    BizResult result = BIZ_OK;

    if (!isAdminSessionValid(session)) {
        return BIZ_ERR_SYSTEM;
    }

    result = getCardPasswordByName(cardNameInput, password, sizeof(password));
    if (result != BIZ_OK) {
        return result;
    }

    return bizRefundByAmount(cardNameInput, password, amountInput, refundRecord, updatedCard);
}

BizResult bizAdminGetBillingStatistics(const LoginSession *session,
                                       const char *yearInput,
                                       BillingStatistics *statistics)
{
    if (!isAdminSessionValid(session)) {
        return BIZ_ERR_SYSTEM;
    }

    return bizGetBillingStatistics(yearInput, statistics);
}

BizResult bizUserQueryBalance(const LoginSession *session, Card *queriedCard)
{
    if (!isUserSessionValid(session)) {
        return BIZ_ERR_SYSTEM;
    }

    return bizQueryCard(session->cardName, queriedCard);
}

BizResult bizUserStartBilling(const LoginSession *session, time_t requestTime, LogonInfo *logonInfo)
{
    if (!isUserSessionValid(session)) {
        return BIZ_ERR_SYSTEM;
    }

    return bizStartBilling(session->cardName, session->password, requestTime, logonInfo);
}

BizResult bizUserStopBilling(const LoginSession *session, time_t requestTime, SettleInfo *settleInfo)
{
    if (!isUserSessionValid(session)) {
        return BIZ_ERR_SYSTEM;
    }

    return bizStopBilling(session->cardName, session->password, requestTime, settleInfo);
}

BizResult bizUserRecharge(const LoginSession *session,
                          const char *amountInput,
                          Money *rechargeRecord,
                          Card *updatedCard)
{
    if (!isUserSessionValid(session)) {
        return BIZ_ERR_SYSTEM;
    }

    return bizRecharge(session->cardName, session->password, amountInput, rechargeRecord, updatedCard);
}

BizResult bizUserRefundByAmount(const LoginSession *session,
                                const char *amountInput,
                                Money *refundRecord,
                                Card *updatedCard)
{
    if (!isUserSessionValid(session)) {
        return BIZ_ERR_SYSTEM;
    }

    return bizRefundByAmount(session->cardName, session->password, amountInput, refundRecord, updatedCard);
}

BizResult bizUserCancelCardWithPassword(const LoginSession *session,
                                        const char *cardNameInput,
                                        const char *passwordInput,
                                        Money *refundRecord,
                                        Card *updatedCard)
{
    char cardName[INPUT_BUF_SIZE];

    if (!isUserSessionValid(session)) {
        return BIZ_ERR_SYSTEM;
    }

    if (validatorNormalizeInput(cardNameInput, cardName, sizeof(cardName)) != 0 ||
        strcmp(cardName, session->cardName) != 0) {
        return BIZ_ERR_WRONG_PASSWORD;
    }

    return bizCancelCard(cardNameInput, passwordInput, refundRecord, updatedCard);
}

BizResult bizAddCard(const char *cardNameInput, const char *passwordInput, const char *amountInput, Card *createdCard)
{
    char cardName[INPUT_BUF_SIZE];
    char password[INPUT_BUF_SIZE];
    char amountText[INPUT_BUF_SIZE];
    int32_t amountCent = 0;
    MoneyParseResult moneyParseResult = MONEY_PARSE_OK;
    Card card;
    time_t now = 0;
    int readResult = 0;
    DataResult dataResult = DATA_OK;

    if (validatorNormalizeInput(cardNameInput, cardName, sizeof(cardName)) != 0 ||
        !validatorIsValidCardName(cardName)) {
        return BIZ_ERR_INVALID_CARD_NAME;
    }

    if (validatorNormalizeInput(passwordInput, password, sizeof(password)) != 0 ||
        !validatorIsValidPassword(password)) {
        return BIZ_ERR_INVALID_PASSWORD;
    }

    if (validatorNormalizeInput(amountInput, amountText, sizeof(amountText)) != 0) {
        return BIZ_ERR_INVALID_AMOUNT;
    }

    moneyParseResult = validatorParseMoneyToCent(amountText, &amountCent);
    if (moneyParseResult == MONEY_PARSE_INVALID) {
        return BIZ_ERR_INVALID_AMOUNT;
    }
    if (moneyParseResult == MONEY_PARSE_TOO_LARGE) {
        return BIZ_ERR_BALANCE_TOO_LARGE;
    }

    readResult = dataLoadCards();
    if (readResult < 0 && readResult != DATA_ERR_FILE_NOT_FOUND) {
        return mapDataResult((DataResult)readResult);
    }

    if (dataCardExists(cardName)) {
        return BIZ_ERR_DUPLICATE_CARD;
    }

    memset(&card, 0, sizeof(card));
    memcpy(card.aCardName, cardName, strlen(cardName) + 1);
    memcpy(card.aPwd, password, strlen(password) + 1);
    card.nStatus = CARD_STATUS_OFFLINE;
    now = time(NULL);
    card.tStart = now;
    card.tEnd = now + (time_t)(365 * 24 * 60 * 60);
    card.tLast = now;
    card.nTotalUseCent = 0;
    card.nUseCount = 0;
    card.nBalanceCent = amountCent;
    card.nDel = 0;

    dataResult = (DataResult)dataAddCard(&card);
    if (dataResult != DATA_OK) {
        return mapDataResult(dataResult);
    }

    dataResult = dataSaveCard(&card);
    if (dataResult != DATA_OK) {
        (void)dataLoadCards();
        return mapDataResult(dataResult);
    }

    logOperation("添加卡");
    if (createdCard != NULL) {
        *createdCard = card;
    }
    return BIZ_OK;
}

BizResult bizQueryCard(const char *cardNameInput, Card *queriedCard)
{
    char cardName[INPUT_BUF_SIZE];
    const Card *card = NULL;
    int readResult = 0;

    if (validatorNormalizeInput(cardNameInput, cardName, sizeof(cardName)) != 0 ||
        !validatorIsValidCardName(cardName)) {
        return BIZ_ERR_INVALID_CARD_NAME;
    }

    readResult = dataLoadCards();
    if (readResult < 0) {
        return mapDataResult((DataResult)readResult);
    }

    card = dataQueryCardByName(cardName);
    if (card == NULL) {
        return BIZ_ERR_CARD_NOT_FOUND;
    }

    logOperation("查询卡");
    if (queriedCard != NULL) {
        *queriedCard = *card;
    }
    return BIZ_OK;
}

BizResult bizQueryCardsByKeyword(const char *keywordInput,
                                 Card *buffer,
                                 size_t capacity,
                                 size_t *actualCount,
                                 size_t *requiredCount)
{
    char keyword[INPUT_BUF_SIZE];
    size_t copied = 0;
    BizResult result = BIZ_OK;

    if (actualCount == NULL || requiredCount == NULL) {
        return BIZ_ERR_SYSTEM;
    }

    *actualCount = 0;
    *requiredCount = 0;

    result = prepareFuzzyQueryKeyword(keywordInput, keyword, sizeof(keyword));
    if (result != BIZ_OK) {
        return result;
    }

    if (dataQueryCardsByKeyword(keyword, buffer, capacity, &copied, requiredCount) != DATA_OK) {
        return BIZ_ERR_SYSTEM;
    }
    if (*requiredCount == 0) {
        return BIZ_ERR_NO_MATCHED_CARD;
    }

    if (buffer == NULL || capacity == 0) {
        return BIZ_OK;
    }

    if (copied != *requiredCount) {
        return BIZ_ERR_SYSTEM;
    }

    logOperation("模糊查询");
    *actualCount = copied;
    return BIZ_OK;
}

const char *bizGetMessage(BizResult result)
{
    switch (result) {
    case BIZ_OK:
        return "操作成功。";
    case BIZ_ERR_INVALID_CARD_NAME:
        return "卡号输入不合法，应为1~18位，且只能包含大小写字母、数字和 _ @ # $ % !。";
    case BIZ_ERR_INVALID_PASSWORD:
        return "密码输入不合法，应为1~8位，且只能包含大小写字母、数字和 _ @ # $ % !。";
    case BIZ_ERR_INVALID_AMOUNT:
        return "开卡金额输入不合法，应为非负金额，且最多保留两位小数。";
    case BIZ_ERR_BALANCE_TOO_LARGE:
        return "余额过大，卡内余额必须小于1000000元。";
    case BIZ_ERR_DUPLICATE_CARD:
        return "卡号已存在，不能重复添加！";
    case BIZ_ERR_CARD_NOT_FOUND:
        return "没有该卡的信息！";
    case BIZ_ERR_NO_MATCHED_CARD:
        return "没有符合关键字的卡信息！";
    case BIZ_ERR_WRONG_PASSWORD:
        return "密码错误！";
    case BIZ_ERR_CARD_UNAVAILABLE:
        return "该卡正在使用，不能上机！";
    case BIZ_ERR_CARD_CANCELED_FOR_START:
        return "该卡已注销，不能上机！";
    case BIZ_ERR_BALANCE_NOT_ENOUGH:
        return "卡号余额不足！";
    case BIZ_ERR_NO_UNSETTLED_BILLING:
        return "未找到该卡的未结算消费记录！";
    case BIZ_ERR_CARD_STATUS_INVALID_FOR_STOP:
        return "该卡当前不在上机状态，不能下机！";
    case BIZ_ERR_CARD_CANCELED_FOR_RECHARGE:
        return "已注销卡不能充值！";
    case BIZ_ERR_CARD_CANCELED_FOR_REFUND:
        return "已注销卡不能退费！";
    case BIZ_ERR_CARD_STATUS_INVALID_FOR_REFUND:
        return "该卡正在上机，不能退费！";
    case BIZ_ERR_CARD_CANCELED_FOR_CANCEL:
        return "该卡已注销，不能重复注销！";
    case BIZ_ERR_CARD_STATUS_INVALID_FOR_CANCEL:
        return "该卡正在上机，不能注销！";
    case BIZ_ERR_INVALID_TIME_RANGE:
        return "时间范围输入不合法！";
    case BIZ_ERR_BILLING_RECORD_NOT_FOUND:
        return "没有找到符合条件的消费记录！";
    case BIZ_ERR_FILE_OPEN:
        return "数据文件异常：卡信息文件打开失败。";
    case BIZ_ERR_FILE_NOT_FOUND:
        return "数据文件异常：卡信息文件不存在。";
    case BIZ_ERR_RECORD_FORMAT:
        return "数据文件内容异常：卡信息文件记录格式错误。";
    case BIZ_ERR_TIME_PARSE:
        return "数据文件内容异常：卡信息文件时间字段解析失败。";
    case BIZ_ERR_NO_MEMORY:
        return "系统内存不足，无法继续操作。";
    default:
        return "系统内部错误。";
    }
}

BizResult bizStartBilling(const char *cardNameInput,
                          const char *passwordInput,
                          time_t requestTime,
                          LogonInfo *logonInfo)
{
    char cardName[INPUT_BUF_SIZE];
    char password[INPUT_BUF_SIZE];
    int cardLoadResult = 0;
    int billingLoadResult = 0;
    const Card *card = NULL;
    Card updatedCard;
    Card originalCard;
    Billing billing;
    DataResult dataResult = DATA_OK;
    time_t now = 0;

    if (validatorNormalizeInput(cardNameInput, cardName, sizeof(cardName)) != 0 ||
        !validatorIsValidCardName(cardName)) {
        return BIZ_ERR_INVALID_CARD_NAME;
    }

    if (validatorNormalizeInput(passwordInput, password, sizeof(password)) != 0 ||
        !validatorIsValidPassword(password)) {
        return BIZ_ERR_INVALID_PASSWORD;
    }

    cardLoadResult = dataLoadCards();
    if (cardLoadResult < 0) {
        return mapDataResult((DataResult)cardLoadResult);
    }

    billingLoadResult = dataLoadBillings();
    if (billingLoadResult < 0 && billingLoadResult != DATA_ERR_FILE_NOT_FOUND) {
        return mapDataResult((DataResult)billingLoadResult);
    }

    card = dataQueryCardByName(cardName);
    if (card == NULL) {
        return BIZ_ERR_CARD_NOT_FOUND;
    }
    if (card->nDel != 0) {
        return BIZ_ERR_CARD_NOT_FOUND;
    }
    if (strcmp(card->aPwd, password) != 0) {
        return BIZ_ERR_WRONG_PASSWORD;
    }
    if (card->nStatus == CARD_STATUS_CANCELED) {
        return BIZ_ERR_CARD_CANCELED_FOR_START;
    }
    if (!isStartBillingAllowedStatus(card->nStatus)) {
        return BIZ_ERR_CARD_UNAVAILABLE;
    }
    if (card->nBalanceCent < 0) {
        return BIZ_ERR_BALANCE_NOT_ENOUGH;
    }

    originalCard = *card;
    updatedCard = *card;
    if (requestTime == (time_t)0) {
        return BIZ_ERR_SYSTEM;
    }
    now = requestTime;
    updatedCard.nStatus = CARD_STATUS_ONLINE;
    updatedCard.tLast = now;

    dataResult = dataUpdateCard(&updatedCard);
    if (dataResult != DATA_OK) {
        return mapDataResult(dataResult);
    }

    memset(&billing, 0, sizeof(billing));
    memcpy(billing.aCardName, updatedCard.aCardName, strlen(updatedCard.aCardName) + 1);
    billing.tStart = now;
    billing.tEnd = (time_t)0;
    billing.nAmountCent = 0;
    billing.nStatus = 0;
    billing.nDel = 0;

    dataResult = dataSaveBilling(&billing);
    if (dataResult != DATA_OK) {
        if (dataUpdateCard(&originalCard) != DATA_OK) {
            return BIZ_ERR_SYSTEM;
        }
        return mapDataResult(dataResult);
    }

    if (logonInfo != NULL) {
        memset(logonInfo, 0, sizeof(*logonInfo));
        memcpy(logonInfo->aCardName, updatedCard.aCardName, strlen(updatedCard.aCardName) + 1);
        logonInfo->tStart = now;
        logonInfo->nStatus = CARD_STATUS_ONLINE;
        logonInfo->nBalanceCent = updatedCard.nBalanceCent;
    }

    logOperation("上机");
    return BIZ_OK;
}

BizResult bizStopBilling(const char *cardNameInput,
                         const char *passwordInput,
                         time_t requestTime,
                         SettleInfo *settleInfo)
{
    char cardName[INPUT_BUF_SIZE];
    char password[INPUT_BUF_SIZE];
    int cardLoadResult = 0;
    int billingLoadResult = 0;
    const Card *card = NULL;
    const Billing *billing = NULL;
    Card originalCard;
    Card updatedCard;
    Billing updatedBilling;
    Rate rate;
    time_t now = 0;
    int durationMinutes = 0;
    int32_t amountCent = 0;
    DataResult dataResult = DATA_OK;

    if (validatorNormalizeInput(cardNameInput, cardName, sizeof(cardName)) != 0 ||
        !validatorIsValidCardName(cardName)) {
        return BIZ_ERR_INVALID_CARD_NAME;
    }

    if (validatorNormalizeInput(passwordInput, password, sizeof(password)) != 0 ||
        !validatorIsValidPassword(password)) {
        return BIZ_ERR_INVALID_PASSWORD;
    }

    cardLoadResult = dataLoadCards();
    if (cardLoadResult < 0) {
        return mapDataResult((DataResult)cardLoadResult);
    }

    billingLoadResult = dataLoadBillings();
    if (billingLoadResult < 0) {
        if (billingLoadResult == DATA_ERR_FILE_NOT_FOUND) {
            return BIZ_ERR_NO_UNSETTLED_BILLING;
        }
        return mapDataResult((DataResult)billingLoadResult);
    }

    card = dataQueryCardByName(cardName);
    if (card == NULL) {
        return BIZ_ERR_CARD_NOT_FOUND;
    }
    if (card->nDel != 0) {
        return BIZ_ERR_CARD_NOT_FOUND;
    }
    if (strcmp(card->aPwd, password) != 0) {
        return BIZ_ERR_WRONG_PASSWORD;
    }
    if (!isStopBillingAllowedStatus(card->nStatus)) {
        return BIZ_ERR_CARD_STATUS_INVALID_FOR_STOP;
    }

    billing = dataQueryLatestUnsettledBillingByCardName(cardName);
    if (billing == NULL) {
        return BIZ_ERR_NO_UNSETTLED_BILLING;
    }

    if (requestTime == (time_t)0) {
        return BIZ_ERR_SYSTEM;
    }
    now = requestTime;
    rate = billingRuleGetDefaultRate();
    if (billingRuleCalculateAmount(billing->tStart, now, &rate, &durationMinutes, &amountCent) != 0) {
        return BIZ_ERR_SYSTEM;
    }

    originalCard = *card;
    updatedCard = *card;
    updatedCard.nStatus = CARD_STATUS_OFFLINE;
    updatedCard.nBalanceCent -= amountCent;
    updatedCard.nTotalUseCent += amountCent;
    updatedCard.nUseCount += 1;
    updatedCard.tLast = now;

    updatedBilling = *billing;
    updatedBilling.tEnd = now;
    updatedBilling.nAmountCent = amountCent;
    updatedBilling.nStatus = 1;

    dataResult = dataUpdateCard(&updatedCard);
    if (dataResult != DATA_OK) {
        return mapDataResult(dataResult);
    }

    dataResult = dataUpdateBilling(&updatedBilling);
    if (dataResult != DATA_OK) {
        if (dataUpdateCard(&originalCard) != DATA_OK) {
            return BIZ_ERR_SYSTEM;
        }
        return mapDataResult(dataResult);
    }

    if (settleInfo != NULL) {
        memset(settleInfo, 0, sizeof(*settleInfo));
        memcpy(settleInfo->aCardName, updatedCard.aCardName, strlen(updatedCard.aCardName) + 1);
        settleInfo->tStart = updatedBilling.tStart;
        settleInfo->tEnd = updatedBilling.tEnd;
        settleInfo->nAmountCent = updatedBilling.nAmountCent;
        settleInfo->nBalanceCent = updatedCard.nBalanceCent;
    }

    (void)durationMinutes;
    logOperation("下机");
    return BIZ_OK;
}

BizResult bizRecharge(const char *cardNameInput,
                      const char *passwordInput,
                      const char *amountInput,
                      Money *rechargeRecord,
                      Card *updatedCard)
{
    char cardName[INPUT_BUF_SIZE];
    char password[INPUT_BUF_SIZE];
    char amountText[INPUT_BUF_SIZE];
    const Card *card = NULL;
    Card originalCard;
    Card rechargedCard;
    Money money = {0};
    int cardLoadResult = 0;
    int32_t amountCent = 0;
    MoneyParseResult moneyParseResult = MONEY_PARSE_OK;
    DataResult dataResult = DATA_OK;
    time_t now = 0;

    if (validatorNormalizeInput(cardNameInput, cardName, sizeof(cardName)) != 0 ||
        !validatorIsValidCardName(cardName)) {
        return BIZ_ERR_INVALID_CARD_NAME;
    }

    if (validatorNormalizeInput(passwordInput, password, sizeof(password)) != 0 ||
        !validatorIsValidPassword(password)) {
        return BIZ_ERR_INVALID_PASSWORD;
    }

    if (validatorNormalizeInput(amountInput, amountText, sizeof(amountText)) != 0) {
        return BIZ_ERR_INVALID_AMOUNT;
    }

    moneyParseResult = validatorParseMoneyToCent(amountText, &amountCent);
    if (moneyParseResult == MONEY_PARSE_INVALID || amountCent <= 0) {
        return BIZ_ERR_INVALID_AMOUNT;
    }
    if (moneyParseResult == MONEY_PARSE_TOO_LARGE) {
        return BIZ_ERR_BALANCE_TOO_LARGE;
    }

    cardLoadResult = dataLoadCards();
    if (cardLoadResult < 0) {
        return mapDataResult((DataResult)cardLoadResult);
    }

    card = dataQueryCardByName(cardName);
    if (card == NULL || card->nDel != 0) {
        return BIZ_ERR_CARD_NOT_FOUND;
    }
    if (strcmp(card->aPwd, password) != 0) {
        return BIZ_ERR_WRONG_PASSWORD;
    }
    if (card->nStatus == CARD_STATUS_CANCELED) {
        return BIZ_ERR_CARD_CANCELED_FOR_RECHARGE;
    }
    if ((int64_t)card->nBalanceCent + amountCent >= MAX_BALANCE_CENT) {
        return BIZ_ERR_BALANCE_TOO_LARGE;
    }

    originalCard = *card;
    rechargedCard = *card;
    now = time(NULL);
    rechargedCard.nBalanceCent += amountCent;

    snprintf(money.aCardName, sizeof(money.aCardName), "%s", rechargedCard.aCardName);
    money.tTime = now;
    money.nStatus = 0;
    money.nMoneyCent = amountCent;
    money.nDel = 0;

    dataResult = dataUpdateCard(&rechargedCard);
    if (dataResult != DATA_OK) {
        return mapDataResult(dataResult);
    }

    dataResult = dataSaveMoney(&money);
    if (dataResult != DATA_OK) {
        if (dataUpdateCard(&originalCard) != DATA_OK) {
            return BIZ_ERR_SYSTEM;
        }
        return mapDataResult(dataResult);
    }

    if (rechargeRecord != NULL) {
        *rechargeRecord = money;
    }
    if (updatedCard != NULL) {
        *updatedCard = rechargedCard;
    }

    logOperation("充值");
    return BIZ_OK;
}

BizResult bizRefund(const char *cardNameInput,
                    const char *passwordInput,
                    Money *refundRecord,
                    Card *updatedCard)
{
    char cardName[INPUT_BUF_SIZE];
    char password[INPUT_BUF_SIZE];
    const Card *card = NULL;
    Card originalCard;
    Card refundedCard;
    Money money = {0};
    int cardLoadResult = 0;
    int32_t refundAmountCent = 0;
    DataResult dataResult = DATA_OK;
    time_t now = 0;

    if (validatorNormalizeInput(cardNameInput, cardName, sizeof(cardName)) != 0 ||
        !validatorIsValidCardName(cardName)) {
        return BIZ_ERR_INVALID_CARD_NAME;
    }

    if (validatorNormalizeInput(passwordInput, password, sizeof(password)) != 0 ||
        !validatorIsValidPassword(password)) {
        return BIZ_ERR_INVALID_PASSWORD;
    }

    cardLoadResult = dataLoadCards();
    if (cardLoadResult < 0) {
        return mapDataResult((DataResult)cardLoadResult);
    }

    card = dataQueryCardByName(cardName);
    if (card == NULL || card->nDel != 0) {
        return BIZ_ERR_CARD_NOT_FOUND;
    }
    if (strcmp(card->aPwd, password) != 0) {
        return BIZ_ERR_WRONG_PASSWORD;
    }
    if (card->nStatus == CARD_STATUS_CANCELED) {
        return BIZ_ERR_CARD_CANCELED_FOR_REFUND;
    }
    if (!isRefundAllowedStatus(card->nStatus)) {
        return BIZ_ERR_CARD_STATUS_INVALID_FOR_REFUND;
    }
    if (card->nBalanceCent <= 0) {
        return BIZ_ERR_BALANCE_NOT_ENOUGH;
    }

    refundAmountCent = card->nBalanceCent;
    originalCard = *card;
    refundedCard = *card;
    now = time(NULL);
    refundedCard.nBalanceCent = 0;

    snprintf(money.aCardName, sizeof(money.aCardName), "%s", refundedCard.aCardName);
    money.tTime = now;
    money.nStatus = 1;
    money.nMoneyCent = refundAmountCent;
    money.nDel = 0;

    dataResult = dataUpdateCard(&refundedCard);
    if (dataResult != DATA_OK) {
        return mapDataResult(dataResult);
    }

    dataResult = dataSaveMoney(&money);
    if (dataResult != DATA_OK) {
        if (dataUpdateCard(&originalCard) != DATA_OK) {
            return BIZ_ERR_SYSTEM;
        }
        return mapDataResult(dataResult);
    }

    if (refundRecord != NULL) {
        *refundRecord = money;
    }
    if (updatedCard != NULL) {
        *updatedCard = refundedCard;
    }

    logOperation("退费");
    return BIZ_OK;
}

BizResult bizRefundByAmount(const char *cardNameInput,
                            const char *passwordInput,
                            const char *amountInput,
                            Money *refundRecord,
                            Card *updatedCard)
{
    char cardName[INPUT_BUF_SIZE];
    char password[INPUT_BUF_SIZE];
    char amountText[INPUT_BUF_SIZE];
    const Card *card = NULL;
    Card originalCard;
    Card refundedCard;
    Money money = {0};
    int cardLoadResult = 0;
    int32_t refundAmountCent = 0;
    MoneyParseResult moneyParseResult = MONEY_PARSE_OK;
    DataResult dataResult = DATA_OK;
    time_t now = 0;

    if (validatorNormalizeInput(cardNameInput, cardName, sizeof(cardName)) != 0 ||
        !validatorIsValidCardName(cardName)) {
        return BIZ_ERR_INVALID_CARD_NAME;
    }

    if (validatorNormalizeInput(passwordInput, password, sizeof(password)) != 0 ||
        !validatorIsValidPassword(password)) {
        return BIZ_ERR_INVALID_PASSWORD;
    }

    if (validatorNormalizeInput(amountInput, amountText, sizeof(amountText)) != 0) {
        return BIZ_ERR_INVALID_AMOUNT;
    }

    moneyParseResult = validatorParseMoneyToCent(amountText, &refundAmountCent);
    if (moneyParseResult == MONEY_PARSE_INVALID || refundAmountCent <= 0) {
        return BIZ_ERR_INVALID_AMOUNT;
    }

    cardLoadResult = dataLoadCards();
    if (cardLoadResult < 0) {
        return mapDataResult((DataResult)cardLoadResult);
    }

    card = dataQueryCardByName(cardName);
    if (card == NULL || card->nDel != 0) {
        return BIZ_ERR_CARD_NOT_FOUND;
    }
    if (strcmp(card->aPwd, password) != 0) {
        return BIZ_ERR_WRONG_PASSWORD;
    }
    if (card->nStatus == CARD_STATUS_CANCELED) {
        return BIZ_ERR_CARD_CANCELED_FOR_REFUND;
    }
    if (!isRefundAllowedStatus(card->nStatus)) {
        return BIZ_ERR_CARD_STATUS_INVALID_FOR_REFUND;
    }
    if (card->nBalanceCent < refundAmountCent) {
        return BIZ_ERR_BALANCE_NOT_ENOUGH;
    }

    originalCard = *card;
    refundedCard = *card;
    now = time(NULL);
    refundedCard.nBalanceCent -= refundAmountCent;

    snprintf(money.aCardName, sizeof(money.aCardName), "%s", refundedCard.aCardName);
    money.tTime = now;
    money.nStatus = 1;
    money.nMoneyCent = refundAmountCent;
    money.nDel = 0;

    dataResult = dataUpdateCard(&refundedCard);
    if (dataResult != DATA_OK) {
        return mapDataResult(dataResult);
    }

    dataResult = dataSaveMoney(&money);
    if (dataResult != DATA_OK) {
        if (dataUpdateCard(&originalCard) != DATA_OK) {
            return BIZ_ERR_SYSTEM;
        }
        return mapDataResult(dataResult);
    }

    if (refundRecord != NULL) {
        *refundRecord = money;
    }
    if (updatedCard != NULL) {
        *updatedCard = refundedCard;
    }

    logOperation("退费");
    return BIZ_OK;
}

BizResult bizQueryBillingsByCardName(const char *cardNameInput, BillingQueryResult *result)
{
    char cardName[INPUT_BUF_SIZE];
    DataResult dataResult = DATA_OK;

    if (result == NULL) {
        return BIZ_ERR_SYSTEM;
    }

    result->items = NULL;
    result->count = 0;

    if (validatorNormalizeInput(cardNameInput, cardName, sizeof(cardName)) != 0 ||
        !validatorIsValidCardName(cardName)) {
        return BIZ_ERR_INVALID_CARD_NAME;
    }

    dataResult = dataQueryBillingsByCardName(cardName, &result->items, &result->count);
    if (dataResult == DATA_ERR_NOT_FOUND || dataResult == DATA_ERR_FILE_NOT_FOUND) {
        return BIZ_ERR_BILLING_RECORD_NOT_FOUND;
    }
    if (dataResult != DATA_OK) {
        return mapDataResult(dataResult);
    }

    logOperation("按卡号查询消费记录");
    return BIZ_OK;
}

BizResult bizQueryBillingsByCardNameAndRange(const char *cardNameInput,
                                             const char *startInput,
                                             const char *endInput,
                                             BillingQueryResult *result)
{
    char cardName[INPUT_BUF_SIZE];
    time_t startTime = (time_t)0;
    time_t endTime = (time_t)0;
    DataResult dataResult = DATA_OK;

    if (result == NULL) {
        return BIZ_ERR_SYSTEM;
    }

    result->items = NULL;
    result->count = 0;

    if (validatorNormalizeInput(cardNameInput, cardName, sizeof(cardName)) != 0 ||
        !validatorIsValidCardName(cardName)) {
        return BIZ_ERR_INVALID_CARD_NAME;
    }

    if (timeValidatorParseDateTime(startInput, &startTime) != 0 ||
        timeValidatorParseDateTime(endInput, &endTime) != 0 ||
        startTime > endTime) {
        return BIZ_ERR_INVALID_TIME_RANGE;
    }

    dataResult = dataQueryBillingsByCardNameAndRange(cardName, startTime, endTime, &result->items, &result->count);
    if (dataResult == DATA_ERR_NOT_FOUND || dataResult == DATA_ERR_FILE_NOT_FOUND) {
        return BIZ_ERR_BILLING_RECORD_NOT_FOUND;
    }
    if (dataResult != DATA_OK) {
        return mapDataResult(dataResult);
    }

    logOperation("按卡号和时间段查询消费记录");
    return BIZ_OK;
}

void bizFreeBillingQueryResult(BillingQueryResult *result)
{
    if (result == NULL) {
        return;
    }

    dataFreeQueriedBillings(result->items);
    result->items = NULL;
    result->count = 0;
}

BizResult bizGetBillingStatistics(const char *yearInput, BillingStatistics *statistics)
{
    Billing *records = NULL;
    size_t count = 0;
    size_t index = 0;
    int targetYear = 0;
    DataResult dataResult = DATA_OK;

    if (statistics == NULL) {
        return BIZ_ERR_SYSTEM;
    }

    memset(statistics, 0, sizeof(*statistics));

    if (parseStatisticsYear(yearInput, &targetYear) != BIZ_OK) {
        return BIZ_ERR_INVALID_TIME_RANGE;
    }

    dataResult = dataQueryAllBillings(&records, &count);
    if (dataResult == DATA_ERR_NOT_FOUND || dataResult == DATA_ERR_FILE_NOT_FOUND) {
        statistics->year = targetYear;
        return BIZ_OK;
    }
    if (dataResult != DATA_OK) {
        return mapDataResult(dataResult);
    }

    statistics->year = targetYear;
    for (index = 0; index < count; index++) {
        struct tm *localValue = NULL;
        int year = 0;
        int month = 0;

        if (records[index].nStatus != 1 || records[index].nDel != 0 || records[index].nAmountCent <= 0) {
            continue;
        }

        statistics->totalAmountCent += records[index].nAmountCent;

        localValue = localtime(&records[index].tEnd);
        if (localValue == NULL) {
            continue;
        }

        year = localValue->tm_year + 1900;
        month = localValue->tm_mon;
        if (year == targetYear && month >= 0 && month < 12) {
            statistics->monthlyAmountCent[month] += records[index].nAmountCent;
        }
    }

    dataFreeQueriedBillings(records);
    logOperation("营业额统计");
    return BIZ_OK;
}

void bizStatistics(void)
{
    printf("[业务逻辑层] 查询统计功能入口。\n");
    logOperation("查询统计");
}

BizResult bizCancelCard(const char *cardNameInput,
                        const char *passwordInput,
                        Money *refundRecord,
                        Card *updatedCard)
{
    char cardName[INPUT_BUF_SIZE];
    char password[INPUT_BUF_SIZE];
    const Card *card = NULL;
    Card originalCard;
    Card canceledCard;
    Money money = {0};
    int cardLoadResult = 0;
    int32_t refundAmountCent = 0;
    DataResult dataResult = DATA_OK;
    time_t now = 0;

    if (validatorNormalizeInput(cardNameInput, cardName, sizeof(cardName)) != 0 ||
        !validatorIsValidCardName(cardName)) {
        return BIZ_ERR_INVALID_CARD_NAME;
    }

    if (validatorNormalizeInput(passwordInput, password, sizeof(password)) != 0 ||
        !validatorIsValidPassword(password)) {
        return BIZ_ERR_INVALID_PASSWORD;
    }

    cardLoadResult = dataLoadCards();
    if (cardLoadResult < 0) {
        return mapDataResult((DataResult)cardLoadResult);
    }

    card = dataQueryCardByName(cardName);
    if (card == NULL || card->nDel != 0) {
        return BIZ_ERR_CARD_NOT_FOUND;
    }
    if (strcmp(card->aPwd, password) != 0) {
        return BIZ_ERR_WRONG_PASSWORD;
    }
    if (card->nStatus == CARD_STATUS_CANCELED) {
        return BIZ_ERR_CARD_CANCELED_FOR_CANCEL;
    }
    if (!isCancelAllowedStatus(card->nStatus)) {
        return BIZ_ERR_CARD_STATUS_INVALID_FOR_CANCEL;
    }

    refundAmountCent = card->nBalanceCent;
    originalCard = *card;
    canceledCard = *card;
    now = time(NULL);
    canceledCard.nStatus = CARD_STATUS_CANCELED;
    canceledCard.nBalanceCent = 0;

    dataResult = dataUpdateCard(&canceledCard);
    if (dataResult != DATA_OK) {
        return mapDataResult(dataResult);
    }

    if (refundAmountCent > 0) {
        snprintf(money.aCardName, sizeof(money.aCardName), "%s", canceledCard.aCardName);
        money.tTime = now;
        money.nStatus = 1;
        money.nMoneyCent = refundAmountCent;
        money.nDel = 0;

        dataResult = dataSaveMoney(&money);
        if (dataResult != DATA_OK) {
            if (dataUpdateCard(&originalCard) != DATA_OK) {
                return BIZ_ERR_SYSTEM;
            }
            return mapDataResult(dataResult);
        }
    }

    if (refundRecord != NULL) {
        *refundRecord = money;
    }
    if (updatedCard != NULL) {
        *updatedCard = canceledCard;
    }

    logOperation("注销卡");
    return BIZ_OK;
}

void bizShutdown(void)
{
    dataCleanup();
    dataCleanupBillings();
    dataCleanupMoneys();
}
