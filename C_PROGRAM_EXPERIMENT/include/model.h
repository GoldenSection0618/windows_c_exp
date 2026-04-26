#ifndef MODEL_H
#define MODEL_H

#include <stdint.h>
#include <time.h>

#define CARD_NAME_MAX_LEN 18
#define CARD_PWD_MAX_LEN 8
#define MAX_CARD_COUNT 1024
#define MAX_BALANCE_CENT 100000000 /* 1000000.00 元，业务规则要求余额必须小于该值 */

typedef enum CardStatus {
    CARD_STATUS_OFFLINE = 0,
    CARD_STATUS_ONLINE = 1,
    CARD_STATUS_CANCELED = 2,
    CARD_STATUS_INVALID = 3
} CardStatus;

typedef struct Card {
    char aCardName[CARD_NAME_MAX_LEN + 1];
    char aPwd[CARD_PWD_MAX_LEN + 1];
    int nStatus;
    time_t tStart;
    time_t tEnd;
    int32_t nTotalUseCent;
    time_t tLast;
    int nUseCount;
    int32_t nBalanceCent;
    int nDel;
} Card;

typedef struct Billing {
    char aCardName[CARD_NAME_MAX_LEN + 1];
    time_t tStart;
    time_t tEnd;
    int32_t nAmountCent;
    int nStatus;
    int nDel;
} Billing;

typedef struct LogonInfo {
    char aCardName[CARD_NAME_MAX_LEN + 1];
    time_t tStart;
    int nStatus;
    int32_t nBalanceCent;
} LogonInfo;

typedef struct SettleInfo {
    char aCardName[CARD_NAME_MAX_LEN + 1];
    time_t tStart;
    time_t tEnd;
    int32_t nAmountCent;
    int32_t nBalanceCent;
} SettleInfo;

typedef struct Money {
    char aCardName[CARD_NAME_MAX_LEN + 1];
    time_t tTime;
    int nStatus;
    int32_t nMoneyCent;
    int nDel;
} Money;

typedef struct Rate {
    int starttime;
    int endtime;
    int unit;
    int32_t nChargeCent;
    int ratetype;
    int del;
} Rate;

typedef struct Admin {
    char name[32];
    char pwd[32];
    int privilege;
    int del;
} Admin;

#endif
