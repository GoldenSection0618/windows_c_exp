#include "data_file_utils.h"

#include "common.h"
#include "platform.h"

#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void dataTrimLineEnding(char *text)
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

int dataParseIntField(const char *text, int *value)
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

int dataParseInt32Field(const char *text, int32_t *value)
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

DataResult dataParseDateTime(const char *text, time_t *outTime)
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

DataResult dataStringToTime(const char *text, time_t *outTime)
{
    return dataParseDateTime(text, outTime);
}

void dataFormatTimeString(time_t value, char *buffer, size_t size)
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

void dataFormatOptionalTimeString(time_t value, char *buffer, size_t size)
{
    if (buffer == NULL || size == 0) {
        return;
    }

    if (value == (time_t)0) {
        snprintf(buffer, size, "0");
        return;
    }

    dataFormatTimeString(value, buffer, size);
}

DataResult dataEnsureDataDirByFilePath(const char *filePath)
{
    char dirPath[INPUT_BUF_SIZE];
    char *slash = NULL;

    if (filePath == NULL) {
        return DATA_ERR_INVALID_ARG;
    }

    snprintf(dirPath, sizeof(dirPath), "%s", filePath);
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
