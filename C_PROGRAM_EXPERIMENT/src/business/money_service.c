#include "business.h"
#include "business_internal.h"
#include "business_legacy.h"

#include "card_repository.h"
#include "card_storage_file.h"
#include "card_validator.h"
#include "common.h"
#include "money_storage_file.h"
#include "operation_log.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

static int isRefundAllowedStatus(int status)
{
    return status == CARD_STATUS_OFFLINE;
}

BizResult bizRecharge(const char *cardNameInput,
                      const char *passwordInput,
                      const char *amountInput,
                      Money *rechargeRecord,
                      Card *updatedCard)
{
    char amountText[INPUT_BUF_SIZE];
    Card card;
    Card originalCard;
    Card rechargedCard;
    Money money = {0};
    int32_t amountCent = 0;
    BizResult authResult = BIZ_OK;
    MoneyParseResult moneyParseResult = MONEY_PARSE_OK;
    DataResult dataResult = DATA_OK;
    time_t now = 0;

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

    authResult = bizLoadCardByCredentialInternal(cardNameInput, passwordInput, &card);
    if (authResult != BIZ_OK) {
        return authResult;
    }
    if (card.nStatus == CARD_STATUS_CANCELED) {
        return BIZ_ERR_CARD_CANCELED_FOR_RECHARGE;
    }
    if ((int64_t)card.nBalanceCent + amountCent >= MAX_BALANCE_CENT) {
        return BIZ_ERR_BALANCE_TOO_LARGE;
    }

    originalCard = card;
    rechargedCard = card;
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
    Card card;
    Card originalCard;
    Card refundedCard;
    Money money = {0};
    int32_t refundAmountCent = 0;
    BizResult authResult = BIZ_OK;
    DataResult dataResult = DATA_OK;
    time_t now = 0;

    authResult = bizLoadCardByCredentialInternal(cardNameInput, passwordInput, &card);
    if (authResult != BIZ_OK) {
        return authResult;
    }
    if (card.nStatus == CARD_STATUS_CANCELED) {
        return BIZ_ERR_CARD_CANCELED_FOR_REFUND;
    }
    if (!isRefundAllowedStatus(card.nStatus)) {
        return BIZ_ERR_CARD_STATUS_INVALID_FOR_REFUND;
    }
    if (card.nBalanceCent <= 0) {
        return BIZ_ERR_BALANCE_NOT_ENOUGH;
    }

    refundAmountCent = card.nBalanceCent;
    originalCard = card;
    refundedCard = card;
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
    char amountText[INPUT_BUF_SIZE];
    Card card;
    Card originalCard;
    Card refundedCard;
    Money money = {0};
    int32_t refundAmountCent = 0;
    BizResult authResult = BIZ_OK;
    MoneyParseResult moneyParseResult = MONEY_PARSE_OK;
    DataResult dataResult = DATA_OK;
    time_t now = 0;

    if (validatorNormalizeInput(amountInput, amountText, sizeof(amountText)) != 0) {
        return BIZ_ERR_INVALID_AMOUNT;
    }

    moneyParseResult = validatorParseMoneyToCent(amountText, &refundAmountCent);
    if (moneyParseResult == MONEY_PARSE_INVALID || refundAmountCent <= 0) {
        return BIZ_ERR_INVALID_AMOUNT;
    }

    authResult = bizLoadCardByCredentialInternal(cardNameInput, passwordInput, &card);
    if (authResult != BIZ_OK) {
        return authResult;
    }
    if (card.nStatus == CARD_STATUS_CANCELED) {
        return BIZ_ERR_CARD_CANCELED_FOR_REFUND;
    }
    if (!isRefundAllowedStatus(card.nStatus)) {
        return BIZ_ERR_CARD_STATUS_INVALID_FOR_REFUND;
    }
    if (card.nBalanceCent < refundAmountCent) {
        return BIZ_ERR_BALANCE_NOT_ENOUGH;
    }

    originalCard = card;
    refundedCard = card;
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
