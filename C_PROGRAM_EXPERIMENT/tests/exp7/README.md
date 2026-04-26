# exp7 测试套件说明

## 1. 验收套件
- `money_repository_api`：实验七充值退费记录数据层验证，覆盖保存、读取、计数、最近记录查询、缺失文件和空文件处理
- `menu_recharge_success`：离线卡充值成功，验证卡余额更新与充值记录追加
- `menu_recharge_online_success`：正在上机卡也允许充值
- `menu_recharge_card_missing`：卡不存在时充值失败，且不污染卡文件和 money 文件
- `menu_recharge_password_wrong`：密码错误时充值失败，且不污染卡文件和 money 文件
- `menu_recharge_card_canceled`：已注销卡禁止充值
- `menu_recharge_amount_invalid`：非法充值金额被拒绝，且不污染卡文件和 money 文件
- `menu_refund_success`：离线卡退费成功，验证卡余额清零与退费记录追加
- `menu_refund_card_online`：正在上机卡禁止退费
- `menu_refund_balance_zero`：余额为 0 的卡禁止退费
- `menu_refund_card_missing`：卡不存在时退费失败，且不污染卡文件和 money 文件
- `menu_refund_password_wrong`：密码错误时退费失败，且不污染卡文件和 money 文件
- `menu_refund_card_canceled`：已注销卡禁止退费
- `menu_cancel_card_success_with_refund`：离线卡注销成功并退回原余额
- `menu_cancel_card_success_zero_balance`：零余额离线卡注销成功，且不写 money 记录
- `menu_cancel_card_online`：正在上机卡禁止注销
- `menu_cancel_card_already_canceled`：已注销卡禁止重复注销
- `menu_cancel_card_missing`：卡不存在时注销失败，且不污染卡文件和 money 文件
- `menu_cancel_card_password_wrong`：密码错误时注销失败，且不污染卡文件和 money 文件
- `menu_query_billings_all_found`：按卡号查询全部消费记录成功
- `menu_query_billings_all_missing`：按卡号查询消费记录无结果
- `menu_query_billings_range_found`：按卡号和时间段查询消费记录成功
- `menu_query_billings_range_missing`：时间段有效但无命中记录
- `menu_query_billings_range_invalid`：时间范围非法时查询失败

## 2. 运行方式
```bash
make test LAB=exp7 TEST_SUITE=money_repository_api
make test LAB=exp7 TEST_SUITE=menu_recharge_success
make test LAB=exp7 TEST_SUITE=menu_recharge_online_success
make test LAB=exp7 TEST_SUITE=menu_recharge_card_missing
make test LAB=exp7 TEST_SUITE=menu_recharge_password_wrong
make test LAB=exp7 TEST_SUITE=menu_recharge_card_canceled
make test LAB=exp7 TEST_SUITE=menu_recharge_amount_invalid
make test LAB=exp7 TEST_SUITE=menu_refund_success
make test LAB=exp7 TEST_SUITE=menu_refund_card_online
make test LAB=exp7 TEST_SUITE=menu_refund_balance_zero
make test LAB=exp7 TEST_SUITE=menu_refund_card_missing
make test LAB=exp7 TEST_SUITE=menu_refund_password_wrong
make test LAB=exp7 TEST_SUITE=menu_refund_card_canceled
make test LAB=exp7 TEST_SUITE=menu_cancel_card_success_with_refund
make test LAB=exp7 TEST_SUITE=menu_cancel_card_success_zero_balance
make test LAB=exp7 TEST_SUITE=menu_cancel_card_online
make test LAB=exp7 TEST_SUITE=menu_cancel_card_already_canceled
make test LAB=exp7 TEST_SUITE=menu_cancel_card_missing
make test LAB=exp7 TEST_SUITE=menu_cancel_card_password_wrong
make test LAB=exp7 TEST_SUITE=menu_query_billings_all_found
make test LAB=exp7 TEST_SUITE=menu_query_billings_all_missing
make test LAB=exp7 TEST_SUITE=menu_query_billings_range_found
make test LAB=exp7 TEST_SUITE=menu_query_billings_range_missing
make test LAB=exp7 TEST_SUITE=menu_query_billings_range_invalid
make test-batch LAB=exp7 TEST_GROUP=acceptance
```

## 3. 当前阶段说明
- 本阶段已完成 `Money` 数据层、菜单 `5. 充值`、菜单 `6. 退费` 和菜单 `8. 注销卡` 主流程
- 已新增菜单 `7` 下的“按卡号查询消费记录”扩展，支持查询全部和限定时间段
- 记录文件固定为 `data/money.txt`
