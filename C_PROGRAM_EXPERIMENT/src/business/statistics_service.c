#include "business.h"
#include "business_internal.h"

#include "billing_query_repository.h"
#include "card_validator.h"
#include "common.h"
#include "operation_log.h"
#include "time_validator.h"

#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

static BizResult parseStatisticsYearMonth(const char *yearMonthInput, int *year, int *month)
{
    char yearMonthText[INPUT_BUF_SIZE];
    char *endptr = NULL;
    long parsedYear = 0;
    long parsedMonth = 0;

    if (year == NULL || month == NULL) {
        return BIZ_ERR_SYSTEM;
    }

    if (validatorNormalizeInput(yearMonthInput, yearMonthText, sizeof(yearMonthText)) != 0 ||
        yearMonthText[0] == '\0') {
        return BIZ_ERR_INVALID_TIME_RANGE;
    }

    errno = 0;
    parsedYear = strtol(yearMonthText, &endptr, 10);
    if (errno != 0 || endptr == yearMonthText || *endptr != '-') {
        return BIZ_ERR_INVALID_TIME_RANGE;
    }

    errno = 0;
    parsedMonth = strtol(endptr + 1, &endptr, 10);
    if (errno != 0 || *endptr != '\0') {
        return BIZ_ERR_INVALID_TIME_RANGE;
    }

    if (parsedYear < 1970 || parsedYear > 9999 || parsedMonth < 1 || parsedMonth > 12) {
        return BIZ_ERR_INVALID_TIME_RANGE;
    }

    *year = (int)parsedYear;
    *month = (int)parsedMonth;
    return BIZ_OK;
}


BizResult bizAdminGetBillingStatistics(const LoginSession *session,
                                       const char *yearMonthInput,
                                       BillingStatistics *statistics)
{
    if (!bizIsAdminSession(session)) {
        return BIZ_ERR_SYSTEM;
    }

    return bizGetBillingStatistics(yearMonthInput, statistics);
}


BizResult bizQueryBillingsByCardName(const char *cardNameInput, BillingQueryResult *result)
{
    char cardName[INPUT_BUF_SIZE];
    DataResult dataResult = DATA_OK;

    if (result == NULL) {
        return BIZ_ERR_SYSTEM;
    }

    result->items = NULL;
    result->count = 0;

    if (validatorNormalizeInput(cardNameInput, cardName, sizeof(cardName)) != 0 ||
        !validatorIsValidCardName(cardName)) {
        return BIZ_ERR_INVALID_CARD_NAME;
    }

    dataResult = dataQueryBillingsByCardName(cardName, &result->items, &result->count);
    if (dataResult == DATA_ERR_NOT_FOUND || dataResult == DATA_ERR_FILE_NOT_FOUND) {
        return BIZ_ERR_BILLING_RECORD_NOT_FOUND;
    }
    if (dataResult != DATA_OK) {
        return mapDataResult(dataResult);
    }

    logOperation("按卡号查询消费记录");
    return BIZ_OK;
}


BizResult bizQueryBillingsByCardNameAndRange(const char *cardNameInput,
                                             const char *startInput,
                                             const char *endInput,
                                             BillingQueryResult *result)
{
    char cardName[INPUT_BUF_SIZE];
    time_t startTime = (time_t)0;
    time_t endTime = (time_t)0;
    DataResult dataResult = DATA_OK;

    if (result == NULL) {
        return BIZ_ERR_SYSTEM;
    }

    result->items = NULL;
    result->count = 0;

    if (validatorNormalizeInput(cardNameInput, cardName, sizeof(cardName)) != 0 ||
        !validatorIsValidCardName(cardName)) {
        return BIZ_ERR_INVALID_CARD_NAME;
    }

    if (timeValidatorParseDateTime(startInput, &startTime) != 0 ||
        timeValidatorParseDateTime(endInput, &endTime) != 0 ||
        startTime > endTime) {
        return BIZ_ERR_INVALID_TIME_RANGE;
    }

    dataResult = dataQueryBillingsByCardNameAndRange(cardName, startTime, endTime, &result->items, &result->count);
    if (dataResult == DATA_ERR_NOT_FOUND || dataResult == DATA_ERR_FILE_NOT_FOUND) {
        return BIZ_ERR_BILLING_RECORD_NOT_FOUND;
    }
    if (dataResult != DATA_OK) {
        return mapDataResult(dataResult);
    }

    logOperation("按卡号和时间段查询消费记录");
    return BIZ_OK;
}


void bizFreeBillingQueryResult(BillingQueryResult *result)
{
    if (result == NULL) {
        return;
    }

    dataFreeQueriedBillings(result->items);
    result->items = NULL;
    result->count = 0;
}


BizResult bizGetBillingStatistics(const char *yearMonthInput, BillingStatistics *statistics)
{
    Billing *records = NULL;
    size_t count = 0;
    size_t index = 0;
    int targetYear = 0;
    int targetMonth = 0;
    DataResult dataResult = DATA_OK;

    if (statistics == NULL) {
        return BIZ_ERR_SYSTEM;
    }

    memset(statistics, 0, sizeof(*statistics));

    if (parseStatisticsYearMonth(yearMonthInput, &targetYear, &targetMonth) != BIZ_OK) {
        return BIZ_ERR_INVALID_TIME_RANGE;
    }

    statistics->year = targetYear;
    statistics->month = targetMonth;

    dataResult = dataQueryAllBillings(&records, &count);
    if (dataResult == DATA_ERR_NOT_FOUND || dataResult == DATA_ERR_FILE_NOT_FOUND) {
        return BIZ_OK;
    }
    if (dataResult != DATA_OK) {
        return mapDataResult(dataResult);
    }

    for (index = 0; index < count; index++) {
        struct tm *localValue = NULL;
        int recordYear = 0;
        int recordMonth = 0;

        if (records[index].nStatus != 1 || records[index].nDel != 0 || records[index].nAmountCent <= 0) {
            continue;
        }

        localValue = localtime(&records[index].tEnd);
        if (localValue == NULL) {
            continue;
        }

        recordYear = localValue->tm_year + 1900;
        recordMonth = localValue->tm_mon + 1;
        if (recordYear == targetYear && recordMonth == targetMonth) {
            statistics->totalAmountCent += records[index].nAmountCent;
            statistics->monthlyAmountCent[targetMonth - 1] += records[index].nAmountCent;
        }
    }

    dataFreeQueriedBillings(records);
    logOperation("按月营业额统计");
    return BIZ_OK;
}


void bizStatistics(void)
{
    printf("[业务逻辑层] 查询统计功能入口。\n");
    logOperation("查询统计");
}

