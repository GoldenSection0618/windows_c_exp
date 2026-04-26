#include "card_repository.h"
#include "card_storage_file.h"
#include "card_storage.h"
#include "common.h"
#include "platform.h"

#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static CardNode *g_pCardListHead = NULL;
static size_t g_cardCount = 0;

static DataResult stringToTime(const char *text, time_t *outTime);
static DataResult praseCard(const char *line, Card *outCard);
static DataResult rewriteCardFile(void);
static DataResult ensureCardDataDir(void);
static void trimLineEnding(char *text);
static void formatTimeString(time_t value, char *buffer, size_t size);
static int parseIntField(const char *text, int *value);
static int parseInt32Field(const char *text, int32_t *value);
static int isCardNameEqual(const char *a, const char *b);
static int doesCardNameContainKeyword(const char *cardName, const char *keyword);
static CardNode *findCardNodeByName(const char *cardName);
static size_t countCardsByKeyword(const char *keyword);
static size_t copyCardsByKeyword(const char *keyword, Card *outCards, size_t capacity);

static int isCardNameEqual(const char *a, const char *b)
{
    if (a == NULL || b == NULL) {
        return 0;
    }
    return strcmp(a, b) == 0;
}

static int doesCardNameContainKeyword(const char *cardName, const char *keyword)
{
    if (cardName == NULL || keyword == NULL || *keyword == '\0') {
        return 0;
    }

    return strstr(cardName, keyword) != NULL;
}

static CardNode *findCardNodeByName(const char *cardName)
{
    CardNode *pCurrent = g_pCardListHead;

    if (cardName == NULL) {
        return NULL;
    }

    while (pCurrent != NULL) {
        if (pCurrent->cardData.nDel == 0 && isCardNameEqual(pCurrent->cardData.aCardName, cardName)) {
            return pCurrent;
        }
        pCurrent = pCurrent->pNext;
    }

    return NULL;
}

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

static void formatTimeString(time_t value, char *buffer, size_t size)
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

static DataResult ensureCardDataDir(void)
{
    char dirPath[INPUT_BUF_SIZE];
    char *slash = NULL;

    snprintf(dirPath, sizeof(dirPath), "%s", CARD_DATA_FILE_PATH);
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

static DataResult praseCard(const char *line, Card *outCard)
{
    char buffer[256];
    char *fields[10];
    char *cursor = NULL;
    char *separator = NULL;
    int index = 0;
    Card card;

    if (line == NULL || outCard == NULL) {
        return DATA_ERR_INVALID_ARG;
    }

    if (snprintf(buffer, sizeof(buffer), "%s", line) >= (int)sizeof(buffer)) {
        return DATA_ERR_RECORD_FORMAT;
    }
    trimLineEnding(buffer);

    cursor = buffer;
    for (index = 0; index < 9; index++) {
        separator = strchr(cursor, '|');
        if (separator == NULL) {
            return DATA_ERR_RECORD_FORMAT;
        }
        *separator = '\0';
        fields[index] = cursor;
        cursor = separator + 1;
    }
    fields[9] = cursor;

    if (strchr(fields[9], '|') != NULL) {
        return DATA_ERR_RECORD_FORMAT;
    }

    for (index = 0; index < 10; index++) {
        if (fields[index][0] == '\0') {
            return DATA_ERR_RECORD_FORMAT;
        }
    }

    memset(&card, 0, sizeof(card));
    if (snprintf(card.aCardName, sizeof(card.aCardName), "%s", fields[0]) >= (int)sizeof(card.aCardName)) {
        return DATA_ERR_RECORD_FORMAT;
    }
    if (snprintf(card.aPwd, sizeof(card.aPwd), "%s", fields[1]) >= (int)sizeof(card.aPwd)) {
        return DATA_ERR_RECORD_FORMAT;
    }
    if (parseIntField(fields[2], &card.nStatus) != 0) {
        return DATA_ERR_RECORD_FORMAT;
    }
    if (stringToTime(fields[3], &card.tStart) != DATA_OK) {
        return DATA_ERR_TIME_PARSE;
    }
    if (stringToTime(fields[4], &card.tEnd) != DATA_OK) {
        return DATA_ERR_TIME_PARSE;
    }
    if (parseInt32Field(fields[5], &card.nTotalUseCent) != 0) {
        return DATA_ERR_RECORD_FORMAT;
    }
    if (stringToTime(fields[6], &card.tLast) != DATA_OK) {
        return DATA_ERR_TIME_PARSE;
    }
    if (parseIntField(fields[7], &card.nUseCount) != 0) {
        return DATA_ERR_RECORD_FORMAT;
    }
    if (parseInt32Field(fields[8], &card.nBalanceCent) != 0) {
        return DATA_ERR_RECORD_FORMAT;
    }
    if (parseIntField(fields[9], &card.nDel) != 0) {
        return DATA_ERR_RECORD_FORMAT;
    }

    *outCard = card;
    return DATA_OK;
}

static DataResult rewriteCardFile(void)
{
    FILE *fp = NULL;
    CardNode *pCurrent = g_pCardListHead;
    char startBuf[CARD_TIME_STR_LEN + 1];
    char endBuf[CARD_TIME_STR_LEN + 1];
    char lastBuf[CARD_TIME_STR_LEN + 1];
    DataResult ret = DATA_OK;

    ret = ensureCardDataDir();
    if (ret != DATA_OK) {
        return ret;
    }

    fp = fopen(CARD_DATA_FILE_PATH, "w");
    if (fp == NULL) {
        return DATA_ERR_FILE_OPEN;
    }

    while (pCurrent != NULL) {
        formatTimeString(pCurrent->cardData.tStart, startBuf, sizeof(startBuf));
        formatTimeString(pCurrent->cardData.tEnd, endBuf, sizeof(endBuf));
        formatTimeString(pCurrent->cardData.tLast, lastBuf, sizeof(lastBuf));

        if (fprintf(fp,
                    "%s|%s|%d|%s|%s|%d|%s|%d|%d|%d\n",
                    pCurrent->cardData.aCardName,
                    pCurrent->cardData.aPwd,
                    pCurrent->cardData.nStatus,
                    startBuf,
                    endBuf,
                    pCurrent->cardData.nTotalUseCent,
                    lastBuf,
                    pCurrent->cardData.nUseCount,
                    pCurrent->cardData.nBalanceCent,
                    pCurrent->cardData.nDel) < 0) {
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

const Card *dataQueryCardByName(const char *cardName)
{
    CardNode *pNode = findCardNodeByName(cardName);

    if (pNode == NULL) {
        return NULL;
    }
    return &pNode->cardData;
}

static size_t countCardsByKeyword(const char *keyword)
{
    CardNode *pCurrent = g_pCardListHead;
    size_t count = 0;

    if (keyword == NULL || *keyword == '\0') {
        return 0;
    }

    while (pCurrent != NULL) {
        if (pCurrent->cardData.nDel == 0 &&
            doesCardNameContainKeyword(pCurrent->cardData.aCardName, keyword)) {
            count++;
        }
        pCurrent = pCurrent->pNext;
    }

    return count;
}

static size_t copyCardsByKeyword(const char *keyword, Card *outCards, size_t capacity)
{
    CardNode *pCurrent = g_pCardListHead;
    size_t count = 0;

    if (keyword == NULL || *keyword == '\0' || outCards == NULL || capacity == 0) {
        return 0;
    }

    while (pCurrent != NULL && count < capacity) {
        if (pCurrent->cardData.nDel == 0 &&
            doesCardNameContainKeyword(pCurrent->cardData.aCardName, keyword)) {
            outCards[count] = pCurrent->cardData;
            count++;
        }
        pCurrent = pCurrent->pNext;
    }

    return count;
}

DataResult dataDeleteCardByName(const char *cardName)
{
    CardNode *pCurrent = g_pCardListHead;
    CardNode *pPrev = NULL;

    if (cardName == NULL || *cardName == '\0') {
        return DATA_ERR_INVALID_ARG;
    }

    while (pCurrent != NULL) {
        if (isCardNameEqual(pCurrent->cardData.aCardName, cardName)) {
            if (pPrev == NULL) {
                g_pCardListHead = pCurrent->pNext;
            } else {
                pPrev->pNext = pCurrent->pNext;
            }

            free(pCurrent);
            if (g_cardCount > 0) {
                g_cardCount--;
            }
            return DATA_OK;
        }

        pPrev = pCurrent;
        pCurrent = pCurrent->pNext;
    }

    return DATA_ERR_NOT_FOUND;
}

int dataAddCard(const Card *card)
{
    CardNode *pNewNode = NULL;
    CardNode *pTail = NULL;

    if (card == NULL) {
        return DATA_ERR_INVALID_ARG;
    }

    if (findCardNodeByName(card->aCardName) != NULL) {
        return DATA_ERR_DUPLICATE;
    }

    pNewNode = (CardNode *)malloc(sizeof(CardNode));
    if (pNewNode == NULL) {
        return DATA_ERR_NO_MEMORY;
    }

    pNewNode->cardData = *card;
    pNewNode->pNext = NULL;

    if (g_pCardListHead == NULL) {
        g_pCardListHead = pNewNode;
    } else {
        pTail = g_pCardListHead;
        while (pTail->pNext != NULL) {
            pTail = pTail->pNext;
        }
        pTail->pNext = pNewNode;
    }

    g_cardCount++;
    return DATA_OK;
}

DataResult dataSaveCard(const Card *card)
{
    FILE *fp = NULL;
    char startBuf[CARD_TIME_STR_LEN + 1];
    char endBuf[CARD_TIME_STR_LEN + 1];
    char lastBuf[CARD_TIME_STR_LEN + 1];
    DataResult ret = DATA_OK;

    if (card == NULL) {
        return DATA_ERR_INVALID_ARG;
    }

    ret = ensureCardDataDir();
    if (ret != DATA_OK) {
        return ret;
    }

    fp = fopen(CARD_DATA_FILE_PATH, "a");
    if (fp == NULL) {
        return DATA_ERR_FILE_OPEN;
    }

    formatTimeString(card->tStart, startBuf, sizeof(startBuf));
    formatTimeString(card->tEnd, endBuf, sizeof(endBuf));
    formatTimeString(card->tLast, lastBuf, sizeof(lastBuf));

    if (fprintf(fp,
                "%s|%s|%d|%s|%s|%d|%s|%d|%d|%d\n",
                card->aCardName,
                card->aPwd,
                card->nStatus,
                startBuf,
                endBuf,
                card->nTotalUseCent,
                lastBuf,
                card->nUseCount,
                card->nBalanceCent,
                card->nDel) < 0) {
        fclose(fp);
        return DATA_ERR_FILE_OPEN;
    }

    if (fclose(fp) != 0) {
        return DATA_ERR_FILE_OPEN;
    }

    return DATA_OK;
}

int dataLoadCards(void)
{
    FILE *fp = NULL;
    char line[256];
    int count = 0;
    DataResult ret = DATA_OK;

    dataCleanup();

    fp = fopen(CARD_DATA_FILE_PATH, "r");
    if (fp == NULL) {
        if (errno == ENOENT) {
            return DATA_ERR_FILE_NOT_FOUND;
        }
        return DATA_ERR_FILE_OPEN;
    }

    while (fgets(line, sizeof(line), fp) != NULL) {
        Card card;

        if (strchr(line, '\n') == NULL && !feof(fp)) {
            fclose(fp);
            dataCleanup();
            return DATA_ERR_RECORD_FORMAT;
        }

        trimLineEnding(line);
        if (line[0] == '\0') {
            continue;
        }

        ret = praseCard(line, &card);
        if (ret != DATA_OK) {
            fclose(fp);
            dataCleanup();
            return ret;
        }

        ret = (DataResult)dataAddCard(&card);
        if (ret != DATA_OK) {
            fclose(fp);
            dataCleanup();
            return (ret == DATA_ERR_DUPLICATE) ? DATA_ERR_RECORD_FORMAT : ret;
        }

        count++;
    }

    if (fclose(fp) != 0) {
        dataCleanup();
        return DATA_ERR_FILE_OPEN;
    }

    return count;
}

int dataGetCardCount(void)
{
    FILE *fp = NULL;
    char line[256];
    int count = 0;

    fp = fopen(CARD_DATA_FILE_PATH, "r");
    if (fp == NULL) {
        if (errno == ENOENT) {
            return 0;
        }
        return DATA_ERR_FILE_OPEN;
    }

    while (fgets(line, sizeof(line), fp) != NULL) {
        Card card;
        DataResult ret = DATA_OK;

        if (strchr(line, '\n') == NULL && !feof(fp)) {
            fclose(fp);
            return DATA_ERR_RECORD_FORMAT;
        }

        trimLineEnding(line);
        if (line[0] == '\0') {
            continue;
        }

        ret = praseCard(line, &card);
        if (ret != DATA_OK) {
            fclose(fp);
            return ret;
        }
        count++;
    }

    if (fclose(fp) != 0) {
        return DATA_ERR_FILE_OPEN;
    }

    return count;
}

int dataCardExists(const char *cardName)
{
    if (cardName == NULL || *cardName == '\0') {
        return 0;
    }

    return findCardNodeByName(cardName) != NULL;
}

DataResult dataUpdateCard(const Card *card)
{
    CardNode *pNode = NULL;
    int readResult = 0;

    if (card == NULL) {
        return DATA_ERR_INVALID_ARG;
    }

    readResult = dataLoadCards();
    if (readResult < 0) {
        return (DataResult)readResult;
    }

    pNode = findCardNodeByName(card->aCardName);
    if (pNode == NULL) {
        return DATA_ERR_NOT_FOUND;
    }

    pNode->cardData = *card;
    return rewriteCardFile();
}

DataResult dataQueryCardsByKeyword(const char *keyword,
                                   Card *outCards,
                                   size_t capacity,
                                   size_t *actualCount,
                                   size_t *requiredCount)
{
    size_t count = 0;

    if (keyword == NULL || *keyword == '\0' || actualCount == NULL || requiredCount == NULL) {
        return DATA_ERR_INVALID_ARG;
    }

    *actualCount = 0;
    *requiredCount = 0;

    count = countCardsByKeyword(keyword);
    *requiredCount = count;

    if (count == 0) {
        return DATA_OK;
    }

    if (outCards == NULL || capacity == 0) {
        return DATA_OK;
    }

    if (capacity < count) {
        return DATA_ERR_INVALID_ARG;
    }

    *actualCount = copyCardsByKeyword(keyword, outCards, capacity);
    if (*actualCount != count) {
        return DATA_ERR_INVALID_ARG;
    }

    return DATA_OK;
}

void dataCleanup(void)
{
    CardNode *pCurrent = g_pCardListHead;

    while (pCurrent != NULL) {
        CardNode *pNext = pCurrent->pNext;
        free(pCurrent);
        pCurrent = pNext;
    }

    g_pCardListHead = NULL;
    g_cardCount = 0;
}
