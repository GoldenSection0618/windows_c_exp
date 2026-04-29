#include "business.h"
#include "business_internal.h"
#include "business_legacy.h"

#include "billing_repository.h"
#include "billing_rule.h"
#include "billing_storage_file.h"
#include "card_repository.h"
#include "card_storage_file.h"
#include "common.h"
#include "operation_log.h"

#include <stdint.h>
#include <string.h>
#include <time.h>

static int isStartBillingAllowedStatus(int status)
{
    return status == CARD_STATUS_OFFLINE;
}


static int isStopBillingAllowedStatus(int status)
{
    return status == CARD_STATUS_ONLINE;
}


BizResult bizStartBilling(const char *cardNameInput,
                          const char *passwordInput,
                          time_t requestTime,
                          LogonInfo *logonInfo)
{
    int billingLoadResult = 0;
    Card card;
    Card updatedCard;
    Card originalCard;
    Billing billing;
    DataResult dataResult = DATA_OK;
    time_t now = 0;
    BizResult authResult = BIZ_OK;

    authResult = bizLoadCardByCredentialInternal(cardNameInput, passwordInput, &card);
    if (authResult != BIZ_OK) {
        return authResult;
    }

    billingLoadResult = dataLoadBillings();
    if (billingLoadResult < 0 && billingLoadResult != DATA_ERR_FILE_NOT_FOUND) {
        return mapDataResult((DataResult)billingLoadResult);
    }

    if (card.nStatus == CARD_STATUS_CANCELED) {
        return BIZ_ERR_CARD_CANCELED_FOR_START;
    }
    if (!isStartBillingAllowedStatus(card.nStatus)) {
        return BIZ_ERR_CARD_UNAVAILABLE;
    }
    if (card.nBalanceCent < 0) {
        return BIZ_ERR_BALANCE_NOT_ENOUGH;
    }

    originalCard = card;
    updatedCard = card;
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
    int billingLoadResult = 0;
    Card card;
    const Billing *billing = NULL;
    Card originalCard;
    Card updatedCard;
    Billing updatedBilling;
    Rate rate;
    time_t now = 0;
    int durationMinutes = 0;
    int32_t amountCent = 0;
    DataResult dataResult = DATA_OK;
    BizResult authResult = BIZ_OK;

    authResult = bizLoadCardByCredentialInternal(cardNameInput, passwordInput, &card);
    if (authResult != BIZ_OK) {
        return authResult;
    }

    billingLoadResult = dataLoadBillings();
    if (billingLoadResult < 0) {
        if (billingLoadResult == DATA_ERR_FILE_NOT_FOUND) {
            return BIZ_ERR_NO_UNSETTLED_BILLING;
        }
        return mapDataResult((DataResult)billingLoadResult);
    }

    if (!isStopBillingAllowedStatus(card.nStatus)) {
        return BIZ_ERR_CARD_STATUS_INVALID_FOR_STOP;
    }

    billing = dataQueryLatestUnsettledBillingByCardName(card.aCardName);
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

    originalCard = card;
    updatedCard = card;
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
