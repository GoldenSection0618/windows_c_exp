# 实验五：添加卡链表主路径确认

## 1. 本轮目标
本轮对应 Iteration 5.3，目标不是重新把数组版迁移到链表版，而是正式确认：当前项目中的“添加卡”主路径已经是链表实现，结构体数组已经退出主路径。

## 2. 当前添加卡主路径
当前 `bizAddCard` 的业务顺序已经满足实验五对链表版添加卡的要求：
1. 输入标准化与合法性校验
2. `dataLoadCards()` 从文件恢复现有卡数据到链表
3. `dataCardExists()` 做重复卡号校验
4. 构造完整 `Card`
5. `dataAddCard()` 将新卡作为链表结点插入当前链表
6. `dataSaveCard()` 将新卡持久化到文本文件
7. 返回结果给表示层输出

## 3. 为什么可以判定数组版已退出主路径
当前仓库里，添加卡主流程不再依赖：
- 结构体数组下标访问
- 固定容量数组存储
- 基于数组计数的插入逻辑

相反，当前插入动作完全依赖：
- 动态分配结点
- 链表尾插
- 链表查重
- 文件恢复后在链表中继续操作

因此实验五后续不需要再做一次“把添加卡从数组切到链表”的迁移，只需把这一事实通过专用测试和说明收口。

## 4. 实验五专属验证
本轮新增实验五测试套件：
- [tests/exp5/menu_add_card_linked.input](../tests/exp5/menu_add_card_linked.input)
- [tests/exp5/menu_add_card_linked.expect.tsv](../tests/exp5/menu_add_card_linked.expect.tsv)
- [tests/exp5/menu_add_card_linked.verify.sh](../tests/exp5/menu_add_card_linked.verify.sh)

该测试验证：
1. 菜单 1 能正常进入添加卡流程
2. 合法输入下添加卡成功
3. 新卡能通过 `dataLoadCards() + dataQueryCardByName()` 找到
4. 新卡的密码、状态、余额、初始累计使用和使用次数符合当前初始化规则

## 5. 本轮结论
Iteration 5.3 的结论是：
- 当前添加卡主路径已经是链表版
- 数组版不再是当前业务主实现
- 后续 5.4 应继续收口“文件读写与链表打通”的实验五叙述与验证
- 当前不需要重构 `bizAddCard` 本身
