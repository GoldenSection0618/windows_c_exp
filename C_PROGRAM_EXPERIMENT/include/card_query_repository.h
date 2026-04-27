#ifndef CARD_QUERY_REPOSITORY_H
#define CARD_QUERY_REPOSITORY_H

#include <stddef.h>

#include "card_repository.h"
#include "model.h"

DataResult dataQueryAllCardsSnapshot(Card **cards, size_t *count);
void dataFreeCardsSnapshot(Card *cards);

#endif
