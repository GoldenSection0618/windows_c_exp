# 实验七：注销卡业务主流程

## 1. 功能目标
菜单 `8. 注销卡` 负责完成卡注销闭环：
- 校验卡号和密码
- 检查卡状态是否允许注销
- 若卡内仍有余额，则先按原余额生成一条退费记录
- 将卡状态更新为 `CARD_STATUS_CANCELED`
- 将卡余额清零

成功后展示两列结果：
- 卡号
- 退款金额

## 2. 业务规则
- 仅 `CARD_STATUS_OFFLINE` 的卡允许注销
- `CARD_STATUS_ONLINE` 的卡不能注销
- `CARD_STATUS_CANCELED` 的卡不能重复注销
- 若原余额 `> 0`，同时向 `data/money.txt` 追加一条退费记录
- 若原余额 `== 0`，只更新卡状态，不写 `money.txt`

## 3. 数据写入顺序
1. 先更新 `cards.txt`
2. 若需要退款，再追加写入 `money.txt`
3. 若 `money.txt` 写入失败，立即回滚卡文件更新

这样可以避免出现“卡已注销但退款记录缺失”的脏状态。

## 4. Money 记录约定
注销时若触发退款，追加的 `Money` 记录字段为：
- `aCardName`：卡号
- `tTime`：当前时间
- `nStatus = 1`：退费
- `nMoneyCent = 原余额`
- `nDel = 0`

## 5. 当前阶段约束
- 注销成功后卡保持 `CARD_STATUS_CANCELED`
- 不支持部分退款
- 不修改 `tLast`、`nUseCount`、`nTotalUseCent`
