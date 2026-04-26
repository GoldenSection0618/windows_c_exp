# 实验六：下机业务主流程

## 1. 本轮目标
实验六迭代 3 补齐菜单 `4. 下机` 的真实业务闭环。

完成后系统具备以下能力：
- 输入卡号和密码执行下机
- 查找该卡最近一条未结算消费记录
- 按统一默认费率完成结算
- 同时更新卡文件与消费记录文件
- 余额不足时仍允许下机，并形成欠费余额

## 2. 下机前置条件
下机业务要求同时满足：
- 卡存在
- 卡未删除
- 密码正确
- 卡状态为 `CARD_STATUS_ONLINE`
- 存在一条该卡最近的未结算消费记录

若任一条件不满足，则下机失败，不写入新状态。

## 3. 最近未结算记录的选择策略
当前数据层复用：
- [`dataQueryLatestUnsettledBillingByCardName()`](/home/user/c_program_experiment/include/billing_repository.h)

选择规则为：
- 仅考虑 `nDel == 0`
- 仅考虑 `nStatus == 0` 的未结算记录
- 同一卡号下取 `tStart` 最大的一条，作为最近未结算记录

## 4. 计费规则
本轮新增独立计费模块：
- [`billing_rule.h`](/home/user/c_program_experiment/include/billing_rule.h)
- [`billing_rule.c`](/home/user/c_program_experiment/src/business/billing_rule.c)

当前默认费率：
- `Rate.unit = 1` 分钟
- `Rate.nChargeCent = 1` 分/分钟

结算公式：
- 时长分钟数 = `max(1, ceil((tEnd - tStart) / 60.0))`
- 消费金额分 = `时长分钟数 * nChargeCent`

因此：
- 不足 1 分钟按 1 分钟计费
- 超过整分钟的部分向上取整

## 5. 双写一致性处理
下机成功时需要同时更新两类数据：
1. 卡文件 `data/cards.txt`
2. 消费记录文件 `data/billings.txt`

更新顺序固定为：
1. 先更新卡状态与余额：`dataUpdateCard()`
2. 再更新消费记录：`dataUpdateBilling()`

若第二步失败，则立即使用原卡快照回滚第一步。

这样可以避免出现：
- 卡已显示“未上机”
- 但消费记录仍是“未结算”

的脏状态。

## 6. 余额充足与不足两类结果
### 6.1 余额充足
成功执行：
- 卡状态从 `CARD_STATUS_ONLINE` 改回 `CARD_STATUS_OFFLINE`
- `nBalanceCent` 扣减本次消费
- `nTotalUseCent` 累加本次消费
- `nUseCount` 加 1
- `tLast` 更新为本次下机时间
- 对应消费记录写入：
  - `tEnd`
  - `nAmountCent`
  - `nStatus = 1`

### 6.2 余额不足
仍允许完成下机：
- 卡状态改回 `CARD_STATUS_OFFLINE`
- 余额允许被扣成负数，表示当前已欠费
- 累计消费、使用次数、最后使用时间照常更新
- 消费记录照常改为已结算

## 7. 本轮结论
实验六迭代 3 已补齐菜单 4 的真实下机主流程。

当前实验六已完成：
- 消费记录数据模型与文本持久化
- 上机业务主流程
- 下机业务主流程

后续迭代可以继续进入：
- 统计视图
- 充值/退费与计费联动
- 更复杂的费率配置来源
