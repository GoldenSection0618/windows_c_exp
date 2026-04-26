#ifndef BILLING_REPOSITORY_H
#define BILLING_REPOSITORY_H

#include "card_repository.h"
#include "billing_query_repository.h"

int dataAddBilling(const Billing *billing);
const Billing *dataQueryLatestUnsettledBillingByCardName(const char *cardName);
void dataCleanupBillings(void);

#endif
