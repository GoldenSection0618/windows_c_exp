# 实验二：数据结构设计与卡管理（链表实现）

## 1. 实验目标
1. 完成计费管理系统核心数据结构设计（`typedef struct`）。
2. 在实验一框架基础上，使用链表实现卡管理的“新增卡、查询卡”。
3. 保持三层结构组织：表示层、业务逻辑层、数据存储层。

## 2. 数据结构设计
本实验在 [model.h](../include/model.h) 中定义以下类型：

1. `Card`：卡信息（卡号、密码、状态、开卡时间、截止时间、累计使用、最后使用、次数、余额、删除标识）
2. `Billing`：消费记录
3. `LogonInfo`：上机信息
4. `SettleInfo`：下机结算信息
5. `Money`：充值/退费记录
6. `Rate`：计费标准
7. `Admin`：管理员信息

当前实现说明：
- 时间字段使用 `time_t`；
- 状态码包含：`0未上机/1正在上机/2已注销/3失效`；
- 金额字段已改为“分”为单位的 `int32_t` 整型存储，例如 `nBalanceCent`、`nTotalUseCent`；
- 卡内余额必须小于 `1000000.00` 元。

## 3. 链表存储方案
数据存储层使用内存链表：

- 头指针：`CardNode *g_pCardListHead`
- 私有计数：`size_t g_cardCount`
- 节点类型定义见 [card_storage.h](../include/card_storage.h)

数据层公共接口见 [data.h](../include/data.h)：
- `dataAddCard`
- `dataQueryCardByName`
- `dataCleanup`

链表存储实现见 [repository.c](../src/data/repository.c)。

## 4. 当前实现分层
- 表示层：
  [main.c](../src/main.c)、[menu.c](../src/presentation/menu.c)、[card_view.c](../src/presentation/card_view.c)
- 业务逻辑层：
  [billing_service.c](../src/business/billing_service.c)、[operation_log.c](../src/business/operation_log.c)
- 数据存储层：
  [repository.c](../src/data/repository.c)

## 5. 已实现功能
### 5.1 菜单 1：新增卡
业务实现位置：[billing_service.c](../src/business/billing_service.c)
显示实现位置：[card_view.c](../src/presentation/card_view.c)

已覆盖：
1. 输入卡号（1\~18位）/密码（1\~8位）/开卡金额；
2. 卡号重复校验；
3. 密码长度校验（含超长拦截）；
4. 开卡金额合法性校验；
5. 余额上限校验：必须小于 `1000000.00` 元；
6. 新卡字段初始化（状态、时间、余额、次数、累计金额、删除标识）；
7. 成功后表格化显示（卡号/密码/状态/余额）。

### 5.2 菜单 2：查询卡
业务实现位置：[billing_service.c](../src/business/billing_service.c)
显示实现位置：[card_view.c](../src/presentation/card_view.c)

已覆盖：
1. 按卡号查找；
2. 存在时显示：卡号、卡状态、余额、累计使用、使用次数、最后使用时间；
3. 不存在时提示“没有该卡的信息！”。

## 6. 测试用例
默认实验二单用例测试：
- 输入：`tests/exp2/menu.input`
- 断言：`tests/exp2/menu.expect.tsv`
- 执行：`make test`

实验二整套测试：
- 验收：`make test-batch LAB=exp2 TEST_GROUP=acceptance`
- 回归：`make test-batch LAB=exp2 TEST_GROUP=regression`

当前已覆盖：
1. 菜单 1/2 可运行；
2. 新增卡正常；
3. 密码超长拦截；
4. 重复卡号拦截；
5. 卡号为空/超长拦截；
6. 金额非法拦截；
7. 金额过大拦截；
8. 查询存在/不存在两种场景；
9. 输入污染与空白输入回归场景。
