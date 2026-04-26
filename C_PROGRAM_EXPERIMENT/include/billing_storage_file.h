#ifndef BILLING_STORAGE_FILE_H
#define BILLING_STORAGE_FILE_H

#include "billing_repository.h"

DataResult dataSaveBilling(const Billing *billing);
int dataLoadBillings(void);
int dataGetBillingCount(void);
DataResult dataUpdateBilling(const Billing *billing);

#endif
