# exp4 测试套件说明

## 1. 验收套件
- `repository_delete_api`：链表删除操作底层验证，覆盖空链表、找不到目标、删除头结点、删除中间结点、删除尾结点，以及删除后继续插入的场景

## 2. 运行方式
```bash
make test LAB=exp4 TEST_SUITE=repository_delete_api
make test-batch LAB=exp4 TEST_GROUP=acceptance
```

当前实验四测试聚焦“纯链表操作”本身，不涉及文件持久化业务。
