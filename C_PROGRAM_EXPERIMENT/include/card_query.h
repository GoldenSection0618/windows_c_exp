#ifndef CARD_QUERY_H
#define CARD_QUERY_H

#include <stddef.h>
#include <stdint.h>

#include "model.h"

#define CARD_QUERY_DEFAULT_PAGE_SIZE 5
#define CARD_QUERY_MAX_PAGE_SIZE 50

typedef enum CardFilterType {
    CARD_FILTER_ALL = 1,
    CARD_FILTER_OFFLINE = 2,
    CARD_FILTER_ONLINE = 3,
    CARD_FILTER_CANCELED = 4,
    CARD_FILTER_LOW_BALANCE = 5
} CardFilterType;

typedef enum CardSortType {
    CARD_SORT_BALANCE_ASC = 1,
    CARD_SORT_BALANCE_DESC = 2,
    CARD_SORT_USE_COUNT_DESC = 3,
    CARD_SORT_TOTAL_USE_DESC = 4,
    CARD_SORT_LAST_USE_DESC = 5
} CardSortType;

typedef struct CardQueryOption {
    CardFilterType filterType;
    CardSortType sortType;
    int32_t lowBalanceLimitCent;
    size_t pageIndex;
    size_t pageSize;
} CardQueryOption;

typedef struct CardQueryPage {
    Card *items;
    size_t itemCount;
    size_t totalCount;
    size_t pageIndex;
    size_t pageSize;
    size_t pageCount;
} CardQueryPage;

void cardQueryInitOption(CardQueryOption *option);
const char *cardQueryGetFilterText(CardFilterType filterType);
const char *cardQueryGetSortText(CardSortType sortType);

#endif
