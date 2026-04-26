# 实验五：文件读写与链表打通确认

## 1. 本轮目标
本轮对应 Iteration 5.4，目标不是重新实现一次文件持久化，而是正式确认：当前项目已经具备“文件读写与链表主路径打通”的能力，实验五后续可以直接复用该主线。

## 2. 当前打通主线
当前仓库中的文件与链表协同关系已经成立：
- `dataLoadCards()`：负责从 `data/cards.txt` 读取全部卡记录，并逐条恢复到链表
- `dataSaveCard()`：负责将单张卡记录追加写入文件
- `dataUpdateCard()`：负责修改链表节点后重写文件
- `praseCard()` / `stringToTime()`：负责把文本记录恢复为 `Card`

## 3. 当前业务如何使用这条主线
### 3.1 添加卡
当前 `bizAddCard` 已按以下顺序工作：
1. 输入标准化与合法性校验
2. `dataLoadCards()` 从文件恢复已有卡数据到链表
3. `dataCardExists()` 做重复卡号校验
4. 构造完整 `Card`
5. `dataAddCard()` 把新卡插入当前链表
6. `dataSaveCard()` 把新卡写入文件
7. 若落盘失败，则通过再次 `dataLoadCards()` 回滚内存状态

### 3.2 查询卡
当前 `bizQueryCard` 已按以下顺序工作：
1. `dataLoadCards()` 从文件恢复链表
2. `dataQueryCardByName()` 在链表中做精确查找
3. 返回结果给表示层输出

因此，当前系统已经满足“文件 -> 链表 -> 业务使用”的实验五前置要求。

## 4. 为什么可以判定已经打通
判断依据不是函数名，而是完整数据流已经存在：
- 文件中已有数据时，程序可重启后重新查询到同一卡
- 添加卡后，数据不仅存在于当前内存链表，也已经落盘
- 查询前不是直接依赖临时内存，而是重新从文件恢复链表后再查找

这说明当前运行态权威容器是链表，持久化介质是文件，二者之间的恢复与同步链路已经成立。

## 5. 实验五专属验证
本轮新增实验五测试套件：
- [tests/exp5/menu_persist_linked_flow.input](../tests/exp5/menu_persist_linked_flow.input)
- [tests/exp5/menu_persist_linked_flow.expect.tsv](../tests/exp5/menu_persist_linked_flow.expect.tsv)
- [tests/exp5/menu_persist_linked_flow.verify.sh](../tests/exp5/menu_persist_linked_flow.verify.sh)

该测试验证：
1. 第一次运行通过菜单 1 添加卡并退出
2. 第二次运行通过菜单 2 查询同一卡号
3. 查询结果能正常显示，证明程序重启后仍可从文件恢复链表
4. 通过 `dataLoadCards() + dataQueryCardByName()` 再次校验文件恢复结果正确

## 6. 本轮结论
Iteration 5.4 的结论是：
- 当前文件读写与链表主路径已经打通
- 实验五后续不需要再重做底层持久化能力
- 5.5 可以把重点放到“精确查询作为实验五能力收口”
- 5.6 再进入模糊查询扩展
