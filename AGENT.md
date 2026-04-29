# AGENT.md

本文档面向后续接手本仓库的 AI agent 或开发者，用于快速理解项目结构、核心功能和修改边界。

## 当前重点分支

当前主要开发分支为：`role-login-session-ui`。

除非用户明确要求，否则后续修改应优先基于该分支进行。不要默认改动其他分支。

## 项目定位

本项目是一个 C 语言课程实验项目：计费管理系统。当前已迁移到 Windows / Visual Studio 开发环境，并新增 Win32 GUI 表示层。

项目核心业务是围绕“卡”进行计费管理，支持用户注册、登录、上机、下机、充值、退费、注销卡，以及管理员后台查询、下机、充值、退费、营业额统计等功能。

## 主要功能

### 用户侧功能

用户通过卡号和密码登录。用户功能包括：

- 用户注册：逻辑类似开卡，但默认赠送 100 元余额，不允许用户输入初始金额。
- 用户登录：输入卡号和密码。
- 查询余额：查询当前登录卡的余额和状态。
- 上机：开始一次计费记录。
- 下机：结束当前未结算计费记录，并扣费。
- 充值：对当前登录卡充值。
- 退费：对当前登录卡按指定金额退费。
- 注销卡：需要再次输入卡号和密码确认，注销后退出登录。

### 管理员功能

管理员账号密码固定为：

```text
账号：root
密码：root
```

管理员功能包括：

- 查询卡：只需要卡号，不需要卡密码。
- 下机：只需要卡号，不需要卡密码。
- 充值：只需要卡号和金额，不需要卡密码。
- 退费：只需要卡号和金额，不需要卡密码。
- 高级查询：支持按卡状态筛选（全部/未上机/上机/已注销/低余额）、按多种方式排序（余额升降、使用次数降、累计消费降、最后使用时间降），可选低余额阈值过滤。
- 营业额统计：输入 `YYYY-MM` 格式，按指定月份统计该月营业额。
- 卡数据文件维护：健康检查 `cards.txt`（逐行校验字段/状态/时间/金额/重复/超长）、导出健康报告、手动备份到 `data/backup/`、从备份恢复。

## 工程结构

仓库中主要有两个 Visual Studio 工程：

```text
C_PROGRAM_EXPERIMENT/
AccountManagement_GUI/
```

### C_PROGRAM_EXPERIMENT

这是主工程，包含控制台入口、业务层、数据层、平台适配层和 GUI 代码。

主要目录：

```text
C_PROGRAM_EXPERIMENT/include/
C_PROGRAM_EXPERIMENT/src/
C_PROGRAM_EXPERIMENT/src/business/
C_PROGRAM_EXPERIMENT/src/data/
C_PROGRAM_EXPERIMENT/src/platform/
C_PROGRAM_EXPERIMENT/src/presentation/
C_PROGRAM_EXPERIMENT/src/presentation/gui/
```

### AccountManagement_GUI

这是独立 GUI 工程，复用 `C_PROGRAM_EXPERIMENT` 下的业务层、数据层和 GUI 源码。当前并未抽出单独的 Core static library，因此 GUI 工程会直接编译一批 `C_PROGRAM_EXPERIMENT` 中的源文件。

## 分层说明

### 1. 模型层

核心数据结构在：

```text
C_PROGRAM_EXPERIMENT/include/model.h
```

主要结构包括：

- `Card`：卡信息。
- `Billing`：计费记录。
- `Money`：充值/退费流水。
- `LogonInfo`：上机返回信息。
- `SettleInfo`：下机结算返回信息。
- `Rate`：计费规则。
- `Admin`：管理员模型，目前主要功能通过固定 root/root 登录实现。

### 2. 业务层

业务层目录：

```text
C_PROGRAM_EXPERIMENT/src/business/
```

当前业务层已按职责拆分：

```text
billing_service.c       上机、下机核心逻辑
money_service.c         充值、退费逻辑
card_service.c          开卡、注册、查卡、模糊查询、余额查询
card_query_service.c    管理员高级查询：按状态筛选、排序、低余额查询
session_service.c       登录态、管理员登录、用户登录、用户/管理员包装 API
statistics_service.c    消费记录查询、YYYY-MM 月营业额统计
cancel_service.c        注销卡逻辑
card_auth.c             卡号/密码认证与卡加载辅助函数
business_result.c       DataResult 到 BizResult 映射，以及错误文案
billing_rule.c          计费规则与计费金额计算
card_validator.c        卡号、密码、金额输入校验
time_validator.c        时间字符串校验
card_file_maintenance_service.c  卡数据文件健康检查、备份与恢复
operation_log.c         操作日志
```

业务层公开接口集中在：

```text
C_PROGRAM_EXPERIMENT/include/business.h
```

注意：`business.h` 中同时保留了新 GUI 使用的 session/user/admin API 和旧控制台兼容 API。新 GUI 代码应优先调用 session/user/admin API，不应直接绕过登录态调用 legacy API。

高级查询的类型定义和 API（`CardQueryFilterType`、`CardQuerySortType`、`CardQueryOption`、`bizQueryCardsAdvanced` 等）声明在独立的：

```text
C_PROGRAM_EXPERIMENT/include/card_query.h
```

### 3. 数据层

数据层目录：

```text
C_PROGRAM_EXPERIMENT/src/data/
```

主要文件：

```text
repository.c             卡信息仓储
billing_repository.c     计费记录仓储
money_repository.c       充值/退费流水仓储
data_file_utils.c        数据文件公共工具：换行清理、字段解析、时间转换、目录创建
card_file_backup.c       卡文件备份到 data/backup/ 并从备份恢复
card_file_health.c       逐行扫描 cards.txt，校验字段完整性并输出异常记录
```

对应头文件在：

```text
C_PROGRAM_EXPERIMENT/include/card_repository.h
C_PROGRAM_EXPERIMENT/include/billing_repository.h
C_PROGRAM_EXPERIMENT/include/money_repository.h
C_PROGRAM_EXPERIMENT/include/data_file_utils.h
C_PROGRAM_EXPERIMENT/include/card_file_backup.h
C_PROGRAM_EXPERIMENT/include/card_file_health.h
C_PROGRAM_EXPERIMENT/include/card_file_maintenance.h
```

当前数据层基于文本文件和内存链表实现。注意：仓储内部维护全局链表状态，例如先 load 再 query/update 的隐式状态仍然存在。修改时应避免在业务层随意组合底层 `dataLoadXXX + dataQueryXXX`，优先使用业务层封装好的函数。

### 4. 平台适配层

平台适配目录：

```text
C_PROGRAM_EXPERIMENT/src/platform/
```

主要负责平台相关能力，例如目录创建等。数据层的 `data_file_utils.c` 会调用平台适配函数。

### 5. 表示层：控制台

控制台表示层在：

```text
C_PROGRAM_EXPERIMENT/src/presentation/
```

主要文件：

```text
menu.c
card_ui.c
card_view.c
card_query_view.c
card_file_maintenance_ui.c
```

控制台代码主要服务课程实验的传统交互方式。后续若主要维护 GUI，不要随意破坏控制台兼容 API。

### 6. 表示层：Win32 GUI

GUI 代码在：

```text
C_PROGRAM_EXPERIMENT/src/presentation/gui/
```

主要文件：

```text
gui_main.cpp                    GUI 程序入口
gui_main_window.cpp             主窗口 WinMain 及居中辅助
gui_main_window.h               主窗口公共声明
gui_main_window_centered_wrapper.cpp  居中包装，处理 DPI 与窗口位置
gui_main_window_core.cpp        Win32 dialog 生命周期、窗口初始化、消息分发、导航路由
gui_main_window_core.h          对外只暴露 RunMainGuiDialog
gui_main_window_internal.h      GUI 内部共享状态和函数声明
gui_mode_config.cpp             GuiMode 到按钮、输入框、标题、状态文案的配置
gui_action_controller.cpp       根据当前 GuiMode 执行业务提交
gui_result_view.cpp             ListView 表格渲染
gui_utils.cpp                   UTF-8/宽字符转换、金额/时间/状态格式化
gui_utils.h                     GUI 工具函数声明
gui_resource.rc                 Win32 资源文件
gui_resource.h                  控件 ID 定义
app.manifest                    Windows 视觉样式 manifest
```

GUI 当前采用 Win32 Dialog + Resource 的方式实现。不要引入 WebView、Qt、MFC 等新 GUI 框架，除非用户明确要求。

## 数据文件

数据文件路径定义主要在：

```text
C_PROGRAM_EXPERIMENT/include/common.h
```

主要数据文件：

```text
data/cards.txt     卡信息
data/billings.txt  计费记录
data/money.txt     充值/退费流水
```

文件格式由各 repository 负责读写。不要随意改变字段顺序和分隔符，否则会破坏已有数据兼容性。

## 登录与权限边界

当前业务上有三类入口：

1. 匿名状态：登录、注册、退出。
2. 管理员状态：调用 `bizAdminXXX` 系列函数。
3. 用户状态：调用 `bizUserXXX` 系列函数。

GUI 代码应使用 `LoginSession` 判断当前角色，并调用对应的 user/admin API。

不要在 GUI 中直接调用 legacy API，例如：

```text
bizRecharge
bizRefundByAmount
bizStartBilling
bizStopBilling
bizCancelCard
```

这些 API 主要用于兼容控制台和业务内部复用。

## 修改建议

### 优先保持稳定的部分

以下部分已有较清晰分层，除非修 bug 或新增功能，不建议继续大规模重构：

- `src/business/` 当前的服务拆分。
- `src/presentation/gui/` 当前的 GUI 拆分。
- `data_file_utils.c` 公共数据文件工具。
- `LoginSession` 不保存明文密码的设计。

### 修改 GUI 时

优先按以下规则修改：

- 修改窗口生命周期、消息分发：看 `gui_main_window_core.cpp`。
- 修改按钮显示、输入框显示、模式文案：看 `gui_mode_config.cpp`。
- 修改提交按钮后的业务调用：看 `gui_action_controller.cpp`。
- 修改结果表格显示：看 `gui_result_view.cpp`。
- 修改控件 ID 或资源布局：看 `gui_resource.rc` 和 `gui_resource.h`。

### 修改业务时

优先按职责定位文件：

- 卡相关：`card_service.c`。
- 登录态和角色相关：`session_service.c`。
- 上机/下机：`billing_service.c`。
- 充值/退费：`money_service.c`。
- 注销卡：`cancel_service.c`。
- 高级查询：`card_query_service.c`。
- 营业额统计和消费记录查询：`statistics_service.c`。
- 卡数据文件维护：`card_file_maintenance_service.c`。
- 卡号密码认证：`card_auth.c`。

不要把新功能重新塞回某一个大文件。新增业务功能时，先判断它属于哪个服务。

### 修改数据层时

数据层仍有全局链表状态。修改时要注意：

- `dataQueryXXX` 通常依赖之前已经 `dataLoadXXX`。
- `dataUpdateXXX` 通常会重新 load 再 rewrite。
- 不要返回局部变量地址。
- 不要让业务层长期持有 repository 内部节点指针。
- 尽量让业务层拿结构体拷贝，而不是直接操作 repository 内部节点。

## 当前仍存在但不建议优先处理的问题

以下问题存在，但继续修改的边际收益已经不高：

1. `business.h` 仍是较大的公共头文件，同时暴露新旧 API。
2. 数据层仍用全局链表作为内部缓存状态。
3. 两个 Visual Studio 工程仍重复编译部分 core/business/data 源码。
4. `AccountManagement_GUI` 尚未通过单独 Core static library 复用公共代码。

这些问题不是当前主要维护风险。除非用户明确要求架构重构，否则不要为了“更干净”而继续大规模修改。

## 编码约束

- 保持 C11 / Visual Studio 兼容。
- GUI C++ 代码保持轻量，不引入复杂框架。
- 不改变现有数据文件格式。
- 不改变用户侧和管理员侧现有业务语义。
- 不把表示层逻辑写入业务层。
- 不让 GUI 绕过 session/user/admin API 直接访问底层 legacy API。
- 修改前先判断影响范围，优先做小步、低风险、可回滚修改。

## 推荐后续 agent 工作方式

1. 先确认当前分支是否为 `role-login-session-ui`。
2. 先定位功能属于 GUI、业务层还是数据层。
3. 优先修改最接近问题的模块。
4. 不做无收益的大规模重构。
5. 修改后检查两个工程文件是否仍包含新增源文件。
6. 对 GUI 行为修改，重点检查登录态、按钮路由、输入框显示、结果表格显示。
7. 对业务行为修改，重点检查用户和管理员两条路径是否一致。
