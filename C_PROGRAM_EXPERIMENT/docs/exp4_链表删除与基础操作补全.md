# 实验四：链表删除与基础操作补全

## 1. 实验目标
1. 在现有卡信息链表存储基础上，补齐链表删除操作。
2. 将“删除结点”能力沉到数据存储层，保持与具体业务解耦。
3. 验证链表在删除后的结构仍然正确，且结点内存能够被释放。

## 2. 当前实现范围
实验四当前聚焦“纯链表能力”，不把文件持久化或注销卡业务混进核心验收点。

已补齐的数据层接口：
- `dataDeleteCardByName(const char *cardName)`

接口定义位置：
- [data.h](../include/data.h)

实现位置：
- [repository.c](../src/data/repository.c)

## 3. 删除操作设计
链表删除采用单链表常规做法：

1. 使用 `pCurrent` 遍历当前结点；
2. 使用 `pPrev` 记录前驱结点；
3. 找到目标后分两种情况处理：
   - 删除头结点：直接移动头指针；
   - 删除非头结点：令前驱的 `pNext` 指向当前结点的后继；
4. 对被删除结点调用 `free`；
5. 同步维护链表计数 `g_cardCount`。

错误处理策略：
- 卡号参数为 `NULL` 或空串：返回 `DATA_ERR_INVALID_ARG`
- 链表中找不到目标：返回 `DATA_ERR_NOT_FOUND`
- 删除成功：返回 `DATA_OK`

## 4. 实验四测试
实验四测试目录：
- [tests/exp4](../tests/exp4)

当前验收用例：
- `repository_delete_api`

覆盖场景：
1. 空链表删除
2. 删除头结点
3. 删除尾结点
4. 删除中间结点
5. 删除不存在的结点
6. 删除清空后再次插入

运行方式：
```bash
make test LAB=exp4 TEST_SUITE=repository_delete_api
make test-batch LAB=exp4 TEST_GROUP=acceptance
```

## 5. 与实验三的边界
实验三仍然负责“链表 + 文件持久化”的协同验证，包括：
- `readCard`
- `saveCard`
- `updateCard`
- 文件记录格式与时间字段保护

实验四这里不再把删除验证挂在 `exp3` 目录下，以避免实验目标混淆。
