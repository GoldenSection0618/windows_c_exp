#include "business.h"
#include "business_internal.h"

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


BizResult bizUserRegister(const char *cardNameInput, const char *passwordInput, Card *createdCard)
{
    return bizAddCard(cardNameInput, passwordInput, "100", createdCard);
}


BizResult bizAdminQueryCard(const LoginSession *session, const char *cardNameInput, Card *queriedCard)
{
    if (!bizIsAdminSession(session)) {
        return BIZ_ERR_SYSTEM;
    }

    return bizQueryCard(cardNameInput, queriedCard);
}

BizResult bizAdminQueryCardsByKeyword(const LoginSession *session,
                                      const char *keywordInput,
                                      Card *buffer,
                                      size_t capacity,
                                      size_t *actualCount,
                                      size_t *requiredCount)
{
    if (!bizIsAdminSession(session)) {
        return BIZ_ERR_SYSTEM;
    }

    return bizQueryCardsByKeyword(keywordInput, buffer, capacity, actualCount, requiredCount);
}


BizResult bizUserQueryBalance(const LoginSession *session, Card *queriedCard)
{
    if (!bizIsUserSession(session)) {
        return BIZ_ERR_SYSTEM;
    }

    return bizQueryCard(session->cardName, queriedCard);
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

