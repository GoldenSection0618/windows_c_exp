#include "business.h"
#include "business_internal.h"
#include "business_legacy.h"

#include <time.h>

BizResult bizUserStartBilling(const LoginSession *session, time_t requestTime, LogonInfo *logonInfo)
{
    Card card;
    BizResult result = BIZ_OK;

    if (!bizIsUserSession(session)) {
        return BIZ_ERR_SYSTEM;
    }

    result = bizLoadCardByName(session->cardName, &card);
    if (result != BIZ_OK) {
        return result;
    }
    return bizStartBilling(card.aCardName, card.aPwd, requestTime, logonInfo);
}


BizResult bizUserStopBilling(const LoginSession *session, time_t requestTime, SettleInfo *settleInfo)
{
    Card card;
    BizResult result = BIZ_OK;

    if (!bizIsUserSession(session)) {
        return BIZ_ERR_SYSTEM;
    }

    result = bizLoadCardByName(session->cardName, &card);
    if (result != BIZ_OK) {
        return result;
    }
    return bizStopBilling(card.aCardName, card.aPwd, requestTime, settleInfo);
}


BizResult bizUserRecharge(const LoginSession *session,
                          const char *amountInput,
                          Money *rechargeRecord,
                          Card *updatedCard)
{
    Card card;
    BizResult result = BIZ_OK;

    if (!bizIsUserSession(session)) {
        return BIZ_ERR_SYSTEM;
    }

    result = bizLoadCardByName(session->cardName, &card);
    if (result != BIZ_OK) {
        return result;
    }
    return bizRecharge(card.aCardName, card.aPwd, amountInput, rechargeRecord, updatedCard);
}


BizResult bizUserRefundByAmount(const LoginSession *session,
                                const char *amountInput,
                                Money *refundRecord,
                                Card *updatedCard)
{
    Card card;
    BizResult result = BIZ_OK;

    if (!bizIsUserSession(session)) {
        return BIZ_ERR_SYSTEM;
    }

    result = bizLoadCardByName(session->cardName, &card);
    if (result != BIZ_OK) {
        return result;
    }
    return bizRefundByAmount(card.aCardName, card.aPwd, amountInput, refundRecord, updatedCard);
}
