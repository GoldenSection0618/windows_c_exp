#include "business.h"
#include "card_view.h"
#include "menu.h"
#include "platform.h"

#include <stdio.h>
#include <time.h>

#define INPUT_BUF_SIZE 256

static void showEntryMenu(void)
{
    printf("%s\n", "37-1023007337-杨立晗-软件2301");
    printf("========== 计费管理系统 ==========%s\n", "");
    printf("1. 管理员登录\n");
    printf("2. 用户注册\n");
    printf("3. 用户登录\n");
    printf("0. 退出系统\n");
}

static void showAdminMenu(void)
{
    printf("========== 管理员后台 ==========%s\n", "");
    printf("1. 查询卡\n");
    printf("2. 下机\n");
    printf("3. 充值\n");
    printf("4. 退费\n");
    printf("5. 营业额统计\n");
    printf("0. 退出登录\n");
}

static void showUserMenu(const LoginSession *session)
{
    printf("========== 用户中心 ==========%s\n", "");
    printf("当前登录卡号：%s\n", session->cardName);
    printf("1. 查余额\n");
    printf("2. 上机\n");
    printf("3. 下机\n");
    printf("4. 充值\n");
    printf("5. 退费\n");
    printf("6. 注销卡\n");
    printf("0. 退出登录\n");
}

static void printBizFailure(const char *prefix, BizResult result)
{
    printf("%s\n", prefix);
    if (result == BIZ_ERR_WRONG_PASSWORD) {
        printf("账号、卡号或密码错误！\n");
        return;
    }
    if (result == BIZ_ERR_SYSTEM) {
        printf("系统内部错误，或当前登录角色没有权限执行该操作。\n");
        return;
    }
    printf("%s\n", bizGetMessage(result));
}

static void showPromotionMessage(void)
{
    printf("Promotion：新用户注册成功，已获得 100 元新手礼包。\n");
}

static void handleAdminLogin(LoginSession *session)
{
    char account[INPUT_BUF_SIZE];
    char password[INPUT_BUF_SIZE];
    BizResult result = BIZ_OK;

    if (readTextInput("请输入管理员账号：", account, sizeof(account)) != 0 ||
        readTextInput("请输入管理员密码：", password, sizeof(password)) != 0) {
        printf("管理员账号或密码输入错误。\n");
        return;
    }

    result = bizAdminLogin(account, password, session);
    if (result != BIZ_OK) {
        printBizFailure("管理员登录失败！", result);
        return;
    }

    printf("管理员登录成功。\n");
}

static void handleUserRegister(void)
{
    char cardName[INPUT_BUF_SIZE];
    char password[INPUT_BUF_SIZE];
    Card card;
    BizResult result = BIZ_OK;

    if (readTextInput("请输入注册卡号（1~18位）：", cardName, sizeof(cardName)) != 0) {
        printf("%s\n", bizGetMessage(BIZ_ERR_INVALID_CARD_NAME));
        return;
    }
    if (readTextInput("请输入注册密码（1~8位）：", password, sizeof(password)) != 0) {
        printf("%s\n", bizGetMessage(BIZ_ERR_INVALID_PASSWORD));
        return;
    }

    result = bizUserRegister(cardName, password, &card);
    if (result != BIZ_OK) {
        printBizFailure("用户注册失败！", result);
        return;
    }

    showPromotionMessage();
    viewShowCardSummary(&card);
}

static void handleUserLogin(LoginSession *session)
{
    char cardName[INPUT_BUF_SIZE];
    char password[INPUT_BUF_SIZE];
    BizResult result = BIZ_OK;

    if (readTextInput("请输入卡号：", cardName, sizeof(cardName)) != 0 ||
        readTextInput("请输入密码：", password, sizeof(password)) != 0) {
        printf("卡号或密码输入错误。\n");
        return;
    }

    result = bizUserLogin(cardName, password, session);
    if (result != BIZ_OK) {
        printBizFailure("用户登录失败！", result);
        return;
    }

    printf("用户登录成功，当前卡号：%s。\n", session->cardName);
}

static void handleAdminQueryCard(const LoginSession *session)
{
    char cardName[INPUT_BUF_SIZE];
    Card card;
    BizResult result = BIZ_OK;

    if (readTextInput("请输入卡号：", cardName, sizeof(cardName)) != 0) {
        printf("%s\n", bizGetMessage(BIZ_ERR_INVALID_CARD_NAME));
        return;
    }

    result = bizAdminQueryCard(session, cardName, &card);
    if (result != BIZ_OK) {
        printBizFailure("查询卡失败！", result);
        return;
    }

    viewShowQueryCardDetails(&card);
}

static void handleAdminStopBilling(const LoginSession *session)
{
    char cardName[INPUT_BUF_SIZE];
    SettleInfo settleInfo;
    BizResult result = BIZ_OK;

    if (readTextInput("请输入需要下机的卡号：", cardName, sizeof(cardName)) != 0) {
        printf("%s\n", bizGetMessage(BIZ_ERR_INVALID_CARD_NAME));
        return;
    }

    result = bizAdminStopBilling(session, cardName, time(NULL), &settleInfo);
    if (result != BIZ_OK) {
        printBizFailure("管理员下机失败！", result);
        return;
    }

    printf("管理员下机成功！\n");
    viewShowSettleInfo(&settleInfo);
}

static void handleAdminRecharge(const LoginSession *session)
{
    char cardName[INPUT_BUF_SIZE];
    char amount[INPUT_BUF_SIZE];
    Money money;
    Card card;
    BizResult result = BIZ_OK;

    if (readTextInput("请输入充值卡号：", cardName, sizeof(cardName)) != 0 ||
        readTextInput("请输入充值金额（元）：", amount, sizeof(amount)) != 0) {
        printf("输入错误。\n");
        return;
    }

    result = bizAdminRecharge(session, cardName, amount, &money, &card);
    if (result != BIZ_OK) {
        printBizFailure("管理员充值失败！", result);
        return;
    }

    printf("管理员充值成功！\n");
    viewShowRechargeInfo(&card, money.nMoneyCent);
}

static void handleAdminRefund(const LoginSession *session)
{
    char cardName[INPUT_BUF_SIZE];
    char amount[INPUT_BUF_SIZE];
    Money money;
    Card card;
    BizResult result = BIZ_OK;

    if (readTextInput("请输入退费卡号：", cardName, sizeof(cardName)) != 0 ||
        readTextInput("请输入退费金额（元）：", amount, sizeof(amount)) != 0) {
        printf("输入错误。\n");
        return;
    }

    result = bizAdminRefundByAmount(session, cardName, amount, &money, &card);
    if (result != BIZ_OK) {
        printBizFailure("管理员退费失败！", result);
        return;
    }

    printf("管理员退费成功！\n");
    viewShowRefundInfo(&card, money.nMoneyCent);
}

static void handleAdminStatistics(const LoginSession *session)
{
    char year[INPUT_BUF_SIZE];
    BillingStatistics statistics;
    int i = 0;
    BizResult result = BIZ_OK;

    if (readTextInput("请输入统计年份（YYYY）：", year, sizeof(year)) != 0) {
        printf("%s\n", bizGetMessage(BIZ_ERR_INVALID_TIME_RANGE));
        return;
    }

    result = bizAdminGetBillingStatistics(session, year, &statistics);
    if (result != BIZ_OK) {
        printBizFailure("营业额统计失败！", result);
        return;
    }

    printf("========== 营业额统计 ==========%s\n", "");
    printf("年份：%d\n", statistics.year);
    printf("总营业额：%.2f 元\n", statistics.totalAmountCent / 100.0);
    for (i = 0; i < 12; i++) {
        printf("%02d月：%.2f 元\n", i + 1, statistics.monthlyAmountCent[i] / 100.0);
    }
}

static void runAdminConsole(LoginSession *session)
{
    int choice = -1;

    while (bizIsAdminSession(session)) {
        showAdminMenu();
        if (readChoiceInput("请选择管理员功能：", &choice) != 0) {
            printf("输入格式错误，请输入数字编号。\n\n");
            continue;
        }

        switch (choice) {
        case 1:
            handleAdminQueryCard(session);
            break;
        case 2:
            handleAdminStopBilling(session);
            break;
        case 3:
            handleAdminRecharge(session);
            break;
        case 4:
            handleAdminRefund(session);
            break;
        case 5:
            handleAdminStatistics(session);
            break;
        case 0:
            bizLogout(session);
            printf("已退出管理员登录。\n");
            break;
        default:
            printf("无效管理员功能编号，请输入 0~5。\n");
            break;
        }
        printf("\n");
    }
}

static void handleUserQueryBalance(const LoginSession *session)
{
    Card card;
    BizResult result = bizUserQueryBalance(session, &card);
    if (result != BIZ_OK) {
        printBizFailure("查询余额失败！", result);
        return;
    }
    viewShowQueryCardDetails(&card);
}

static void handleUserStartBilling(const LoginSession *session)
{
    LogonInfo logonInfo;
    BizResult result = bizUserStartBilling(session, time(NULL), &logonInfo);
    if (result != BIZ_OK) {
        printBizFailure("上机失败！", result);
        return;
    }
    printf("上机成功！\n");
    viewShowLogonInfo(&logonInfo);
}

static void handleUserStopBilling(const LoginSession *session)
{
    SettleInfo settleInfo;
    BizResult result = bizUserStopBilling(session, time(NULL), &settleInfo);
    if (result != BIZ_OK) {
        printBizFailure("下机失败！", result);
        return;
    }
    printf("下机成功！\n");
    viewShowSettleInfo(&settleInfo);
}

static void handleUserRecharge(const LoginSession *session)
{
    char amount[INPUT_BUF_SIZE];
    Money money;
    Card card;
    BizResult result = BIZ_OK;

    if (readTextInput("请输入充值金额（元）：", amount, sizeof(amount)) != 0) {
        printf("%s\n", bizGetMessage(BIZ_ERR_INVALID_AMOUNT));
        return;
    }

    result = bizUserRecharge(session, amount, &money, &card);
    if (result != BIZ_OK) {
        printBizFailure("充值失败！", result);
        return;
    }

    printf("充值成功！\n");
    viewShowRechargeInfo(&card, money.nMoneyCent);
}

static void handleUserRefund(const LoginSession *session)
{
    char amount[INPUT_BUF_SIZE];
    Money money;
    Card card;
    BizResult result = BIZ_OK;

    if (readTextInput("请输入退费金额（元）：", amount, sizeof(amount)) != 0) {
        printf("%s\n", bizGetMessage(BIZ_ERR_INVALID_AMOUNT));
        return;
    }

    result = bizUserRefundByAmount(session, amount, &money, &card);
    if (result != BIZ_OK) {
        printBizFailure("退费失败！", result);
        return;
    }

    printf("退费成功！\n");
    viewShowRefundInfo(&card, money.nMoneyCent);
}

static void handleUserCancelCard(LoginSession *session)
{
    char cardName[INPUT_BUF_SIZE];
    char password[INPUT_BUF_SIZE];
    Money money;
    Card card;
    BizResult result = BIZ_OK;

    printf("注销卡是高风险操作，请再次输入卡号和密码确认。\n");
    if (readTextInput("请输入卡号：", cardName, sizeof(cardName)) != 0 ||
        readTextInput("请输入密码：", password, sizeof(password)) != 0) {
        printf("卡号或密码输入错误。\n");
        return;
    }

    result = bizUserCancelCardWithPassword(session, cardName, password, &money, &card);
    if (result != BIZ_OK) {
        printBizFailure("注销卡失败！", result);
        return;
    }

    printf("注销卡成功！\n");
    viewShowCancelCardInfo(&card, money.nMoneyCent);
    bizLogout(session);
    printf("卡已注销，当前用户已自动退出登录。\n");
}

static void runUserConsole(LoginSession *session)
{
    int choice = -1;

    while (bizIsUserSession(session)) {
        showUserMenu(session);
        if (readChoiceInput("请选择用户功能：", &choice) != 0) {
            printf("输入格式错误，请输入数字编号。\n\n");
            continue;
        }

        switch (choice) {
        case 1:
            handleUserQueryBalance(session);
            break;
        case 2:
            handleUserStartBilling(session);
            break;
        case 3:
            handleUserStopBilling(session);
            break;
        case 4:
            handleUserRecharge(session);
            break;
        case 5:
            handleUserRefund(session);
            break;
        case 6:
            handleUserCancelCard(session);
            break;
        case 0:
            bizLogout(session);
            printf("已退出用户登录。\n");
            break;
        default:
            printf("无效用户功能编号，请输入 0~6。\n");
            break;
        }
        printf("\n");
    }
}

int main(void)
{
    LoginSession session;
    int choice = -1;

    platformInitConsole();
    bizInitSession(&session);

    while (1) {
        showEntryMenu();

        if (readChoiceInput("请选择登录方式：", &choice) != 0) {
            printf("输入格式错误，请输入数字编号。\n\n");
            continue;
        }

        switch (choice) {
        case 1:
            handleAdminLogin(&session);
            if (bizIsAdminSession(&session)) {
                runAdminConsole(&session);
            }
            break;
        case 2:
            handleUserRegister();
            break;
        case 3:
            handleUserLogin(&session);
            if (bizIsUserSession(&session)) {
                runUserConsole(&session);
            }
            break;
        case 0:
            bizShutdown();
            return 0;
        default:
            printf("无效登录方式，请输入 0~3。\n");
            break;
        }
        printf("\n");
    }
}
