/*
 * 文件：model.h
 * 作用：定义计费管理系统的核心数据模型和业务常量。
 * 说明：这些结构体直接对应数据文件字段，修改字段顺序会影响 repository 读写兼容性。
 */
#ifndef MODEL_H
#define MODEL_H

#include <stdint.h>
#include <time.h>

#define CARD_NAME_MAX_LEN 18
#define CARD_PWD_MAX_LEN 8
#define MAX_CARD_COUNT 1024
#define MAX_BALANCE_CENT 100000000 /* 1000000.00 元，业务规则要求余额必须小于该值 */

/* 卡状态：用于判断上机、登录、充值、退费和注销等业务边界。 */
typedef enum CardStatus {
    CARD_STATUS_OFFLINE = 0,  /* 未上机，可登录和上机 */
    CARD_STATUS_ONLINE = 1,   /* 正在上机，不能重复登录或注销 */
    CARD_STATUS_CANCELED = 2, /* 已注销，不能继续业务操作 */
    CARD_STATUS_INVALID = 3   /* 失效状态，保留给文件异常或扩展场景 */
} CardStatus;

/* 卡信息模型：保存卡号、认证信息、状态、余额和累计使用信息。 */
typedef struct Card {
    char aCardName[CARD_NAME_MAX_LEN + 1]; /* 卡号，业务唯一标识 */
    char aPwd[CARD_PWD_MAX_LEN + 1];       /* 卡密码，仅用于认证 */
    int nStatus;                           /* 卡状态，取值见 CardStatus */
    time_t tStart;                         /* 开卡时间 */
    time_t tEnd;                           /* 截止时间，当前业务主要保留字段 */
    int32_t nTotalUseCent;                 /* 累计消费金额，单位：分 */
    time_t tLast;                          /* 最后使用时间 */
    int nUseCount;                         /* 累计使用次数 */
    int32_t nBalanceCent;                  /* 当前余额，单位：分 */
    int nDel;                              /* 删除标识，0-未删除，1-删除 */
} Card;

/* 计费记录模型：保存一次上机到下机的消费过程。 */
typedef struct Billing {
    char aCardName[CARD_NAME_MAX_LEN + 1]; /* 消费卡号 */
    time_t tStart;                         /* 上机时间 */
    time_t tEnd;                           /* 下机时间，未结算时为 0 */
    int32_t nAmountCent;                   /* 本次消费金额，单位：分 */
    int nStatus;                           /* 消费状态，0-未结算，1-已结算 */
    int nDel;                              /* 删除标识，0-未删除，1-删除 */
} Billing;

/* 上机返回信息：供表示层展示上机结果。 */
typedef struct LogonInfo {
    char aCardName[CARD_NAME_MAX_LEN + 1]; /* 上机卡号 */
    time_t tStart;                         /* 上机时间 */
    int nStatus;                           /* 上机后的卡状态 */
    int32_t nBalanceCent;                  /* 上机时余额，单位：分 */
} LogonInfo;

/* 下机结算信息：供表示层展示结算金额和剩余余额。 */
typedef struct SettleInfo {
    char aCardName[CARD_NAME_MAX_LEN + 1]; /* 结算卡号 */
    time_t tStart;                         /* 上机时间 */
    time_t tEnd;                           /* 下机时间 */
    int32_t nAmountCent;                   /* 本次消费金额，单位：分 */
    int32_t nBalanceCent;                  /* 结算后余额，单位：分 */
} SettleInfo;

/* 充值退费流水模型：保存充值或退费操作的金额和发生时间。 */
typedef struct Money {
    char aCardName[CARD_NAME_MAX_LEN + 1]; /* 操作卡号 */
    time_t tTime;                          /* 充值或退费时间 */
    int nStatus;                           /* 流水状态，0-充值，1-退费 */
    int32_t nMoneyCent;                    /* 流水金额，单位：分 */
    int nDel;                              /* 删除标识，0-未删除，1-删除 */
} Money;

/* 计费规则模型：保留课程实验中的费率配置字段。 */
typedef struct Rate {
    int starttime;                         /* 生效起始时间段 */
    int endtime;                           /* 生效结束时间段 */
    int unit;                              /* 计费单位 */
    int32_t nChargeCent;                   /* 单位费用，单位：分 */
    int ratetype;                          /* 计费类型 */
    int del;                               /* 删除标识 */
} Rate;

/* 管理员模型：当前 GUI/业务主要使用固定 root/root 登录。 */
typedef struct Admin {
    char name[32];                         /* 管理员账号 */
    char pwd[32];                          /* 管理员密码 */
    int privilege;                         /* 权限位，预留扩展 */
    int del;                               /* 删除标识 */
} Admin;

#endif
