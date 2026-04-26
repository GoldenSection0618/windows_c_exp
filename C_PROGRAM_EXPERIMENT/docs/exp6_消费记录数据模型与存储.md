# 实验六：消费记录数据模型与存储

## 1. 本轮目标
实验六迭代 1 只补齐消费记录基础设施，不实现上机、下机业务。

本轮完成后，项目需要具备以下能力：
- 定义消费记录 `Billing` 的内存载体
- 使用文本文件持久化消费记录
- 读取全部消费记录到链表
- 追加一条消费记录
- 按卡号查找最近一条未结算消费记录
- 更新指定消费记录
- 释放消费记录链表

## 2. 当前数据模型
项目继续复用 [`model.h`](/home/user/c_program_experiment/include/model.h) 中已有的 `Billing` 结构：
- `aCardName`：卡号
- `tStart`：上机时间
- `tEnd`：下机时间
- `nAmountCent`：消费金额，单位为分
- `nStatus`：结算状态，约定 `0=未结算`、`1=已结算`
- `nDel`：删除标识

内存载体使用链表结点 [`BillingNode`](/home/user/c_program_experiment/include/billing_storage.h)，与卡信息链表保持一致的组织方式。

## 3. 文件存储方案
消费记录文件固定为：
- `data/billings.txt`

文件格式为一行一条记录，字段顺序固定：
```text
卡号|上机时间|下机时间|消费金额分|结算状态|删除标识
```

时间字段规则：
- 正常时间：`YYYY-MM-DD HH:MM:SS`
- 未下机记录的 `tEnd`：保存为 `0`

示例：
```text
card001|2026-03-28 10:00:00|0|0|0|0
card001|2026-03-27 09:00:00|2026-03-27 11:00:00|600|1|0
```

## 4. 数据层接口
### 4.1 链表/仓储接口
见 [`billing_repository.h`](/home/user/c_program_experiment/include/billing_repository.h)：
- `dataAddBilling()`：向当前消费记录链表追加结点
- `dataQueryLatestUnsettledBillingByCardName()`：按卡号查找最近一条未结算记录
- `dataCleanupBillings()`：释放当前消费记录链表

### 4.2 文件持久化接口
见 [`billing_storage_file.h`](/home/user/c_program_experiment/include/billing_storage_file.h)：
- `dataSaveBilling()`：追加保存一条消费记录
- `dataLoadBillings()`：读取全部消费记录并重建链表
- `dataGetBillingCount()`：统计消费记录文件条数
- `dataUpdateBilling()`：按“卡号 + 上机时间”定位并更新记录

## 5. 本轮结论
实验六迭代 1 的消费记录基础设施已经具备：
- 可以单独创建一条未结算消费记录并成功保存
- 可以清空内存后重新从文件读取
- 可以按卡号查询最近一条未结算记录
- 可以更新已存在的消费记录并重写文件

因此下一轮可以直接进入：
1. 上机业务主流程
2. 追加未结算消费记录
3. 更新卡状态为“正在上机”
