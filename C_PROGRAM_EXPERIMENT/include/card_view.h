#ifndef CARD_VIEW_H
#define CARD_VIEW_H

#include <stddef.h>
#include <stdint.h>

#include "card_query.h"
#include "model.h"

void viewShowCardSummary(const Card *card);
void viewShowQueryCardDetails(const Card *card);
void viewShowFuzzyQueryResults(const char *keyword, const Card *cards, size_t count);
void viewShowAdvancedQueryPage(const Card *cards,
                               size_t count,
                               size_t pageIndex,
                               size_t pageSize,
                               const CardQueryOption *option);
void viewShowLogonInfo(const LogonInfo *logonInfo);
void viewShowSettleInfo(const SettleInfo *settleInfo);
void viewShowRechargeInfo(const Card *card, int32_t rechargeAmountCent);
void viewShowRefundInfo(const Card *card, int32_t refundAmountCent);
void viewShowCancelCardInfo(const Card *card, int32_t refundAmountCent);
void viewShowBillingRecords(const Billing *items, size_t count);

#endif
