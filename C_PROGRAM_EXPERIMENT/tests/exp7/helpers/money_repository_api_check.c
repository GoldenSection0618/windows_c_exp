#include "money_repository.h"
#include "money_storage_file.h"
#include "common.h"

#include <stdio.h>
#include <string.h>

static Money makeMoney(const char *cardName, time_t tTime, int status, int amountCent)
{
    Money money = {0};

    snprintf(money.aCardName, sizeof(money.aCardName), "%s", cardName);
    money.tTime = tTime;
    money.nStatus = status;
    money.nMoneyCent = amountCent;
    money.nDel = 0;
    return money;
}

int main(void)
{
    Money rechargeA = makeMoney("cardA", (time_t)1000, 0, 5000);
    Money refundA = makeMoney("cardA", (time_t)2000, 1, 1200);
    Money rechargeB = makeMoney("cardB", (time_t)1500, 0, 3000);
    const Money *found = NULL;
    int count = 0;

    remove(MONEY_DATA_FILE_PATH);
    dataCleanupMoneys();

    if (dataQueryLatestMoneyByCardName("cardA") != NULL) {
        fprintf(stderr, "expected empty money list\n");
        return 1;
    }

    if (dataLoadMoneys() != DATA_ERR_FILE_NOT_FOUND) {
        fprintf(stderr, "expected missing money file error\n");
        return 1;
    }

    if (dataGetMoneyCount() != DATA_ERR_FILE_NOT_FOUND) {
        fprintf(stderr, "expected missing money file count error\n");
        return 1;
    }

    {
        FILE *fp = fopen(MONEY_DATA_FILE_PATH, "w");
        if (fp == NULL) {
            fprintf(stderr, "create empty money file failed\n");
            return 1;
        }
        fclose(fp);
    }

    if (dataLoadMoneys() != 0) {
        fprintf(stderr, "expected empty money file load count 0\n");
        return 1;
    }

    dataCleanupMoneys();
    if (dataAddMoney(&rechargeA) != DATA_OK || dataAddMoney(&refundA) != DATA_OK) {
        fprintf(stderr, "dataAddMoney failed\n");
        return 1;
    }
    found = dataQueryLatestMoneyByCardName("cardA");
    if (found == NULL || found->tTime != (time_t)2000 || found->nStatus != 1 || found->nMoneyCent != 1200) {
        fprintf(stderr, "latest in-memory money query failed\n");
        return 1;
    }

    dataCleanupMoneys();
    remove(MONEY_DATA_FILE_PATH);

    if (dataSaveMoney(&rechargeA) != DATA_OK ||
        dataSaveMoney(&refundA) != DATA_OK ||
        dataSaveMoney(&rechargeB) != DATA_OK) {
        fprintf(stderr, "dataSaveMoney failed\n");
        return 1;
    }

    count = dataGetMoneyCount();
    if (count != 3) {
        fprintf(stderr, "money file count mismatch: got %d want 3\n", count);
        return 1;
    }

    count = dataLoadMoneys();
    if (count != 3) {
        fprintf(stderr, "money load count mismatch: got %d want 3\n", count);
        return 1;
    }

    found = dataQueryLatestMoneyByCardName("cardA");
    if (found == NULL || found->tTime != (time_t)2000 || found->nStatus != 1 || found->nMoneyCent != 1200) {
        fprintf(stderr, "latest persisted money query for cardA failed\n");
        return 1;
    }

    found = dataQueryLatestMoneyByCardName("cardB");
    if (found == NULL || found->tTime != (time_t)1500 || found->nStatus != 0 || found->nMoneyCent != 3000) {
        fprintf(stderr, "latest persisted money query for cardB failed\n");
        return 1;
    }

    dataCleanupMoneys();
    return 0;
}
