/*
 * 文件：card_repository.h
 * 作用：声明卡信息数据层接口，负责卡记录的新增、查询、删除和缓存释放。
 * 说明：repository 内部维护链表缓存，查询返回值可能指向内部节点，调用方不要长期持有。
 */
#ifndef CARD_REPOSITORY_H
#define CARD_REPOSITORY_H

#include <stddef.h>

#include "model.h"

/* 数据层统一返回码：用于映射到业务层 BizResult。 */
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

/* 新增卡并写入数据文件，内部会维护 repository 缓存状态。 */
int dataAddCard(const Card *card);

/* 按卡号删除或标记删除卡记录。 */
DataResult dataDeleteCardByName(const char *cardName);

/* 按卡号精确查询，返回 repository 内部 Card 指针。 */
const Card *dataQueryCardByName(const char *cardName);

/* 按关键字模糊查询卡号，buffer 不足时返回 requiredCount。 */
DataResult dataQueryCardsByKeyword(const char *keyword,
                                   Card *outCards,
                                   size_t capacity,
                                   size_t *actualCount,
                                   size_t *requiredCount);

/* 查询全部卡记录，返回结构体拷贝到调用方提供的缓冲区。 */
DataResult dataQueryAllCards(Card *outCards,
                             size_t capacity,
                             size_t *actualCount,
                             size_t *requiredCount);

/* 判断卡号是否已存在。 */
int dataCardExists(const char *cardName);

/* 释放卡 repository 内部链表缓存。 */
void dataCleanup(void);

#endif
