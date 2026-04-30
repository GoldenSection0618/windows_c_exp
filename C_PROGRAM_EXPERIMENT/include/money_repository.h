/*
 * 文件：money_repository.h
 * 作用：声明充值退费流水数据层接口，负责新增流水、查询最近流水和释放缓存。
 * 说明：查询接口返回 repository 内部 Money 指针，调用方需要及时拷贝，不要长期持有。
 */
#ifndef MONEY_REPOSITORY_H
#define MONEY_REPOSITORY_H

#include "card_repository.h"
#include "money_storage.h"

/* 新增一条充值或退费流水并写入数据文件。 */
int dataAddMoney(const Money *money);

/* 查询指定卡最近一条充值或退费流水，返回内部缓存指针。 */
const Money *dataQueryLatestMoneyByCardName(const char *cardName);

/* 释放充值退费 repository 内部链表缓存。 */
void dataCleanupMoneys(void);

#endif
