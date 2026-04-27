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

static int isCancelAllowedStatus(int status)
{
    return status == CARD_STATUS_OFFLINE;
}


BizResult bizUserCancelCardWithPassword(const LoginSession *session,
                                        const char *cardNameInput,
                                        const char *passwordInput,
                                        Money *refundRecord,
                                        Card *updatedCard)
{
    char cardName[INPUT_BUF_SIZE];
    Card card;
    BizResult result = BIZ_OK;

    if (!bizIsUserSession(session)) {
        return BIZ_ERR_SYSTEM;
    }

    if (validatorNormalizeInput(cardNameInput, cardName, sizeof(cardName)) != 0 ||
        strcmp(cardName, session->cardName) != 0) {
        return BIZ_ERR_WRONG_PASSWORD;
    }

    result = bizLoadCardByCredentialInternal(cardNameInput, passwordInput, &card);
    if (result != BIZ_OK) {
        return result == BIZ_ERR_INVALID_PASSWORD ? BIZ_ERR_WRONG_PASSWORD : result;
    }
    return bizCancelCard(card.aCardName, card.aPwd, refundRecord, updatedCard);
}


BizResult bizCancelCard(const char *cardNameInput,
                        const char *passwordInput,
                        Money *refundRecord,
                        Card *updatedCard)
{
    Card card;
    Card originalCard;
    Card canceledCard;
    Money money = {0};
    int32_t refundAmountCent = 0;
    DataResult dataResult = DATA_OK;
    time_t now = 0;
    BizResult authResult = BIZ_OK;

    authResult = bizLoadCardByCredentialInternal(cardNameInput, passwordInput, &card);
    if (authResult != BIZ_OK) {
        return authResult;
    }
    if (card.nStatus == CARD_STATUS_CANCELED) {
        return BIZ_ERR_CARD_CANCELED_FOR_CANCEL;
    }
    if (!isCancelAllowedStatus(card.nStatus)) {
        return BIZ_ERR_CARD_STATUS_INVALID_FOR_CANCEL;
    }

    refundAmountCent = card.nBalanceCent;
    originalCard = card;
    canceledCard = card;
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
