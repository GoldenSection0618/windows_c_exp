#include "card_query.h"

#include "business.h"
#include "card_repository.h"
#include "card_storage_file.h"
#include "card_validator.h"
#include "common.h"

#include <stdlib.h>
#include <string.h>

static CardSortType g_sortType = CARD_SORT_BALANCE_ASC;

static BizResult mapAdvancedQueryDataResult(DataResult result)
{
    switch (result) {
    case DATA_OK:
        return BIZ_OK;
    case DATA_ERR_FILE_OPEN:
        return BIZ_ERR_FILE_OPEN;
    case DATA_ERR_FILE_NOT_FOUND:
        return BIZ_ERR_FILE_NOT_FOUND;
    case DATA_ERR_RECORD_FORMAT:
        return BIZ_ERR_RECORD_FORMAT;
    case DATA_ERR_TIME_PARSE:
        return BIZ_ERR_TIME_PARSE;
    case DATA_ERR_NO_MEMORY:
        return BIZ_ERR_NO_MEMORY;
    default:
        return BIZ_ERR_SYSTEM;
    }
}

void cardQueryInitOption(CardQueryOption *option)
{
    if (option == NULL) {
        return;
    }

    option->filterType = CARD_FILTER_ALL;
    option->sortType = CARD_SORT_BALANCE_ASC;
    option->lowBalanceLimitCent = 0;
    option->pageIndex = 0;
    option->pageSize = CARD_QUERY_DEFAULT_PAGE_SIZE;
}

const char *cardQueryGetFilterText(CardFilterType filterType)
{
    switch (filterType) {
    case CARD_FILTER_ALL:
        return "全部卡";
    case CARD_FILTER_OFFLINE:
        return "未上机卡";
    case CARD_FILTER_ONLINE:
        return "正在上机卡";
    case CARD_FILTER_CANCELED:
        return "已注销卡";
    case CARD_FILTER_LOW_BALANCE:
        return "低余额卡";
    default:
        return "未知筛选条件";
    }
}

const char *cardQueryGetSortText(CardSortType sortType)
{
    switch (sortType) {
    case CARD_SORT_BALANCE_ASC:
        return "余额升序";
    case CARD_SORT_BALANCE_DESC:
        return "余额降序";
    case CARD_SORT_USE_COUNT_DESC:
        return "使用次数降序";
    case CARD_SORT_TOTAL_USE_DESC:
        return "累计消费降序";
    case CARD_SORT_LAST_USE_DESC:
        return "最后使用时间降序";
    default:
        return "未知排序方式";
    }
}

static int isValidFilterType(CardFilterType filterType)
{
    return filterType >= CARD_FILTER_ALL && filterType <= CARD_FILTER_LOW_BALANCE;
}

static int isValidSortType(CardSortType sortType)
{
    return sortType >= CARD_SORT_BALANCE_ASC && sortType <= CARD_SORT_LAST_USE_DESC;
}

static int isCardMatchedByOption(const Card *card, const CardQueryOption *option)
{
    if (card == NULL || option == NULL || card->nDel != 0) {
        return 0;
    }

    switch (option->filterType) {
    case CARD_FILTER_ALL:
        return 1;
    case CARD_FILTER_OFFLINE:
        return card->nStatus == CARD_STATUS_OFFLINE;
    case CARD_FILTER_ONLINE:
        return card->nStatus == CARD_STATUS_ONLINE;
    case CARD_FILTER_CANCELED:
        return card->nStatus == CARD_STATUS_CANCELED;
    case CARD_FILTER_LOW_BALANCE:
        return card->nBalanceCent < option->lowBalanceLimitCent;
    default:
        return 0;
    }
}

static int compareInt32Asc(int32_t left, int32_t right)
{
    if (left < right) {
        return -1;
    }
    if (left > right) {
        return 1;
    }
    return 0;
}

static int compareIntDesc(int left, int right)
{
    if (left > right) {
        return -1;
    }
    if (left < right) {
        return 1;
    }
    return 0;
}

static int compareTimeDesc(time_t left, time_t right)
{
    if (left > right) {
        return -1;
    }
    if (left < right) {
        return 1;
    }
    return 0;
}

static int compareCardNameAsc(const Card *left, const Card *right)
{
    return strcmp(left->aCardName, right->aCardName);
}

static int compareCardForSort(const void *a, const void *b)
{
    const Card *left = (const Card *)a;
    const Card *right = (const Card *)b;
    int result = 0;

    switch (g_sortType) {
    case CARD_SORT_BALANCE_ASC:
        result = compareInt32Asc(left->nBalanceCent, right->nBalanceCent);
        break;
    case CARD_SORT_BALANCE_DESC:
        result = -compareInt32Asc(left->nBalanceCent, right->nBalanceCent);
        break;
    case CARD_SORT_USE_COUNT_DESC:
        result = compareIntDesc(left->nUseCount, right->nUseCount);
        break;
    case CARD_SORT_TOTAL_USE_DESC:
        result = -compareInt32Asc(left->nTotalUseCent, right->nTotalUseCent);
        break;
    case CARD_SORT_LAST_USE_DESC:
        result = compareTimeDesc(left->tLast, right->tLast);
        break;
    default:
        result = 0;
        break;
    }

    if (result != 0) {
        return result;
    }
    return compareCardNameAsc(left, right);
}

static BizResult normalizeAdvancedQueryOption(const CardQueryOption *input, CardQueryOption *output)
{
    if (input == NULL || output == NULL) {
        return BIZ_ERR_SYSTEM;
    }

    *output = *input;

    if (!isValidFilterType(output->filterType) || !isValidSortType(output->sortType)) {
        return BIZ_ERR_SYSTEM;
    }

    if (output->pageSize == 0) {
        output->pageSize = CARD_QUERY_DEFAULT_PAGE_SIZE;
    }
    if (output->pageSize > CARD_QUERY_MAX_PAGE_SIZE) {
        output->pageSize = CARD_QUERY_MAX_PAGE_SIZE;
    }

    if (output->filterType == CARD_FILTER_LOW_BALANCE && output->lowBalanceLimitCent <= 0) {
        return BIZ_ERR_INVALID_AMOUNT;
    }

    return BIZ_OK;
}

BizResult bizQueryCardsAdvanced(const CardQueryOption *option, CardQueryPage *page)
{
    CardQueryOption normalizedOption;
    Card *allCards = NULL;
    Card *matchedCards = NULL;
    size_t allCount = 0;
    size_t matchedCount = 0;
    size_t index = 0;
    size_t start = 0;
    size_t end = 0;
    size_t pageItemCount = 0;
    int loadResult = 0;
    BizResult result = BIZ_OK;

    if (page == NULL) {
        return BIZ_ERR_SYSTEM;
    }

    memset(page, 0, sizeof(*page));

    result = normalizeAdvancedQueryOption(option, &normalizedOption);
    if (result != BIZ_OK) {
        return result;
    }

    loadResult = dataLoadCards();
    if (loadResult < 0) {
        return mapAdvancedQueryDataResult((DataResult)loadResult);
    }
    allCount = (size_t)loadResult;
    if (allCount == 0) {
        return BIZ_ERR_NO_MATCHED_CARD;
    }

    allCards = (Card *)malloc(allCount * sizeof(Card));
    matchedCards = (Card *)malloc(allCount * sizeof(Card));
    if (allCards == NULL || matchedCards == NULL) {
        free(allCards);
        free(matchedCards);
        return BIZ_ERR_NO_MEMORY;
    }

    result = bizQueryCardsByKeyword("a", allCards, allCount, &index, &matchedCount);
    if (result != BIZ_OK && result != BIZ_ERR_NO_MATCHED_CARD) {
        free(allCards);
        free(matchedCards);
        return result;
    }

    matchedCount = 0;
    for (index = 0; index < allCount; index++) {
        const Card *card = NULL;
        card = dataQueryCardByName(allCards[index].aCardName);
        if (card != NULL && isCardMatchedByOption(card, &normalizedOption)) {
            matchedCards[matchedCount] = *card;
            matchedCount++;
        }
    }

    if (matchedCount == 0) {
        free(allCards);
        free(matchedCards);
        return BIZ_ERR_NO_MATCHED_CARD;
    }

    g_sortType = normalizedOption.sortType;
    qsort(matchedCards, matchedCount, sizeof(Card), compareCardForSort);

    page->totalCount = matchedCount;
    page->pageSize = normalizedOption.pageSize;
    page->pageCount = (matchedCount + normalizedOption.pageSize - 1) / normalizedOption.pageSize;
    page->pageIndex = normalizedOption.pageIndex;
    if (page->pageIndex >= page->pageCount) {
        page->pageIndex = page->pageCount - 1;
    }

    start = page->pageIndex * page->pageSize;
    end = start + page->pageSize;
    if (end > matchedCount) {
        end = matchedCount;
    }
    pageItemCount = end - start;

    page->items = (Card *)malloc(pageItemCount * sizeof(Card));
    if (page->items == NULL) {
        free(allCards);
        free(matchedCards);
        memset(page, 0, sizeof(*page));
        return BIZ_ERR_NO_MEMORY;
    }

    memcpy(page->items, matchedCards + start, pageItemCount * sizeof(Card));
    page->itemCount = pageItemCount;

    free(allCards);
    free(matchedCards);
    return BIZ_OK;
}

void bizFreeCardQueryPage(CardQueryPage *page)
{
    if (page == NULL) {
        return;
    }

    free(page->items);
    page->items = NULL;
    page->itemCount = 0;
    page->totalCount = 0;
    page->pageIndex = 0;
    page->pageSize = 0;
    page->pageCount = 0;
}
