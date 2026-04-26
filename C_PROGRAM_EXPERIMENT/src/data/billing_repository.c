#include "billing_repository.h"
#include "billing_storage.h"
#include "billing_storage_file.h"
#include "common.h"
#include "platform.h"

#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static BillingNode *g_pBillingListHead = NULL;
static size_t g_billingCount = 0;

static DataResult ensureBillingDataDir(void);
static void trimLineEnding(char *text);
static void formatBillingTimeString(time_t value, char *buffer, size_t size);
static int parseIntField(const char *text, int *value);
static int parseInt32Field(const char *text, int32_t *value);
static DataResult parseBillingTimeField(const char *text, time_t *outTime);
static DataResult stringToTime(const char *text, time_t *outTime);
static DataResult parseBilling(const char *line, Billing *outBilling);
static DataResult rewriteBillingFile(void);
static int isBillingCardNameEqual(const char *a, const char *b);
static BillingNode *findBillingNodeByCardNameAndStart(const char *cardName, time_t tStart);
static int billingMatchesCardName(const Billing *billing, const char *cardName);
static int billingMatchesRange(const Billing *billing, time_t startTime, time_t endTime);
static size_t countMatchedBillings(const char *cardName, int useRange, time_t startTime, time_t endTime);
static size_t copyMatchedBillings(const char *cardName,
                                  int useRange,
                                  time_t startTime,
                                  time_t endTime,
                                  Billing *records,
                                  size_t capacity);
static size_t countAllBillings(void);
static size_t copyAllBillings(Billing *records, size_t capacity);

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

static void formatBillingTimeString(time_t value, char *buffer, size_t size)
{
    struct tm *localValue = NULL;

    if (buffer == NULL || size == 0) {
        return;
    }

    if (value == (time_t)0) {
        snprintf(buffer, size, "0");
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

static DataResult parseBillingTimeField(const char *text, time_t *outTime)
{
    if (text == NULL || outTime == NULL) {
        return DATA_ERR_INVALID_ARG;
    }

    if (strcmp(text, "0") == 0) {
        *outTime = (time_t)0;
        return DATA_OK;
    }

    return stringToTime(text, outTime);
}

static DataResult parseBilling(const char *line, Billing *outBilling)
{
    char buffer[256];
    char *fields[6];
    char *cursor = NULL;
    char *separator = NULL;
    int index = 0;
    Billing billing;

    if (line == NULL || outBilling == NULL) {
        return DATA_ERR_INVALID_ARG;
    }

    if (snprintf(buffer, sizeof(buffer), "%s", line) >= (int)sizeof(buffer)) {
        return DATA_ERR_RECORD_FORMAT;
    }
    trimLineEnding(buffer);

    cursor = buffer;
    for (index = 0; index < 5; index++) {
        separator = strchr(cursor, '|');
        if (separator == NULL) {
            return DATA_ERR_RECORD_FORMAT;
        }
        *separator = '\0';
        fields[index] = cursor;
        cursor = separator + 1;
    }
    fields[5] = cursor;

    if (strchr(fields[5], '|') != NULL) {
        return DATA_ERR_RECORD_FORMAT;
    }

    for (index = 0; index < 6; index++) {
        if (fields[index][0] == '\0') {
            return DATA_ERR_RECORD_FORMAT;
        }
    }

    memset(&billing, 0, sizeof(billing));
    if (snprintf(billing.aCardName, sizeof(billing.aCardName), "%s", fields[0]) >= (int)sizeof(billing.aCardName)) {
        return DATA_ERR_RECORD_FORMAT;
    }
    if (parseBillingTimeField(fields[1], &billing.tStart) != DATA_OK) {
        return DATA_ERR_TIME_PARSE;
    }
    if (parseBillingTimeField(fields[2], &billing.tEnd) != DATA_OK) {
        return DATA_ERR_TIME_PARSE;
    }
    if (parseInt32Field(fields[3], &billing.nAmountCent) != 0) {
        return DATA_ERR_RECORD_FORMAT;
    }
    if (parseIntField(fields[4], &billing.nStatus) != 0) {
        return DATA_ERR_RECORD_FORMAT;
    }
    if (parseIntField(fields[5], &billing.nDel) != 0) {
        return DATA_ERR_RECORD_FORMAT;
    }

    *outBilling = billing;
    return DATA_OK;
}

static DataResult ensureBillingDataDir(void)
{
    char dirPath[INPUT_BUF_SIZE];
    char *slash = NULL;

    snprintf(dirPath, sizeof(dirPath), "%s", BILLING_DATA_FILE_PATH);
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

static int isBillingCardNameEqual(const char *a, const char *b)
{
    if (a == NULL || b == NULL) {
        return 0;
    }
    return strcmp(a, b) == 0;
}

static BillingNode *findBillingNodeByCardNameAndStart(const char *cardName, time_t tStart)
{
    BillingNode *pCurrent = g_pBillingListHead;

    while (pCurrent != NULL) {
        if (pCurrent->billingData.nDel == 0 &&
            isBillingCardNameEqual(pCurrent->billingData.aCardName, cardName) &&
            pCurrent->billingData.tStart == tStart) {
            return pCurrent;
        }
        pCurrent = pCurrent->pNext;
    }

    return NULL;
}

static int billingMatchesCardName(const Billing *billing, const char *cardName)
{
    if (billing == NULL || cardName == NULL || *cardName == '\0') {
        return 0;
    }

    return billing->nDel == 0 && isBillingCardNameEqual(billing->aCardName, cardName);
}

static int billingMatchesRange(const Billing *billing, time_t startTime, time_t endTime)
{
    if (billing == NULL) {
        return 0;
    }

    return billing->tStart >= startTime && billing->tStart <= endTime;
}

static size_t countMatchedBillings(const char *cardName, int useRange, time_t startTime, time_t endTime)
{
    BillingNode *pCurrent = g_pBillingListHead;
    size_t count = 0;

    while (pCurrent != NULL) {
        if (billingMatchesCardName(&pCurrent->billingData, cardName) &&
            (!useRange || billingMatchesRange(&pCurrent->billingData, startTime, endTime))) {
            count++;
        }
        pCurrent = pCurrent->pNext;
    }

    return count;
}

static size_t copyMatchedBillings(const char *cardName,
                                  int useRange,
                                  time_t startTime,
                                  time_t endTime,
                                  Billing *records,
                                  size_t capacity)
{
    BillingNode *pCurrent = g_pBillingListHead;
    size_t copied = 0;

    while (pCurrent != NULL && copied < capacity) {
        if (billingMatchesCardName(&pCurrent->billingData, cardName) &&
            (!useRange || billingMatchesRange(&pCurrent->billingData, startTime, endTime))) {
            records[copied] = pCurrent->billingData;
            copied++;
        }
        pCurrent = pCurrent->pNext;
    }

    return copied;
}

static size_t countAllBillings(void)
{
    BillingNode *pCurrent = g_pBillingListHead;
    size_t count = 0;

    while (pCurrent != NULL) {
        if (pCurrent->billingData.nDel == 0) {
            count++;
        }
        pCurrent = pCurrent->pNext;
    }

    return count;
}

static size_t copyAllBillings(Billing *records, size_t capacity)
{
    BillingNode *pCurrent = g_pBillingListHead;
    size_t copied = 0;

    while (pCurrent != NULL && copied < capacity) {
        if (pCurrent->billingData.nDel == 0) {
            records[copied] = pCurrent->billingData;
            copied++;
        }
        pCurrent = pCurrent->pNext;
    }

    return copied;
}

int dataAddBilling(const Billing *billing)
{
    BillingNode *pNewNode = NULL;
    BillingNode *pTail = NULL;

    if (billing == NULL) {
        return DATA_ERR_INVALID_ARG;
    }

    pNewNode = (BillingNode *)malloc(sizeof(BillingNode));
    if (pNewNode == NULL) {
        return DATA_ERR_NO_MEMORY;
    }

    pNewNode->billingData = *billing;
    pNewNode->pNext = NULL;

    if (g_pBillingListHead == NULL) {
        g_pBillingListHead = pNewNode;
    } else {
        pTail = g_pBillingListHead;
        while (pTail->pNext != NULL) {
            pTail = pTail->pNext;
        }
        pTail->pNext = pNewNode;
    }

    g_billingCount++;
    return DATA_OK;
}

const Billing *dataQueryLatestUnsettledBillingByCardName(const char *cardName)
{
    BillingNode *pCurrent = g_pBillingListHead;
    const Billing *pMatched = NULL;

    if (cardName == NULL || *cardName == '\0') {
        return NULL;
    }

    while (pCurrent != NULL) {
        if (pCurrent->billingData.nDel == 0 &&
            pCurrent->billingData.nStatus == 0 &&
            isBillingCardNameEqual(pCurrent->billingData.aCardName, cardName)) {
            if (pMatched == NULL || pCurrent->billingData.tStart > pMatched->tStart) {
                pMatched = &pCurrent->billingData;
            }
        }
        pCurrent = pCurrent->pNext;
    }

    return pMatched;
}

DataResult dataQueryBillingsByCardName(const char *cardName, Billing **records, size_t *count)
{
    int loadResult = 0;
    size_t matchedCount = 0;
    Billing *buffer = NULL;

    if (cardName == NULL || *cardName == '\0' || records == NULL || count == NULL) {
        return DATA_ERR_INVALID_ARG;
    }

    *records = NULL;
    *count = 0;

    loadResult = dataLoadBillings();
    if (loadResult < 0) {
        return (DataResult)loadResult;
    }

    matchedCount = countMatchedBillings(cardName, 0, (time_t)0, (time_t)0);
    if (matchedCount == 0) {
        return DATA_ERR_NOT_FOUND;
    }

    buffer = (Billing *)malloc(matchedCount * sizeof(Billing));
    if (buffer == NULL) {
        return DATA_ERR_NO_MEMORY;
    }

    if (copyMatchedBillings(cardName, 0, (time_t)0, (time_t)0, buffer, matchedCount) != matchedCount) {
        free(buffer);
        return DATA_ERR_FILE_OPEN;
    }

    *records = buffer;
    *count = matchedCount;
    return DATA_OK;
}

DataResult dataQueryBillingsByCardNameAndRange(const char *cardName,
                                               time_t startTime,
                                               time_t endTime,
                                               Billing **records,
                                               size_t *count)
{
    int loadResult = 0;
    size_t matchedCount = 0;
    Billing *buffer = NULL;

    if (cardName == NULL || *cardName == '\0' || records == NULL || count == NULL || startTime > endTime) {
        return DATA_ERR_INVALID_ARG;
    }

    *records = NULL;
    *count = 0;

    loadResult = dataLoadBillings();
    if (loadResult < 0) {
        return (DataResult)loadResult;
    }

    matchedCount = countMatchedBillings(cardName, 1, startTime, endTime);
    if (matchedCount == 0) {
        return DATA_ERR_NOT_FOUND;
    }

    buffer = (Billing *)malloc(matchedCount * sizeof(Billing));
    if (buffer == NULL) {
        return DATA_ERR_NO_MEMORY;
    }

    if (copyMatchedBillings(cardName, 1, startTime, endTime, buffer, matchedCount) != matchedCount) {
        free(buffer);
        return DATA_ERR_FILE_OPEN;
    }

    *records = buffer;
    *count = matchedCount;
    return DATA_OK;
}

DataResult dataQueryAllBillings(Billing **records, size_t *count)
{
    int loadResult = 0;
    size_t matchedCount = 0;
    Billing *buffer = NULL;

    if (records == NULL || count == NULL) {
        return DATA_ERR_INVALID_ARG;
    }

    *records = NULL;
    *count = 0;

    loadResult = dataLoadBillings();
    if (loadResult < 0) {
        return (DataResult)loadResult;
    }

    matchedCount = countAllBillings();
    if (matchedCount == 0) {
        return DATA_ERR_NOT_FOUND;
    }

    buffer = (Billing *)malloc(matchedCount * sizeof(Billing));
    if (buffer == NULL) {
        return DATA_ERR_NO_MEMORY;
    }

    if (copyAllBillings(buffer, matchedCount) != matchedCount) {
        free(buffer);
        return DATA_ERR_FILE_OPEN;
    }

    *records = buffer;
    *count = matchedCount;
    return DATA_OK;
}

void dataFreeQueriedBillings(Billing *records)
{
    free(records);
}

void dataCleanupBillings(void)
{
    BillingNode *pCurrent = g_pBillingListHead;

    while (pCurrent != NULL) {
        BillingNode *pNext = pCurrent->pNext;
        free(pCurrent);
        pCurrent = pNext;
    }

    g_pBillingListHead = NULL;
    g_billingCount = 0;
}

DataResult dataSaveBilling(const Billing *billing)
{
    FILE *fp = NULL;
    char startBuf[CARD_TIME_STR_LEN + 1];
    char endBuf[CARD_TIME_STR_LEN + 1];
    DataResult ret = DATA_OK;

    if (billing == NULL) {
        return DATA_ERR_INVALID_ARG;
    }

    ret = ensureBillingDataDir();
    if (ret != DATA_OK) {
        return ret;
    }

    fp = fopen(BILLING_DATA_FILE_PATH, "a");
    if (fp == NULL) {
        return DATA_ERR_FILE_OPEN;
    }

    formatBillingTimeString(billing->tStart, startBuf, sizeof(startBuf));
    formatBillingTimeString(billing->tEnd, endBuf, sizeof(endBuf));

    if (fprintf(fp,
                "%s|%s|%s|%d|%d|%d\n",
                billing->aCardName,
                startBuf,
                endBuf,
                billing->nAmountCent,
                billing->nStatus,
                billing->nDel) < 0) {
        fclose(fp);
        return DATA_ERR_FILE_OPEN;
    }

    if (fclose(fp) != 0) {
        return DATA_ERR_FILE_OPEN;
    }

    return DATA_OK;
}

int dataLoadBillings(void)
{
    FILE *fp = NULL;
    char line[256];
    int count = 0;

    dataCleanupBillings();

    fp = fopen(BILLING_DATA_FILE_PATH, "r");
    if (fp == NULL) {
        if (errno == ENOENT) {
            return DATA_ERR_FILE_NOT_FOUND;
        }
        return DATA_ERR_FILE_OPEN;
    }

    while (fgets(line, sizeof(line), fp) != NULL) {
        Billing billing;
        DataResult ret = parseBilling(line, &billing);

        if (ret != DATA_OK) {
            fclose(fp);
            dataCleanupBillings();
            return ret;
        }

        ret = (DataResult)dataAddBilling(&billing);
        if (ret != DATA_OK) {
            fclose(fp);
            dataCleanupBillings();
            return ret;
        }

        count++;
    }

    if (ferror(fp)) {
        fclose(fp);
        dataCleanupBillings();
        return DATA_ERR_FILE_OPEN;
    }

    if (fclose(fp) != 0) {
        dataCleanupBillings();
        return DATA_ERR_FILE_OPEN;
    }

    return count;
}

int dataGetBillingCount(void)
{
    FILE *fp = NULL;
    char line[256];
    int count = 0;

    fp = fopen(BILLING_DATA_FILE_PATH, "r");
    if (fp == NULL) {
        if (errno == ENOENT) {
            return DATA_ERR_FILE_NOT_FOUND;
        }
        return DATA_ERR_FILE_OPEN;
    }

    while (fgets(line, sizeof(line), fp) != NULL) {
        Billing billing;
        DataResult ret = parseBilling(line, &billing);

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

static DataResult rewriteBillingFile(void)
{
    FILE *fp = NULL;
    BillingNode *pCurrent = g_pBillingListHead;
    char startBuf[CARD_TIME_STR_LEN + 1];
    char endBuf[CARD_TIME_STR_LEN + 1];
    DataResult ret = DATA_OK;

    ret = ensureBillingDataDir();
    if (ret != DATA_OK) {
        return ret;
    }

    fp = fopen(BILLING_DATA_FILE_PATH, "w");
    if (fp == NULL) {
        return DATA_ERR_FILE_OPEN;
    }

    while (pCurrent != NULL) {
        formatBillingTimeString(pCurrent->billingData.tStart, startBuf, sizeof(startBuf));
        formatBillingTimeString(pCurrent->billingData.tEnd, endBuf, sizeof(endBuf));

        if (fprintf(fp,
                    "%s|%s|%s|%d|%d|%d\n",
                    pCurrent->billingData.aCardName,
                    startBuf,
                    endBuf,
                    pCurrent->billingData.nAmountCent,
                    pCurrent->billingData.nStatus,
                    pCurrent->billingData.nDel) < 0) {
            fclose(fp);
            return DATA_ERR_FILE_OPEN;
        }
        pCurrent = pCurrent->pNext;
    }

    if (fclose(fp) != 0) {
        return DATA_ERR_FILE_OPEN;
    }

    return DATA_OK;
}

DataResult dataUpdateBilling(const Billing *billing)
{
    BillingNode *pNode = NULL;

    if (billing == NULL) {
        return DATA_ERR_INVALID_ARG;
    }

    pNode = findBillingNodeByCardNameAndStart(billing->aCardName, billing->tStart);
    if (pNode == NULL) {
        return DATA_ERR_NOT_FOUND;
    }

    pNode->billingData = *billing;
    return rewriteBillingFile();
}
