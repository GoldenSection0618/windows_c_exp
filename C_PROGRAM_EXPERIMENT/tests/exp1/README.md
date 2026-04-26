# exp1 测试套件说明

## 1. 套件
- `menu`：实验一主菜单交互测试（1~8、9、1w、ttt、0）

## 2. 运行方式
单套件：
```bash
make test LAB=exp1 TEST_SUITE=menu
```

批量（统一接口）：
```bash
make test-batch LAB=exp1 TEST_GROUP=acceptance
make test-batch LAB=exp1 TEST_GROUP=regression
```
