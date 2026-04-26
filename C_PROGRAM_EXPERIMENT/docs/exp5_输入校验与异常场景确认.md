# 实验五：输入校验与异常场景确认

## 1. 本轮目标
本轮对应 Iteration 5.7，目标不是新增新的业务能力，而是把实验五要求的输入校验与异常场景补齐到专属测试中，确保菜单 1、菜单 2 的核心验收路径完整可复现。

## 2. 当前已具备的校验能力
当前项目已经具备以下输入与异常处理能力：
- 添加卡时校验卡号长度和字符集
- 添加卡时校验密码长度和字符集
- 添加卡时校验开卡金额格式与上限
- 添加卡时基于文件恢复后的链表判断重复卡号
- 查询卡时支持精确查询和模糊查询
- 精确查询无结果时给出明确提示
- 模糊查询无结果时给出明确提示

## 3. 本轮补齐的实验五场景
本轮把实验五要求但此前尚未纳入 `exp5` 套件的两类场景补齐：
1. 添加卡：密码超长
2. 添加卡：重复卡号

补齐后，实验五测试覆盖形成闭环：
- 添加卡：密码正常
- 添加卡：密码超长
- 添加卡：重复卡号
- 查询卡：精确查询
- 查询卡：模糊查询

## 4. 新增实验五测试
本轮新增测试套件：
- [tests/exp5/menu_add_card_password_too_long.input](../tests/exp5/menu_add_card_password_too_long.input)
- [tests/exp5/menu_add_card_password_too_long.expect.tsv](../tests/exp5/menu_add_card_password_too_long.expect.tsv)
- [tests/exp5/menu_add_card_duplicate.input](../tests/exp5/menu_add_card_duplicate.input)
- [tests/exp5/menu_add_card_duplicate.expect.tsv](../tests/exp5/menu_add_card_duplicate.expect.tsv)

对应验证目标：
- 密码超长时拒绝添加，并输出密码非法提示
- 已有卡号再次添加时拒绝写入，并输出重复卡号提示

## 5. 本轮结论
Iteration 5.7 的结论是：
- 实验五要求的输入校验与异常场景已经补齐到专属测试
- 现有添加卡、精确查询、模糊查询主路径无需再重构
- 5.8 可以进入验收输出整理阶段
