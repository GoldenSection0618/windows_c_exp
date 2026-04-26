#ifndef MONEY_STORAGE_H
#define MONEY_STORAGE_H

#include "model.h"

typedef struct MoneyNode {
    Money moneyData;
    struct MoneyNode *pNext;
} MoneyNode;

#endif
