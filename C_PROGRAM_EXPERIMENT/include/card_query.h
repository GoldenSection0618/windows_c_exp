/*
 * 文件：card_query.h
 * 作用：声明管理员高级查询的筛选、排序选项和查询 API。
 * 说明：高级查询返回动态分配的结果数组，调用者必须使用 bizFreeCardQueryResult 释放。
 */
#ifndef CARD_QUERY_H
#define CARD_QUERY_H

#include <stddef.h>
#include <stdint.h>

#include "business.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 高级查询筛选类型：用于限定卡状态或低余额范围。 */
typedef enum CardQueryFilterType {
    CARD_QUERY_FILTER_ALL = 1,
    CARD_QUERY_FILTER_OFFLINE = 2,
    CARD_QUERY_FILTER_ONLINE = 3,
    CARD_QUERY_FILTER_CANCELED = 4,
    CARD_QUERY_FILTER_LOW_BALANCE = 5
} CardQueryFilterType;

/* 高级查询排序类型：用于控制返回列表顺序。 */
typedef enum CardQuerySortType {
    CARD_QUERY_SORT_BALANCE_ASC = 1,
    CARD_QUERY_SORT_BALANCE_DESC = 2,
    CARD_QUERY_SORT_USE_COUNT_DESC = 3,
    CARD_QUERY_SORT_TOTAL_USE_DESC = 4,
    CARD_QUERY_SORT_LAST_USE_DESC = 5
} CardQuerySortType;

/* 高级查询条件：低余额筛选时 lowBalanceLimitCent 才参与过滤。 */
typedef struct CardQueryOption {
    CardQueryFilterType filterType; /* 筛选条件 */
    CardQuerySortType sortType;     /* 排序方式 */
    int32_t lowBalanceLimitCent;    /* 低余额阈值，单位：分 */
} CardQueryOption;

/*
 * 功能：执行高级查卡，不校验管理员 session。
 * 参数：outCards 返回动态分配的 Card 数组，outCount 返回数量。
 * 返回：BIZ_OK 表示成功，调用者之后需释放 outCards。
 */
BizResult bizQueryCardsAdvanced(const CardQueryOption *option, Card **outCards, size_t *outCount);

/* 管理员高级查卡：先校验管理员 session，再执行筛选和排序。 */
BizResult bizAdminQueryCardsAdvanced(const LoginSession *session,
                                     const CardQueryOption *option,
                                     Card **outCards,
                                     size_t *outCount);

/* 释放高级查询返回的 Card 数组。 */
void bizFreeCardQueryResult(Card *cards);

/* 返回筛选类型的中文显示文本。 */
const char *bizGetCardQueryFilterText(CardQueryFilterType filterType);

/* 返回排序类型的中文显示文本。 */
const char *bizGetCardQuerySortText(CardQuerySortType sortType);

#ifdef __cplusplus
}
#endif

#endif
