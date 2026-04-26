# 实验七：按卡号查询消费记录

## 1. 功能目标
- 从菜单 `7. 查询统计` 进入
- 当前仅实现一个子功能：`1. 按卡号查询消费记录`
- 支持两种查询模式：
  - `1. 查询全部`
  - `2. 限定时间段`

## 2. 查询规则
- 卡号按现有卡号规则校验
- 查询全部：
  - 返回该卡号全部未删除消费记录
- 限定时间段：
  - 仍先按卡号匹配
  - 再按 `tStart` 过滤
  - 过滤条件为：`startTime <= tStart <= endTime`
- 若 `billings.txt` 不存在、为空，或没有命中记录，则统一提示：
  - `没有找到符合条件的消费记录！`

## 3. 结果展示
- 列表字段固定为：
  - 卡号
  - 上机时间
  - 下机时间
  - 消费金额
  - 结算状态
- `tEnd == 0` 时，下机时间显示 `--`
- 结算状态：
  - `0 -> 未结算`
  - `1 -> 已结算`

## 4. 实现分层
- 数据层：
  - `dataQueryBillingsByCardName(...)`
  - `dataQueryBillingsByCardNameAndRange(...)`
- 业务层：
  - `bizQueryBillingsByCardName(...)`
  - `bizQueryBillingsByCardNameAndRange(...)`
  - `bizFreeBillingQueryResult(...)`
- 表示层：
  - `handleBillingQueryInteraction()`
  - `viewShowBillingRecords(...)`

## 5. 当前范围
- 这是实验七的可选扩展
- 当前只实现“按卡号查询消费记录”
- 暂不实现：
  - 营业额统计
  - 月营业额统计
