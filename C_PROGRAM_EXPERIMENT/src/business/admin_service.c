#include "business.h"
#include "business_internal.h"
#include "business_legacy.h"

#include "common.h"

#include <time.h>

BizResult bizAdminStopBilling(const LoginSession *session,
                              const char *cardNameInput,
                              time_t requestTime,
                              SettleInfo *settleInfo)
{
    char password[INPUT_BUF_SIZE];
    BizResult result = BIZ_OK;

    if (!bizIsAdminSession(session)) {
        return BIZ_ERR_SYSTEM;
    }

    result = bizGetCardPasswordByName(cardNameInput, password, sizeof(password));
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

    if (!bizIsAdminSession(session)) {
        return BIZ_ERR_SYSTEM;
    }

    result = bizGetCardPasswordByName(cardNameInput, password, sizeof(password));
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

    if (!bizIsAdminSession(session)) {
        return BIZ_ERR_SYSTEM;
    }

    result = bizGetCardPasswordByName(cardNameInput, password, sizeof(password));
    if (result != BIZ_OK) {
        return result;
    }

    return bizRefundByAmount(cardNameInput, password, amountInput, refundRecord, updatedCard);
}
