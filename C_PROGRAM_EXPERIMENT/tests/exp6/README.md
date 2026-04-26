# exp6 测试套件说明

## 1. 验收套件
- `billing_repository_api`：实验六消费记录基础设施验证，覆盖保存、读取、查找未结算记录、更新记录、程序重启后重载
- `menu_start_billing_success`：实验六上机成功验证，覆盖菜单 3、卡状态更新、未结算消费记录追加与结果展示
- `menu_start_billing_card_missing`：实验六上机失败验证，覆盖卡不存在场景
- `menu_start_billing_password_wrong`：实验六上机失败验证，覆盖密码错误场景
- `menu_start_billing_card_unavailable`：实验六上机失败验证，覆盖卡正在上机场景
- `menu_start_billing_card_canceled`：实验六上机失败验证，覆盖卡已注销场景
- `menu_start_billing_balance_not_enough`：实验六上机失败验证，覆盖余额不足场景
- `menu_start_billing_card_file_empty`：实验六上机失败验证，覆盖卡文件为空场景
- `menu_start_billing_billing_save_fail`：实验六上机失败验证，覆盖消费记录写入失败与卡状态回滚
- `menu_stop_billing_success`：实验六下机成功验证，覆盖菜单 4、卡状态恢复、余额扣减与消费记录结算
- `menu_stop_billing_no_unsettled`：实验六下机失败验证，覆盖未找到未结算记录场景
- `menu_stop_billing_balance_not_enough`：实验六下机验证，覆盖允许欠费下机并形成负余额场景
- `menu_stop_billing_password_wrong`：实验六下机失败验证，覆盖密码错误场景
- `menu_stop_billing_billing_file_empty`：实验六下机失败验证，覆盖消费记录文件为空场景
- `menu_stop_billing_multiple_unsettled`：实验六脏数据验证，覆盖同一卡多条未结算记录时优先结算最近一条
- `menu_stop_billing_billing_update_fail`：实验六下机失败验证，覆盖消费记录更新失败与卡状态回滚

## 2. 运行方式
```bash
make test LAB=exp6 TEST_SUITE=billing_repository_api
make test LAB=exp6 TEST_SUITE=menu_start_billing_success
make test LAB=exp6 TEST_SUITE=menu_start_billing_card_missing
make test LAB=exp6 TEST_SUITE=menu_start_billing_password_wrong
make test LAB=exp6 TEST_SUITE=menu_start_billing_card_unavailable
make test LAB=exp6 TEST_SUITE=menu_start_billing_card_canceled
make test LAB=exp6 TEST_SUITE=menu_start_billing_balance_not_enough
make test LAB=exp6 TEST_SUITE=menu_start_billing_card_file_empty
make test LAB=exp6 TEST_SUITE=menu_start_billing_billing_save_fail
make test LAB=exp6 TEST_SUITE=menu_stop_billing_success
make test LAB=exp6 TEST_SUITE=menu_stop_billing_no_unsettled
make test LAB=exp6 TEST_SUITE=menu_stop_billing_balance_not_enough
make test LAB=exp6 TEST_SUITE=menu_stop_billing_password_wrong
make test LAB=exp6 TEST_SUITE=menu_stop_billing_billing_file_empty
make test LAB=exp6 TEST_SUITE=menu_stop_billing_multiple_unsettled
make test LAB=exp6 TEST_SUITE=menu_stop_billing_billing_update_fail
make test-batch LAB=exp6 TEST_GROUP=acceptance
```

当前迭代 5 在上机、下机主流程基础上，进一步统一了结果展示和失败提示。


## 3. 输出与提示约定
- 上机失败先输出 `上机失败！`，再输出细化原因
- 下机失败先输出 `下机失败！`，再输出细化原因
- 下机成功结果表格中的余额列统一标记为 `结算后余额`
