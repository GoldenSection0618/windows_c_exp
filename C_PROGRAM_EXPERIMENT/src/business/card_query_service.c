#include "card_query.h"

#include "business_internal.h"
#include "card_storage_file.h"
#include "operation_log.h"

#include <stdlib.h>
#include <string.h>

static int compareInt32Value(int32_t left, int32_t right)
{
    if (left < right) {
        return -1;
    }
    if (left > right) {
        return 1;
    }
    return 0;
}

static int compareTimeValue(time_t left, time_t right)
{
    if (left < right) {
        return -1;
    }
    if (left > right) {
        return 1;
    }
    return 0;
}

static int compareBalanceAsc(const void *left, const void *right)
{
    const Card *a = (const Card *)left;
    const Card *b = (const Card *)right;
    int result = compareInt32Value(a->nBalanceCent, b->nBalanceCent);
    return result != 0 ? result : strcmp(a->aCardName, b->aCardName);
}

static int compareBalanceDesc(const void *left, const void *right)
{
    const Card *a = (const Card *)left;
    const Card *b = (const Card *)right;
    int result = compareInt32Value(b->nBalanceCent, a->nBalanceCent);
    return result != 0 ? result : strcmp(a->aCardName, b->aCardName);
}

static int compareUseCountDesc(const void *left, const void *right)
{
    const Card *a = (const Card *)left;
    const Card *b = (const Card *)right;
    int result = 0;

    if (a->nUseCount < b->nUseCount) {
        result = 1;
    } else if (a->nUseCount > b->nUseCount) {
        result = -1;
    }

    return result != 0 ? result : strcmp(a->aCardName, b->aCardName);
}

static int compareTotalUseDesc(const void *left, const void *right)
{
    const Card *a = (const Card *)left;
    const Card *b = (const Card *)right;
    int result = compareInt32Value(b->nTotalUseCent, a->nTotalUseCent);
    return result != 0 ? result : strcmp(a->aCardName, b->aCardName);
}

static int compareLastUseDesc(const void *left, const void *right)
{
    const Card *a = (const Card *)left;
    const Card *b = (const Card *)right;
    int result = compareTimeValue(b->tLast, a->tLast);
    return result != 0 ? result : strcmp(a->aCardName, b->aCardName);
}

static int isValidFilter(CardQueryFilterType filterType)
{
    return filterType >= CARD_QUERY_FILTER_ALL && filterType <= CARD_QUERY_FILTER_LOW_BALANCE;
}

static int isValidSort(CardQuerySortType sortType)
{
    return sortType >= CARD_QUERY_SORT_BALANCE_ASC && sortType <= CARD_QUERY_SORT_LAST_USE_DESC;
}

static int matchCardFilter(const Card *card, const CardQueryOption *option)
{
    if (card == NULL || option == NULL || card->nDel != 0) {
        return 0;
    }

    switch (option->filterType) {
    case CARD_QUERY_FILTER_ALL:
        return 1;
    case CARD_QUERY_FILTER_OFFLINE:
        return card->nStatus == CARD_STATUS_OFFLINE;
    case CARD_QUERY_FILTER_ONLINE:
        return card->nStatus == CARD_STATUS_ONLINE;
    case CARD_QUERY_FILTER_CANCELED:
        return card->nStatus == CARD_STATUS_CANCELED;
    case CARD_QUERY_FILTER_LOW_BALANCE:
        return card->nBalanceCent < option->lowBalanceLimitCent;
    default:
        return 0;
    }
}

static int (*getComparator(CardQuerySortType sortType))(const void *, const void *)
{
    switch (sortType) {
    case CARD_QUERY_SORT_BALANCE_ASC:
        return compareBalanceAsc;
    case CARD_QUERY_SORT_BALANCE_DESC:
        return compareBalanceDesc;
    case CARD_QUERY_SORT_USE_COUNT_DESC:
        return compareUseCountDesc;
    case CARD_QUERY_SORT_TOTAL_USE_DESC:
        return compareTotalUseDesc;
    case CARD_QUERY_SORT_LAST_USE_DESC:
        return compareLastUseDesc;
    default:
        return compareBalanceAsc;
    }
}

static BizResult queryAllCards(Card **cards, size_t *count)
{
    Card *items = NULL;
    size_t actualCount = 0;
    size_t requiredCount = 0;
    int loadResult = 0;
    DataResult queryResult = DATA_OK;

    if (cards == NULL || count == NULL) {
        return BIZ_ERR_SYSTEM;
    }

    *cards = NULL;
    *count = 0;

    loadResult = dataLoadCards();
    if (loadResult < 0) {
        return mapDataResult((DataResult)loadResult);
    }

    queryResult = dataQueryAllCards(NULL, 0, &actualCount, &requiredCount);
    if (queryResult != DATA_OK) {
        return mapDataResult(queryResult);
    }
    if (requiredCount == 0) {
        return BIZ_ERR_NO_MATCHED_CARD;
    }

    items = (Card *)malloc(requiredCount * sizeof(Card));
    if (items == NULL) {
        return BIZ_ERR_NO_MEMORY;
    }

    queryResult = dataQueryAllCards(items, requiredCount, &actualCount, &requiredCount);
    if (queryResult != DATA_OK || actualCount != requiredCount) {
        free(items);
        return queryResult == DATA_OK ? BIZ_ERR_SYSTEM : mapDataResult(queryResult);
    }

    *cards = items;
    *count = actualCount;
    return BIZ_OK;
}

BizResult bizQueryCardsAdvanced(const CardQueryOption *option, Card **outCards, size_t *outCount)
{
    CardQueryOption normalizedOption;
    Card *allCards = NULL;
    Card *matchedCards = NULL;
    size_t allCount = 0;
    size_t matchedCount = 0;
    size_t index = 0;
    BizResult result = BIZ_OK;

    if (option == NULL || outCards == NULL || outCount == NULL) {
        return BIZ_ERR_SYSTEM;
    }

    *outCards = NULL;
    *outCount = 0;
    normalizedOption = *option;

    if (!isValidFilter(normalizedOption.filterType) || !isValidSort(normalizedOption.sortType)) {
        return BIZ_ERR_SYSTEM;
    }
    if (normalizedOption.filterType == CARD_QUERY_FILTER_LOW_BALANCE && normalizedOption.lowBalanceLimitCent <= 0) {
        return BIZ_ERR_INVALID_AMOUNT;
    }

    result = queryAllCards(&allCards, &allCount);
    if (result != BIZ_OK) {
        return result;
    }

    matchedCards = (Card *)malloc(allCount * sizeof(Card));
    if (matchedCards == NULL) {
        free(allCards);
        return BIZ_ERR_NO_MEMORY;
    }

    for (index = 0; index < allCount; index++) {
        if (matchCardFilter(&allCards[index], &normalizedOption)) {
            matchedCards[matchedCount] = allCards[index];
            matchedCount++;
        }
    }
    free(allCards);

    if (matchedCount == 0) {
        free(matchedCards);
        return BIZ_ERR_NO_MATCHED_CARD;
    }

    qsort(matchedCards, matchedCount, sizeof(Card), getComparator(normalizedOption.sortType));
    logOperation("高级查询卡");

    *outCards = matchedCards;
    *outCount = matchedCount;
    return BIZ_OK;
}

BizResult bizAdminQueryCardsAdvanced(const LoginSession *session,
                                     const CardQueryOption *option,
                                     Card **outCards,
                                     size_t *outCount)
{
    if (!bizIsAdminSession(session)) {
        return BIZ_ERR_SYSTEM;
    }

    return bizQueryCardsAdvanced(option, outCards, outCount);
}

void bizFreeCardQueryResult(Card *cards)
{
    free(cards);
}

const char *bizGetCardQueryFilterText(CardQueryFilterType filterType)
{
    switch (filterType) {
    case CARD_QUERY_FILTER_ALL:
        return "全部卡";
    case CARD_QUERY_FILTER_OFFLINE:
        return "未上机卡";
    case CARD_QUERY_FILTER_ONLINE:
        return "正在上机卡";
    case CARD_QUERY_FILTER_CANCELED:
        return "已注销卡";
    case CARD_QUERY_FILTER_LOW_BALANCE:
        return "低余额卡";
    default:
        return "未知筛选";
    }
}

const char *bizGetCardQuerySortText(CardQuerySortType sortType)
{
    switch (sortType) {
    case CARD_QUERY_SORT_BALANCE_ASC:
        return "余额升序";
    case CARD_QUERY_SORT_BALANCE_DESC:
        return "余额降序";
    case CARD_QUERY_SORT_USE_COUNT_DESC:
        return "使用次数降序";
    case CARD_QUERY_SORT_TOTAL_USE_DESC:
        return "累计消费降序";
    case CARD_QUERY_SORT_LAST_USE_DESC:
        return "最后使用时间降序";
    default:
        return "未知排序";
    }
}
