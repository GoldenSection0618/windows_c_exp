#ifndef BUSINESS_LEGACY_H
#define BUSINESS_LEGACY_H

#include "business.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Legacy console/internal API. New GUI code must not include this header directly. */

BizResult bizStartBilling(const char *cardNameInput,
                          const char *passwordInput,
                          time_t requestTime,
                          LogonInfo *logonInfo);

BizResult bizStopBilling(const char *cardNameInput,
                         const char *passwordInput,
                         time_t requestTime,
                         SettleInfo *settleInfo);

BizResult bizRecharge(const char *cardNameInput,
                      const char *passwordInput,
                      const char *amountInput,
                      Money *rechargeRecord,
                      Card *updatedCard);

BizResult bizRefund(const char *cardNameInput,
                    const char *passwordInput,
                    Money *refundRecord,
                    Card *updatedCard);

BizResult bizRefundByAmount(const char *cardNameInput,
                            const char *passwordInput,
                            const char *amountInput,
                            Money *refundRecord,
                            Card *updatedCard);

BizResult bizCancelCard(const char *cardNameInput,
                        const char *passwordInput,
                        Money *refundRecord,
                        Card *updatedCard);

BizResult bizQueryBillingsByCardName(const char *cardNameInput,
                                     BillingQueryResult *result);

BizResult bizQueryBillingsByCardNameAndRange(const char *cardNameInput,
                                             const char *startInput,
                                             const char *endInput,
                                             BillingQueryResult *result);

void bizFreeBillingQueryResult(BillingQueryResult *result);

BizResult bizGetBillingStatistics(const char *yearMonthInput,
                                  BillingStatistics *statistics);

void bizStatistics(void);

#ifdef __cplusplus
}
#endif

#endif
