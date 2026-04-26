#ifndef BILLING_STORAGE_H
#define BILLING_STORAGE_H

#include "model.h"

typedef struct BillingNode {
    Billing billingData;
    struct BillingNode *pNext;
} BillingNode;

#endif
