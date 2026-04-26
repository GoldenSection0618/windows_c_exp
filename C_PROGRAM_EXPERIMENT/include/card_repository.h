#ifndef CARD_REPOSITORY_H
#define CARD_REPOSITORY_H

#include <stddef.h>

#include "model.h"

typedef enum DataResult {
    DATA_OK = 0,
    DATA_ERR_DUPLICATE = -1,
    DATA_ERR_NO_MEMORY = -2,
    DATA_ERR_INVALID_ARG = -3,
    DATA_ERR_FILE_OPEN = -4,
    DATA_ERR_FILE_NOT_FOUND = -5,
    DATA_ERR_RECORD_FORMAT = -6,
    DATA_ERR_TIME_PARSE = -7,
    DATA_ERR_NOT_FOUND = -8
} DataResult;

int dataAddCard(const Card *card);
DataResult dataDeleteCardByName(const char *cardName);
const Card *dataQueryCardByName(const char *cardName);
DataResult dataQueryCardsByKeyword(const char *keyword,
                                   Card *outCards,
                                   size_t capacity,
                                   size_t *actualCount,
                                   size_t *requiredCount);
int dataCardExists(const char *cardName);
void dataCleanup(void);

#endif
