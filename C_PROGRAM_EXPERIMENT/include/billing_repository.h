/*
 * 文件：billing_repository.h
 * 作用：声明计费记录数据层接口，负责新增计费记录、查询未结算记录和释放缓存。
 * 说明：查询接口返回 repository 内部 Billing 指针，调用方需要及时拷贝，不要长期持有。
 */
#ifndef BILLING_REPOSITORY_H
#define BILLING_REPOSITORY_H

#include "card_repository.h"
#include "billing_query_repository.h"

/* 新增一条计费记录并写入计费记录文件。 */
int dataAddBilling(const Billing *billing);

/* 查询指定卡最近一条未结算计费记录，返回内部缓存指针。 */
const Billing *dataQueryLatestUnsettledBillingByCardName(const char *cardName);

/* 释放计费记录 repository 内部链表缓存。 */
void dataCleanupBillings(void);

#endif
