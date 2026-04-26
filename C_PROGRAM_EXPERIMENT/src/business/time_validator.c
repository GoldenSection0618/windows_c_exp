#include "time_validator.h"

#include "card_validator.h"
#include "common.h"

#include <stdio.h>
#include <string.h>
#include <time.h>

int timeValidatorParseDateTime(const char *input, time_t *outTime)
{
    char buffer[INPUT_BUF_SIZE];
    int year = 0;
    int month = 0;
    int day = 0;
    int hour = 0;
    int minute = 0;
    int second = 0;
    char tail = '\0';
    struct tm tmValue;
    time_t parsedTime = 0;

    if (input == NULL || outTime == NULL) {
        return -1;
    }

    if (validatorNormalizeInput(input, buffer, sizeof(buffer)) != 0 || buffer[0] == '\0') {
        return -1;
    }

    if (sscanf(buffer,
               "%d-%d-%d %d:%d:%d%c",
               &year,
               &month,
               &day,
               &hour,
               &minute,
               &second,
               &tail) != 6) {
        return -1;
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
        return -1;
    }

    if (tmValue.tm_year != year - 1900 || tmValue.tm_mon != month - 1 || tmValue.tm_mday != day ||
        tmValue.tm_hour != hour || tmValue.tm_min != minute || tmValue.tm_sec != second) {
        return -1;
    }

    *outTime = parsedTime;
    return 0;
}
