# exp2 测试套件说明

## 1. 验收套件
- `menu`：实验二主流程（新增/查询/重复/超长密码/不存在查询）
- `menu_add_valid`：新增卡正常路径
- `menu_add_duplicate`：重复卡号拦截
- `menu_add_password_too_long`：密码超长拦截
- `menu_add_card_empty`：空卡号拦截
- `menu_add_card_too_long`：超长卡号拦截
- `menu_add_amount_invalid`：金额非法（非数字、负数）拦截
- `menu_query_existing_and_missing`：查询存在与不存在
- `menu_invalid_menu_inputs`：菜单输入异常（9/1w/ttt）

## 2. 回归套件
- `menu_regression_long_input`：超长输入后残留污染下一轮菜单读取
- `menu_regression_whitespace`：纯空白卡号/密码被错误当作合法输入

## 3. 运行方式
```bash
make test-batch LAB=exp2 TEST_GROUP=acceptance
make test-batch LAB=exp2 TEST_GROUP=regression
```

也可手动指定清单文件：
```bash
make test-batch LAB=exp2 TEST_SUITES_FILE=tests/exp2/suites.acceptance
```
