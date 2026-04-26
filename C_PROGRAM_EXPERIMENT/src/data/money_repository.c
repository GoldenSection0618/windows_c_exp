#include "money_repository.h"
#include "common.h"
#include "platform.h"

#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static MoneyNode *g_pMoneyListHead = NULL;
static size_t g_moneyCount = 0;

static DataResult ensureMoneyDataDir(void);
static void trimLineEnding(char *text);
static void formatMoneyTimeString(time_t value, char *buffer, size_t size);
static int parseIntField(const char *text, int *value);
static int parseInt32Field(const char *text, int32_t *value);
static DataResult stringToTime(const char *text, time_t *outTime);
static DataResult parseMoneyRecord(const char *line, Money *outMoney);
static int isMoneyCardNameEqual(const char *a, const char *b);

static void trimLineEnding(char *text)
{
    size_t len = 0;

    if (text == NULL) {
        return;
    }

    len = strlen(text);
    while (len > 0 && (text[len - 1] == '\n' || text[len - 1] == '\r')) {
        text[len - 1] = '\0';
        len--;
    }
}

static void formatMoneyTimeString(time_t value, char *buffer, size_t size)
{
    struct tm *localValue = NULL;

    if (buffer == NULL || size == 0) {
        return;
    }

    localValue = localtime(&value);
    if (localValue == NULL) {
        buffer[0] = '\0';
        return;
    }

    if (strftime(buffer, size, "%Y-%m-%d %H:%M:%S", localValue) == 0) {
        buffer[0] = '\0';
    }
}

static int parseIntField(const char *text, int *value)
{
    char *endptr = NULL;
    long parsedValue = 0;

    if (text == NULL || value == NULL || *text == '\0') {
        return -1;
    }

    errno = 0;
    parsedValue = strtol(text, &endptr, 10);
    if (errno != 0 || endptr == text || *endptr != '\0') {
        return -1;
    }
    if (parsedValue < INT_MIN || parsedValue > INT_MAX) {
        return -1;
    }

    *value = (int)parsedValue;
    return 0;
}

static int parseInt32Field(const char *text, int32_t *value)
{
    char *endptr = NULL;
    long parsedValue = 0;

    if (text == NULL || value == NULL || *text == '\0') {
        return -1;
    }

    errno = 0;
    parsedValue = strtol(text, &endptr, 10);
    if (errno != 0 || endptr == text || *endptr != '\0') {
        return -1;
    }
    if (parsedValue < INT32_MIN || parsedValue > INT32_MAX) {
        return -1;
    }

    *value = (int32_t)parsedValue;
    return 0;
}

static DataResult stringToTime(const char *text, time_t *outTime)
{
    int year = 0;
    int month = 0;
    int day = 0;
    int hour = 0;
    int minute = 0;
    int second = 0;
    char tail = '\0';
    struct tm tmValue;
    time_t parsedTime = 0;

    if (text == NULL || outTime == NULL) {
        return DATA_ERR_INVALID_ARG;
    }

    if (sscanf(text, "%d-%d-%d %d:%d:%d%c", &year, &month, &day, &hour, &minute, &second, &tail) != 6) {
        return DATA_ERR_TIME_PARSE;
    }

    memset(&tmValue, 0, sizeof(tmValue));
    tmValue.tm_year = year - 1900;
    tmValue.tm_mon = month - 1;
    tmValue.tm_mday = day;
    tmValue.tm_hour = hour;
    tmValue.tm_min = minute;
    tmValue.tm_sec = second;
    tmValue.tm_isdst = -1;

    parsedTime = mktime(&tmValue);
    if (parsedTime == (time_t)-1) {
        return DATA_ERR_TIME_PARSE;
    }

    if (tmValue.tm_year != year - 1900 || tmValue.tm_mon != month - 1 || tmValue.tm_mday != day ||
        tmValue.tm_hour != hour || tmValue.tm_min != minute || tmValue.tm_sec != second) {
        return DATA_ERR_TIME_PARSE;
    }

    *outTime = parsedTime;
    return DATA_OK;
}

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
    trimLineEnding(buffer);

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
    if (stringToTime(fields[1], &money.tTime) != DATA_OK) {
        return DATA_ERR_TIME_PARSE;
    }
    if (parseIntField(fields[2], &money.nStatus) != 0) {
        return DATA_ERR_RECORD_FORMAT;
    }
    if (parseInt32Field(fields[3], &money.nMoneyCent) != 0) {
        return DATA_ERR_RECORD_FORMAT;
    }
    if (parseIntField(fields[4], &money.nDel) != 0) {
        return DATA_ERR_RECORD_FORMAT;
    }

    *outMoney = money;
    return DATA_OK;
}

static DataResult ensureMoneyDataDir(void)
{
    char dirPath[INPUT_BUF_SIZE];
    char *slash = NULL;

    snprintf(dirPath, sizeof(dirPath), "%s", MONEY_DATA_FILE_PATH);
    slash = strrchr(dirPath, '/');
    if (slash == NULL) {
        return DATA_OK;
    }

    *slash = '\0';
    if (dirPath[0] == '\0') {
        return DATA_OK;
    }

    if (platformEnsureDirectoryExists(dirPath) != 0) {
        return DATA_ERR_FILE_OPEN;
    }

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

    ret = ensureMoneyDataDir();
    if (ret != DATA_OK) {
        return ret;
    }

    fp = fopen(MONEY_DATA_FILE_PATH, "a");
    if (fp == NULL) {
        return DATA_ERR_FILE_OPEN;
    }

    formatMoneyTimeString(money->tTime, timeBuf, sizeof(timeBuf));
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
