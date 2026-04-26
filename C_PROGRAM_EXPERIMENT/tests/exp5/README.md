# exp5 测试套件说明

## 1. 验收套件
- `menu_add_card_password_too_long`：实验五添加卡异常验证，覆盖密码超长场景
- `menu_add_card_duplicate`：实验五添加卡异常验证，覆盖重复卡号场景
- `menu_query_card_fuzzy_found`：实验五模糊查询命中验证，覆盖菜单 2 模糊查询多结果场景
- `menu_query_card_fuzzy_missing`：实验五模糊查询未命中验证，覆盖菜单 2 模糊查询无结果场景
- `menu_query_card_exact_found`：实验五精确查询命中验证，覆盖菜单 2 精确查询存在卡号场景
- `menu_query_card_exact_missing`：实验五精确查询未命中验证，覆盖菜单 2 查询不存在卡号场景
- `menu_persist_linked_flow`：实验五文件-链表打通验证，覆盖首次添加、程序重启后查询、文件恢复后链表查找
- `menu_add_card_linked`：实验五链表版添加卡验证，覆盖菜单 1 正常添加、链表插入与文件恢复后查找
- `repository_list_foundation`：实验五链表基础设施验证，覆盖空链表、首结点插入、多结点尾插、重复卡号拦截、删除头/中/尾结点，以及 `dataCleanup()` 后重建场景

## 2. 运行方式
```bash
make test LAB=exp5 TEST_SUITE=repository_list_foundation
make test LAB=exp5 TEST_SUITE=menu_add_card_linked
make test LAB=exp5 TEST_SUITE=menu_add_card_password_too_long
make test LAB=exp5 TEST_SUITE=menu_add_card_duplicate
make test LAB=exp5 TEST_SUITE=menu_persist_linked_flow
make test LAB=exp5 TEST_SUITE=menu_query_card_exact_found
make test LAB=exp5 TEST_SUITE=menu_query_card_exact_missing
make test LAB=exp5 TEST_SUITE=menu_query_card_fuzzy_found
make test LAB=exp5 TEST_SUITE=menu_query_card_fuzzy_missing
make test-batch LAB=exp5 TEST_GROUP=acceptance
```

当前 5.7 在 5.6 基础上补齐实验五要求的输入校验与异常场景，不额外修改现有业务主路径。
