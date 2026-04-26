# 实验七：充值退费记录数据层

## 1. 目标
本阶段只补 `Money` 数据层，不实现菜单 5、6、8 的业务流程。

## 2. 数据模型
`Money` 字段语义如下：
- `aCardName`：卡号
- `tTime`：记录时间
- `nStatus`：记录类型，`0=充值`、`1=退费`
- `nMoneyCent`：金额，单位为分
- `nDel`：删除标识，当前阶段默认写 `0`

## 3. 存储格式
记录文件路径：`data/money.txt`

一行一条记录，固定格式：
```text
卡号|时间|状态|金额分|删除标识
```

时间格式固定为：
```text
YYYY-MM-DD HH:MM:SS
```

## 4. 数据层能力
- `dataAddMoney()`：追加到内存链表
- `dataSaveMoney()`：追加写入 `data/money.txt`
- `dataLoadMoneys()`：从文件读取全部记录并重建链表
- `dataGetMoneyCount()`：统计文件中的记录数
- `dataQueryLatestMoneyByCardName()`：查询某张卡最新一条充值/退费记录
- `dataCleanupMoneys()`：释放消费记录链表

## 5. 当前阶段边界
- 只做充值退费记录的数据层
- 不接入菜单 5、6、8
- 不修改已有卡文件和消费记录文件格式
