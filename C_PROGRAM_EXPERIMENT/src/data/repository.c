#include "card_repository.h"
#include "card_storage_file.h"
#include "card_storage.h"
#include "common.h"
#include "data_file_utils.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static CardNode *g_pCardListHead = NULL;
static size_t g_cardCount = 0;

static DataResult praseCard(const char *line, Card *outCard);
static DataResult rewriteCardFile(void);
static int isCardNameEqual(const char *a, const char *b);
static int doesCardNameContainKeyword(const char *cardName, const char *keyword);
static CardNode *findCardNodeByName(const char *cardName);
static size_t countCardsByKeyword(const char *keyword);
static size_t copyCardsByKeyword(const char *keyword, Card *outCards, size_t capacity);
static size_t countVisibleCards(void);
static size_t copyVisibleCards(Card *outCards, size_t capacity);

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
    dataTrimLineEnding(buffer);

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
    if (dataParseIntField(fields[2], &card.nStatus) != 0) {
        return DATA_ERR_RECORD_FORMAT;
    }
    if (dataStringToTime(fields[3], &card.tStart) != DATA_OK) {
        return DATA_ERR_TIME_PARSE;
    }
    if (dataStringToTime(fields[4], &card.tEnd) != DATA_OK) {
        return DATA_ERR_TIME_PARSE;
    }
    if (dataParseInt32Field(fields[5], &card.nTotalUseCent) != 0) {
        return DATA_ERR_RECORD_FORMAT;
    }
    if (dataStringToTime(fields[6], &card.tLast) != DATA_OK) {
        return DATA_ERR_TIME_PARSE;
    }
    if (dataParseIntField(fields[7], &card.nUseCount) != 0) {
        return DATA_ERR_RECORD_FORMAT;
    }
    if (dataParseInt32Field(fields[8], &card.nBalanceCent) != 0) {
        return DATA_ERR_RECORD_FORMAT;
    }
    if (dataParseIntField(fields[9], &card.nDel) != 0) {
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

    ret = dataEnsureDataDirByFilePath(CARD_DATA_FILE_PATH);
    if (ret != DATA_OK) {
        return ret;
    }

    fp = fopen(CARD_DATA_FILE_PATH, "w");
    if (fp == NULL) {
        return DATA_ERR_FILE_OPEN;
    }

    while (pCurrent != NULL) {
        dataFormatTimeString(pCurrent->cardData.tStart, startBuf, sizeof(startBuf));
        dataFormatTimeString(pCurrent->cardData.tEnd, endBuf, sizeof(endBuf));
        dataFormatTimeString(pCurrent->cardData.tLast, lastBuf, sizeof(lastBuf));

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

static size_t countVisibleCards(void)
{
    CardNode *pCurrent = g_pCardListHead;
    size_t count = 0;

    while (pCurrent != NULL) {
        if (pCurrent->cardData.nDel == 0) {
            count++;
        }
        pCurrent = pCurrent->pNext;
    }

    return count;
}

static size_t copyVisibleCards(Card *outCards, size_t capacity)
{
    CardNode *pCurrent = g_pCardListHead;
    size_t count = 0;

    if (outCards == NULL || capacity == 0) {
        return 0;
    }

    while (pCurrent != NULL && count < capacity) {
        if (pCurrent->cardData.nDel == 0) {
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

    ret = dataEnsureDataDirByFilePath(CARD_DATA_FILE_PATH);
    if (ret != DATA_OK) {
        return ret;
    }

    fp = fopen(CARD_DATA_FILE_PATH, "a");
    if (fp == NULL) {
        return DATA_ERR_FILE_OPEN;
    }

    dataFormatTimeString(card->tStart, startBuf, sizeof(startBuf));
    dataFormatTimeString(card->tEnd, endBuf, sizeof(endBuf));
    dataFormatTimeString(card->tLast, lastBuf, sizeof(lastBuf));

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

        dataTrimLineEnding(line);
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

        dataTrimLineEnding(line);
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

DataResult dataQueryAllCards(Card *outCards,
                             size_t capacity,
                             size_t *actualCount,
                             size_t *requiredCount)
{
    size_t count = 0;

    if (actualCount == NULL || requiredCount == NULL) {
        return DATA_ERR_INVALID_ARG;
    }

    *actualCount = 0;
    *requiredCount = 0;

    count = countVisibleCards();
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

    *actualCount = copyVisibleCards(outCards, capacity);
    return (*actualCount == count) ? DATA_OK : DATA_ERR_INVALID_ARG;
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
