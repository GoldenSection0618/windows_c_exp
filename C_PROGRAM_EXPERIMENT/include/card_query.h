#ifndef CARD_QUERY_H
#define CARD_QUERY_H

#include <stddef.h>
#include <stdint.h>

#include "business.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum CardQueryFilterType {
    CARD_QUERY_FILTER_ALL = 1,
    CARD_QUERY_FILTER_OFFLINE = 2,
    CARD_QUERY_FILTER_ONLINE = 3,
    CARD_QUERY_FILTER_CANCELED = 4,
    CARD_QUERY_FILTER_LOW_BALANCE = 5
} CardQueryFilterType;

typedef enum CardQuerySortType {
    CARD_QUERY_SORT_BALANCE_ASC = 1,
    CARD_QUERY_SORT_BALANCE_DESC = 2,
    CARD_QUERY_SORT_USE_COUNT_DESC = 3,
    CARD_QUERY_SORT_TOTAL_USE_DESC = 4,
    CARD_QUERY_SORT_LAST_USE_DESC = 5
} CardQuerySortType;

typedef struct CardQueryOption {
    CardQueryFilterType filterType;
    CardQuerySortType sortType;
    int32_t lowBalanceLimitCent;
} CardQueryOption;

BizResult bizQueryCardsAdvanced(const CardQueryOption *option, Card **outCards, size_t *outCount);
BizResult bizAdminQueryCardsAdvanced(const LoginSession *session,
                                     const CardQueryOption *option,
                                     Card **outCards,
                                     size_t *outCount);
void bizFreeCardQueryResult(Card *cards);
const char *bizGetCardQueryFilterText(CardQueryFilterType filterType);
const char *bizGetCardQuerySortText(CardQuerySortType sortType);

#ifdef __cplusplus
}
#endif

#endif
