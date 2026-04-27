#include "card_query_repository.h"

#include "common.h"
#include "time_validator.h"

#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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

    *value = (int32_t)parsedValue;
    return 0;
}

static DataResult parseCardLine(const char *line, Card *outCard)
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
    if (timeValidatorParseDateTime(fields[3], &card.tStart) != 0) {
        return DATA_ERR_TIME_PARSE;
    }
    if (timeValidatorParseDateTime(fields[4], &card.tEnd) != 0) {
        return DATA_ERR_TIME_PARSE;
    }
    if (parseInt32Field(fields[5], &card.nTotalUseCent) != 0) {
        return DATA_ERR_RECORD_FORMAT;
    }
    if (timeValidatorParseDateTime(fields[6], &card.tLast) != 0) {
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

DataResult dataQueryAllCardsSnapshot(Card **cards, size_t *count)
{
    FILE *fp = NULL;
    char line[256];
    Card *buffer = NULL;
    size_t capacity = 0;
    size_t used = 0;

    if (cards == NULL || count == NULL) {
        return DATA_ERR_INVALID_ARG;
    }

    *cards = NULL;
    *count = 0;

    fp = fopen(CARD_DATA_FILE_PATH, "r");
    if (fp == NULL) {
        return errno == ENOENT ? DATA_ERR_FILE_NOT_FOUND : DATA_ERR_FILE_OPEN;
    }

    while (fgets(line, sizeof(line), fp) != NULL) {
        Card card;
        DataResult result = DATA_OK;

        if (strchr(line, '\n') == NULL && !feof(fp)) {
            free(buffer);
            fclose(fp);
            return DATA_ERR_RECORD_FORMAT;
        }

        trimLineEnding(line);
        if (line[0] == '\0') {
            continue;
        }

        result = parseCardLine(line, &card);
        if (result != DATA_OK) {
            free(buffer);
            fclose(fp);
            return result;
        }

        if (used == capacity) {
            size_t newCapacity = capacity == 0 ? 8 : capacity * 2;
            Card *newBuffer = (Card *)realloc(buffer, newCapacity * sizeof(Card));
            if (newBuffer == NULL) {
                free(buffer);
                fclose(fp);
                return DATA_ERR_NO_MEMORY;
            }
            buffer = newBuffer;
            capacity = newCapacity;
        }

        buffer[used] = card;
        used++;
    }

    if (fclose(fp) != 0) {
        free(buffer);
        return DATA_ERR_FILE_OPEN;
    }

    if (used == 0) {
        free(buffer);
        return DATA_ERR_NOT_FOUND;
    }

    *cards = buffer;
    *count = used;
    return DATA_OK;
}

void dataFreeCardsSnapshot(Card *cards)
{
    free(cards);
}
