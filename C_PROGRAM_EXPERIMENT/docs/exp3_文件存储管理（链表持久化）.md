# 实验三：文件存储管理（链表持久化）

## 1. 实现目标
本实验在实验二“链表内存存储”的基础上，引入文本文件持久化能力，使卡信息能够：
- 长期保存到 `data/cards.txt`
- 程序再次运行时从文件恢复到链表
- 基于文件最新内容完成添加卡、查询卡
- 提供记录计数、存在性判断、更新单条记录等底层能力

当前项目的实现原则是：
- 链表负责运行时内存组织
- 文本文件负责持久化保存
- 相关卡业务执行前先从文件加载链表
- 修改卡数据时同时保证链表与文件一致

## 2. 分层职责
### 2.1 表示层
- [main.c](../src/main.c)
- [menu.c](../src/presentation/menu.c)
- [card_view.c](../src/presentation/card_view.c)

职责：
- 菜单显示与菜单编号读取
- 添加卡/查询卡输入采集
- 根据业务结果码输出成功或失败信息
- 表格化显示卡信息

### 2.2 业务逻辑层
- [billing_service.c](../src/business/billing_service.c)
- [operation_log.c](../src/business/operation_log.c)

职责：
- 校验卡号、密码、金额输入
- 限制卡号、密码必须匹配字符集 `^[A-Za-z0-9_@#$%!]+$`
- 执行添加卡、查询卡业务流程
- 调用数据层完成文件加载、重复校验、落盘、查询
- 把结果码返回给表示层

### 2.3 数据存储层
- [repository.c](../src/data/repository.c)
- [card_storage.h](../include/card_storage.h)

职责：
- 维护内存链表 `g_pCardListHead`
- 管理私有计数 `g_cardCount`
- 负责卡信息文本文件的读取、解析、写入、重写

## 3. 文本文件格式
卡数据文件固定为：`data/cards.txt`

每张卡一行，字段顺序固定，使用 `|` 分隔：

```text
卡号|密码|状态|开卡时间|截止时间|累计金额分|最后使用时间|使用次数|余额分|删除标识
```

示例：
```text
card001|pass01|0|2026-03-16 16:44:49|2027-03-16 16:44:49|0|2026-03-16 16:44:49|0|10000|0
```

说明：
- 金额字段继续使用“分”为单位的整型存储
- 时间字段统一使用 `YYYY-MM-DD HH:MM:SS`
- 写入顺序与读取解析顺序严格一致
- 卡号和密码只允许大小写字母、数字以及 `_ @ # $ % !`
- 任意不在允许集合内的字符（如 `|`、`-`、空格）都会被业务层直接拒绝
- 空行会被跳过，坏记录会触发错误保护

## 4. 关键函数职责
### 4.1 dataSaveCard(const Card *card)
- 将一条卡记录按文本格式追加写入 `data/cards.txt`
- 添加卡成功后用于落盘
- 文件打开失败时返回错误状态

### 4.2 dataLoadCards(void)
- 先清空当前链表，再从 `data/cards.txt` 逐行读取
- 每条记录调用 `praseCard()` 解析，再装入链表
- 文件不存在时返回“文件不存在”状态
- 空文件返回 `0`，表示成功读取到 0 条记录
- 记录格式或时间解析错误时返回失败状态并清空半成品链表

### 4.3 praseCard(const char *line, Card *outCard)
- 把单行文本拆分为 10 个固定字段
- 完成整数、金额、时间字段的类型转换
- 任一字段缺失或格式错误都会返回失败

### 4.4 stringToTime(const char *text, time_t *outTime)
- 把 `YYYY-MM-DD HH:MM:SS` 转成 `time_t`
- 使用 `mktime` 做合法性校验
- 对非法时间串返回失败状态

### 4.5 bizAddCard(...)
- 表示层传入原始输入字符串
- 业务层完成 trim、长度校验、金额校验
- 调用 `dataLoadCards()` 获取文件最新内容
- 调用 `dataCardExists()` 检查重复卡号
- 构造完整 `Card`
- 先更新链表，再调用 `dataSaveCard()` 落盘
- 若落盘失败，调用 `dataLoadCards()` 回滚内存状态

### 4.6 bizQueryCard(...)
- 表示层传入卡号字符串
- 业务层先调用 `dataLoadCards()` 同步文件数据到链表
- 再通过 `dataQueryCardByName()` 做精确查询
- 查询成功后交给表示层表格显示

### 4.7 其他底层能力
- `dataGetCardCount()`：统计当前文件中的有效卡记录数
- `dataCardExists()`：判断当前加载链表中是否存在卡号
- `dataUpdateCard()`：修改链表节点后重写整个文件，供后续实验复用

## 5. 当前实现如何在链表下完成文件持久化
1. 相关卡业务开始前，先调用 `dataLoadCards()`
2. `dataLoadCards()` 从 `data/cards.txt` 读取文本并恢复链表
3. 添加卡时，先在链表中做重复校验，再将新卡加入链表并调用 `dataSaveCard()` 追加到文件
4. 查询卡时，链表数据来自文件最新内容，再做精确匹配
5. 更新卡时，先改链表节点，再调用 `dataUpdateCard()` 重写整个文件
6. 程序结束时调用 `dataCleanup()` 释放链表

这意味着：
- 链表是运行态缓存与查询载体
- 文本文件是唯一持久化数据源
- 二者通过 `readCard/saveCard/updateCard` 保持一致

## 6. 已完成测试
默认测试：
```bash
make test
```

批量测试：
```bash
make test-batch LAB=exp3 TEST_GROUP=acceptance
make test-batch LAB=exp3 TEST_GROUP=regression
```

当前 `exp3` 已覆盖：
1. 正常添加卡并落盘
2. 重复卡号拒绝添加
3. 密码超长拒绝添加
4. 卡号或密码包含分隔符 `|` 时拒绝添加且文件不被污染
5. 卡号或密码包含不在允许字符集内的字符时拒绝添加且文件不被污染
6. 允许字符集 `A-Za-z0-9_@#$%!` 可正常保存和查询
7. 查询存在的卡
8. 查询不存在的卡
9. 文件不存在时的查询提示
10. 空文件按无记录处理
11. 损坏记录格式保护
12. 非法时间字段保护
13. 程序重启后的持久化查询验证
14. `readCard/getCardCount/isCardExists/updateCard` 的底层协同验证
