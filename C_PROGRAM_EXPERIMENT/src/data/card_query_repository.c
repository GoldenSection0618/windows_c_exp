#include "card_query_repository.h"

#include "card_storage_file.h"

#include <stdlib.h>

DataResult dataQueryAllCardsSnapshot(Card **cards, size_t *count)
{
    Card *buffer = NULL;
    size_t actualCount = 0;
    size_t requiredCount = 0;
    int loadResult = 0;
    DataResult queryResult = DATA_OK;

    if (cards == NULL || count == NULL) {
        return DATA_ERR_INVALID_ARG;
    }

    *cards = NULL;
    *count = 0;

    loadResult = dataLoadCards();
    if (loadResult < 0) {
        return (DataResult)loadResult;
    }
    if (loadResult == 0) {
        return DATA_ERR_NOT_FOUND;
    }

    requiredCount = (size_t)loadResult;
    buffer = (Card *)malloc(requiredCount * sizeof(Card));
    if (buffer == NULL) {
        return DATA_ERR_NO_MEMORY;
    }

    queryResult = dataQueryAllCards(buffer, requiredCount, &actualCount, &requiredCount);
    if (queryResult != DATA_OK) {
        free(buffer);
        return queryResult;
    }

    *cards = buffer;
    *count = actualCount;
    return DATA_OK;
}

void dataFreeCardsSnapshot(Card *cards)
{
    free(cards);
}
