#include "money_repository.h"
#include "common.h"
#include "data_file_utils.h"

#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static MoneyNode *g_pMoneyListHead = NULL;
static size_t g_moneyCount = 0;

static DataResult parseMoneyRecord(const char *line, Money *outMoney);
static int isMoneyCardNameEqual(const char *a, const char *b);

static DataResult parseMoneyRecord(const char *line, Money *outMoney)
{
    char buffer[256];
    char *fields[5];
    char *cursor = NULL;
    char *separator = NULL;
    int index = 0;
    Money money = {0};

    if (line == NULL || outMoney == NULL) {
        return DATA_ERR_INVALID_ARG;
    }

    if (snprintf(buffer, sizeof(buffer), "%s", line) >= (int)sizeof(buffer)) {
        return DATA_ERR_RECORD_FORMAT;
    }
    dataTrimLineEnding(buffer);

    cursor = buffer;
    for (index = 0; index < 4; index++) {
        separator = strchr(cursor, '|');
        if (separator == NULL) {
            return DATA_ERR_RECORD_FORMAT;
        }
        *separator = '\0';
        fields[index] = cursor;
        cursor = separator + 1;
    }
    fields[4] = cursor;

    if (strchr(fields[4], '|') != NULL) {
        return DATA_ERR_RECORD_FORMAT;
    }

    for (index = 0; index < 5; index++) {
        if (fields[index][0] == '\0') {
            return DATA_ERR_RECORD_FORMAT;
        }
    }

    if (snprintf(money.aCardName, sizeof(money.aCardName), "%s", fields[0]) >= (int)sizeof(money.aCardName)) {
        return DATA_ERR_RECORD_FORMAT;
    }
    if (dataStringToTime(fields[1], &money.tTime) != DATA_OK) {
        return DATA_ERR_TIME_PARSE;
    }
    if (dataParseIntField(fields[2], &money.nStatus) != 0) {
        return DATA_ERR_RECORD_FORMAT;
    }
    if (dataParseInt32Field(fields[3], &money.nMoneyCent) != 0) {
        return DATA_ERR_RECORD_FORMAT;
    }
    if (dataParseIntField(fields[4], &money.nDel) != 0) {
        return DATA_ERR_RECORD_FORMAT;
    }

    *outMoney = money;
    return DATA_OK;
}

static int isMoneyCardNameEqual(const char *a, const char *b)
{
    if (a == NULL || b == NULL) {
        return 0;
    }
    return strcmp(a, b) == 0;
}

int dataAddMoney(const Money *money)
{
    MoneyNode *pNewNode = NULL;
    MoneyNode *pTail = NULL;

    if (money == NULL) {
        return DATA_ERR_INVALID_ARG;
    }

    pNewNode = (MoneyNode *)malloc(sizeof(MoneyNode));
    if (pNewNode == NULL) {
        return DATA_ERR_NO_MEMORY;
    }

    pNewNode->moneyData = *money;
    pNewNode->pNext = NULL;

    if (g_pMoneyListHead == NULL) {
        g_pMoneyListHead = pNewNode;
    } else {
        pTail = g_pMoneyListHead;
        while (pTail->pNext != NULL) {
            pTail = pTail->pNext;
        }
        pTail->pNext = pNewNode;
    }

    g_moneyCount++;
    return DATA_OK;
}

const Money *dataQueryLatestMoneyByCardName(const char *cardName)
{
    MoneyNode *pCurrent = g_pMoneyListHead;
    const Money *pMatched = NULL;

    if (cardName == NULL || *cardName == '\0') {
        return NULL;
    }

    while (pCurrent != NULL) {
        if (pCurrent->moneyData.nDel == 0 && isMoneyCardNameEqual(pCurrent->moneyData.aCardName, cardName)) {
            if (pMatched == NULL || pCurrent->moneyData.tTime > pMatched->tTime) {
                pMatched = &pCurrent->moneyData;
            }
        }
        pCurrent = pCurrent->pNext;
    }

    return pMatched;
}

void dataCleanupMoneys(void)
{
    MoneyNode *pCurrent = g_pMoneyListHead;

    while (pCurrent != NULL) {
        MoneyNode *pNext = pCurrent->pNext;
        free(pCurrent);
        pCurrent = pNext;
    }

    g_pMoneyListHead = NULL;
    g_moneyCount = 0;
}

DataResult dataSaveMoney(const Money *money)
{
    FILE *fp = NULL;
    char timeBuf[CARD_TIME_STR_LEN + 1];
    DataResult ret = DATA_OK;

    if (money == NULL) {
        return DATA_ERR_INVALID_ARG;
    }

    ret = dataEnsureDataDirByFilePath(MONEY_DATA_FILE_PATH);
    if (ret != DATA_OK) {
        return ret;
    }

    fp = fopen(MONEY_DATA_FILE_PATH, "a");
    if (fp == NULL) {
        return DATA_ERR_FILE_OPEN;
    }

    dataFormatTimeString(money->tTime, timeBuf, sizeof(timeBuf));
    if (fprintf(fp, "%s|%s|%d|%d|%d\n",
                money->aCardName,
                timeBuf,
                money->nStatus,
                money->nMoneyCent,
                money->nDel) < 0) {
        fclose(fp);
        return DATA_ERR_FILE_OPEN;
    }

    if (fclose(fp) != 0) {
        return DATA_ERR_FILE_OPEN;
    }

    return DATA_OK;
}

int dataLoadMoneys(void)
{
    FILE *fp = NULL;
    char line[256];
    int count = 0;

    dataCleanupMoneys();

    fp = fopen(MONEY_DATA_FILE_PATH, "r");
    if (fp == NULL) {
        if (errno == ENOENT) {
            return DATA_ERR_FILE_NOT_FOUND;
        }
        return DATA_ERR_FILE_OPEN;
    }

    while (fgets(line, sizeof(line), fp) != NULL) {
        Money money;
        DataResult ret = parseMoneyRecord(line, &money);

        if (ret != DATA_OK) {
            fclose(fp);
            dataCleanupMoneys();
            return ret;
        }

        ret = (DataResult)dataAddMoney(&money);
        if (ret != DATA_OK) {
            fclose(fp);
            dataCleanupMoneys();
            return ret;
        }

        count++;
    }

    if (ferror(fp)) {
        fclose(fp);
        dataCleanupMoneys();
        return DATA_ERR_FILE_OPEN;
    }

    if (fclose(fp) != 0) {
        dataCleanupMoneys();
        return DATA_ERR_FILE_OPEN;
    }

    return count;
}

int dataGetMoneyCount(void)
{
    FILE *fp = NULL;
    char line[256];
    int count = 0;

    fp = fopen(MONEY_DATA_FILE_PATH, "r");
    if (fp == NULL) {
        if (errno == ENOENT) {
            return DATA_ERR_FILE_NOT_FOUND;
        }
        return DATA_ERR_FILE_OPEN;
    }

    while (fgets(line, sizeof(line), fp) != NULL) {
        Money money;
        DataResult ret = parseMoneyRecord(line, &money);

        if (ret != DATA_OK) {
            fclose(fp);
            return ret;
        }
        count++;
    }

    if (ferror(fp)) {
        fclose(fp);
        return DATA_ERR_FILE_OPEN;
    }

    if (fclose(fp) != 0) {
        return DATA_ERR_FILE_OPEN;
    }

    return count;
}
