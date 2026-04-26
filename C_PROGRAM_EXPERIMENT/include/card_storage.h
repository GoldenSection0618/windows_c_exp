#ifndef CARD_STORAGE_H
#define CARD_STORAGE_H

#include "model.h"

typedef struct CardNode {
    Card cardData;
    struct CardNode *pNext;
} CardNode;

#endif
