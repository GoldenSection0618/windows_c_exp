/*
 * 文件：business.h
 * 作用：声明业务层公开 API，包括登录态、用户操作、管理员操作和基础卡管理接口。
 * 说明：GUI 应优先调用 session/user/admin API，不应绕过登录态直接调用 legacy API。
 */
#ifndef BUSINESS_H
#define BUSINESS_H

#include <stddef.h>
#include <stdint.h>

#include "model.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 业务层统一返回码：负数表示具体失败原因，BIZ_OK 表示成功。 */
typedef enum BizResult {
    BIZ_OK = 0,
    BIZ_ERR_INVALID_CARD_NAME = -1,
    BIZ_ERR_INVALID_PASSWORD = -2,
    BIZ_ERR_INVALID_AMOUNT = -3,
    BIZ_ERR_BALANCE_TOO_LARGE = -4,
    BIZ_ERR_DUPLICATE_CARD = -5,
    BIZ_ERR_CARD_NOT_FOUND = -6,
    BIZ_ERR_FILE_OPEN = -7,
    BIZ_ERR_FILE_NOT_FOUND = -8,
    BIZ_ERR_RECORD_FORMAT = -9,
    BIZ_ERR_TIME_PARSE = -10,
    BIZ_ERR_NO_MEMORY = -11,
    BIZ_ERR_SYSTEM = -12,
    BIZ_ERR_NO_MATCHED_CARD = -13,
    BIZ_ERR_WRONG_PASSWORD = -14,
    BIZ_ERR_CARD_UNAVAILABLE = -15,
    BIZ_ERR_BALANCE_NOT_ENOUGH = -16,
    BIZ_ERR_NO_UNSETTLED_BILLING = -17,
    BIZ_ERR_CARD_STATUS_INVALID_FOR_STOP = -18,
    BIZ_ERR_CARD_CANCELED_FOR_START = -19,
    BIZ_ERR_CARD_CANCELED_FOR_RECHARGE = -20,
    BIZ_ERR_CARD_CANCELED_FOR_REFUND = -21,
    BIZ_ERR_CARD_STATUS_INVALID_FOR_REFUND = -22,
    BIZ_ERR_INVALID_TIME_RANGE = -23,
    BIZ_ERR_BILLING_RECORD_NOT_FOUND = -24,
    BIZ_ERR_CARD_CANCELED_FOR_CANCEL = -25,
    BIZ_ERR_CARD_STATUS_INVALID_FOR_CANCEL = -26,
    BIZ_ERR_CARD_CANCELED_FOR_LOGIN = -27,
    BIZ_ERR_CARD_ONLINE_FOR_LOGIN = -28
} BizResult;

/* 登录角色：用于区分匿名、管理员和普通用户的操作权限。 */
typedef enum LoginRole {
    LOGIN_ROLE_NONE = 0,
    LOGIN_ROLE_ADMIN = 1,
    LOGIN_ROLE_USER = 2
} LoginRole;

/* 登录态：只保存角色、登录标记和用户卡号，不保存明文密码。 */
typedef struct LoginSession {
    LoginRole role;
    int loggedIn;
    char cardName[CARD_NAME_MAX_LEN + 1];
} LoginSession;

/* 消费记录查询结果：items 由业务层分配，调用者按接口约定释放。 */
typedef struct BillingQueryResult {
    Billing *items;
    size_t count;
} BillingQueryResult;

/* 营业额统计结果：支持按指定 YYYY-MM 统计，也保留全年月度数组。 */
typedef struct BillingStatistics {
    int year;
    int month;
    int32_t totalAmountCent;
    int32_t monthlyAmountCent[12];
} BillingStatistics;

/* 初始化登录态为空状态。 */
void bizInitSession(LoginSession *session);

/* 退出当前登录态，清空角色和用户卡号。 */
void bizLogout(LoginSession *session);

/* 判断当前 session 是否为有效管理员登录态。 */
int bizIsAdminSession(const LoginSession *session);

/* 判断当前 session 是否为有效用户登录态。 */
int bizIsUserSession(const LoginSession *session);

/* 管理员登录：账号密码固定由业务层校验，成功后写入管理员 session。 */
BizResult bizAdminLogin(const char *accountInput, const char *passwordInput, LoginSession *session);

/* 用户注册：按默认赠送余额创建卡，不允许输入初始金额。 */
BizResult bizUserRegister(const char *cardNameInput, const char *passwordInput, Card *createdCard);

/* 用户登录：校验卡号密码，已注销卡和正在上机卡不能登录。 */
BizResult bizUserLogin(const char *cardNameInput, const char *passwordInput, LoginSession *session);

/* 管理员按卡号查询卡信息，不需要卡密码。 */
BizResult bizAdminQueryCard(const LoginSession *session, const char *cardNameInput, Card *queriedCard);

/* 管理员按关键字模糊查询卡号，buffer 不足时通过 requiredCount 返回所需容量。 */
BizResult bizAdminQueryCardsByKeyword(const LoginSession *session,
                                      const char *keywordInput,
                                      Card *buffer,
                                      size_t capacity,
                                      size_t *actualCount,
                                      size_t *requiredCount);

/* 管理员强制下机：只校验管理员 session 和卡号，不要求用户密码。 */
BizResult bizAdminStopBilling(const LoginSession *session,
                              const char *cardNameInput,
                              time_t requestTime,
                              SettleInfo *settleInfo);

/* 管理员充值：按卡号和金额更新余额并记录充值流水。 */
BizResult bizAdminRecharge(const LoginSession *session,
                           const char *cardNameInput,
                           const char *amountInput,
                           Money *rechargeRecord,
                           Card *updatedCard);

/* 管理员退费：按卡号和金额扣减余额并记录退费流水。 */
BizResult bizAdminRefundByAmount(const LoginSession *session,
                                 const char *cardNameInput,
                                 const char *amountInput,
                                 Money *refundRecord,
                                 Card *updatedCard);

/* 管理员营业额统计：yearMonthInput 必须符合 YYYY-MM 格式。 */
BizResult bizAdminGetBillingStatistics(const LoginSession *session,
                                       const char *yearMonthInput,
                                       BillingStatistics *statistics);

/* 用户查询当前登录卡余额。 */
BizResult bizUserQueryBalance(const LoginSession *session, Card *queriedCard);

/* 用户上机：基于当前登录卡创建未结算计费记录。 */
BizResult bizUserStartBilling(const LoginSession *session, time_t requestTime, LogonInfo *logonInfo);

/* 用户下机：结算当前登录卡的未结算记录并扣费。 */
BizResult bizUserStopBilling(const LoginSession *session, time_t requestTime, SettleInfo *settleInfo);

/* 用户给当前登录卡充值。 */
BizResult bizUserRecharge(const LoginSession *session,
                          const char *amountInput,
                          Money *rechargeRecord,
                          Card *updatedCard);

/* 用户按金额从当前登录卡退费。 */
BizResult bizUserRefundByAmount(const LoginSession *session,
                                const char *amountInput,
                                Money *refundRecord,
                                Card *updatedCard);

/* 用户注销当前登录卡：需要再次输入卡号和密码确认。 */
BizResult bizUserCancelCardWithPassword(const LoginSession *session,
                                        const char *cardNameInput,
                                        const char *passwordInput,
                                        Money *refundRecord,
                                        Card *updatedCard);

/* 兼容控制台的开卡接口：创建指定初始金额的新卡。 */
BizResult bizAddCard(const char *cardNameInput, const char *passwordInput, const char *amountInput, Card *createdCard);

/* 兼容控制台的精确查卡接口。 */
BizResult bizQueryCard(const char *cardNameInput, Card *queriedCard);

/* 兼容控制台的模糊查卡接口。 */
BizResult bizQueryCardsByKeyword(const char *keywordInput,
                                 Card *buffer,
                                 size_t capacity,
                                 size_t *actualCount,
                                 size_t *requiredCount);

/* 将业务返回码转换为面向表示层的中文提示。 */
const char *bizGetMessage(BizResult result);

/* 释放业务层持有的全局资源。 */
void bizShutdown(void);

#ifdef __cplusplus
}
#endif

#endif
