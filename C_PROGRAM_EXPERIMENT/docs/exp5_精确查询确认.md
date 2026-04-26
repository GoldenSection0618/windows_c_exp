# 实验五：精确查询确认

## 1. 本轮目标
本轮对应 Iteration 5.5，目标不是重写查询逻辑，而是正式确认：当前项目已经具备“文件 -> 链表 -> 精确匹配”的查询主路径，实验五后续可以直接在这条主线上继续扩展。

## 2. 当前精确查询主线
当前仓库中的精确查询职责已经明确分层：
- `dataLoadCards()`：负责从 `data/cards.txt` 读取全部卡记录，并恢复到链表
- `bizQueryCard()`：负责输入规范化、触发链表恢复、精确查找并返回业务结果
- `dataQueryCardByName()`：负责在链表中逐节点进行卡号精确匹配
- `viewShowQueryCardDetails()`：负责以列表形式输出查询结果

## 3. 当前查询流程
当前菜单 2 已按以下顺序工作：
1. 读取用户输入卡号
2. `dataLoadCards()` 清空旧链表并从文件重建
3. `dataQueryCardByName()` 做逐节点精确比较
4. 找到则按列表格式展示卡信息
5. 找不到则输出“没有该卡的信息！”

因此，当前查询已经不是“临时内存查找”，而是标准的“文件恢复 -> 链表查找 -> 控制台展示”流程。

## 4. 为什么可以判定精确查询已完成
判断依据不是函数名，而是完整行为已经满足实验五要求：
- 查询前会先从文件恢复链表
- 查询时按完整卡号逐节点精确匹配
- 查询结果已能列表展示
- 未找到时有明确提示

这说明实验五要求的“精确查询”主路径已经成立，不需要再重写底层查询实现。

## 5. 实验五专属验证
本轮新增实验五测试套件：
- [tests/exp5/menu_query_card_exact_found.input](../tests/exp5/menu_query_card_exact_found.input)
- [tests/exp5/menu_query_card_exact_found.expect.tsv](../tests/exp5/menu_query_card_exact_found.expect.tsv)
- [tests/exp5/menu_query_card_exact_missing.input](../tests/exp5/menu_query_card_exact_missing.input)
- [tests/exp5/menu_query_card_exact_missing.expect.tsv](../tests/exp5/menu_query_card_exact_missing.expect.tsv)

这两组测试分别验证：
1. 精确卡号存在时可成功查到并输出结果列表
2. 精确卡号不存在时会给出明确提示

## 6. 本轮结论
Iteration 5.5 的结论是：
- 当前精确查询已经完成
- 实验五后续不需要再重做精确查询主路径
- 5.6 可以把重点放到模糊查询扩展
