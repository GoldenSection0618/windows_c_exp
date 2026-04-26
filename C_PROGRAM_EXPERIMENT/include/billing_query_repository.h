#ifndef BILLING_QUERY_REPOSITORY_H
#define BILLING_QUERY_REPOSITORY_H

#include <stddef.h>
#include <time.h>

#include "card_repository.h"
#include "model.h"

DataResult dataQueryBillingsByCardName(const char *cardName, Billing **records, size_t *count);
DataResult dataQueryBillingsByCardNameAndRange(const char *cardName,
                                               time_t startTime,
                                               time_t endTime,
                                               Billing **records,
                                               size_t *count);
DataResult dataQueryAllBillings(Billing **records, size_t *count);
void dataFreeQueriedBillings(Billing *records);

#endif
