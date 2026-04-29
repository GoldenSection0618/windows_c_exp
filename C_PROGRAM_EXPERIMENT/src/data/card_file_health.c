#include "card_file_health.h"

#include "common.h"
#include "data_file_utils.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CARD_FILE_FIELD_COUNT 10

typedef struct SeenCardName {
    char cardName[CARD_NAME_MAX_LEN + 1];
} SeenCardName;

static int isBlankLine(const char *text)
{
    if (text == NULL) {
        return 1;
    }

    while (*text != '\0') {
        if (*text != ' ' && *text != '\t' && *text != '\r' && *text != '\n') {
            return 0;
        }
        text++;
    }

    return 1;
}

static void copyText(char *dst, size_t size, const char *src)
{
    if (dst == NULL || size == 0) {
        return;
    }

    if (src == NULL) {
        dst[0] = '\0';
        return;
    }

    snprintf(dst, size, "%s", src);
}

static void fillIssue(CardFileIssue *issue,
                      int lineNo,
                      CardFileIssueType type,
                      const char *cardName,
                      const char *reason,
                      const char *rawLine)
{
    if (issue == NULL) {
        return;
    }

    memset(issue, 0, sizeof(*issue));
    issue->lineNo = lineNo;
    issue->type = type;
    copyText(issue->cardName, sizeof(issue->cardName), cardName);
    copyText(issue->reason, sizeof(issue->reason), reason);
    copyText(issue->rawLine, sizeof(issue->rawLine), rawLine);
}

static void recordIssue(CardFileIssue *issues,
                        size_t capacity,
                        size_t *actualCount,
                        size_t *requiredCount,
                        const CardFileIssue *issue)
{
    if (requiredCount != NULL) {
        (*requiredCount)++;
    }

    if (issues != NULL && actualCount != NULL && *actualCount < capacity && issue != NULL) {
        issues[*actualCount] = *issue;
        (*actualCount)++;
    }
}

static int splitFields(char *line, char *fields[], int maxFields)
{
    int count = 0;
    char *cursor = line;
    char *separator = NULL;

    if (line == NULL || fields == NULL || maxFields <= 0) {
        return -1;
    }

    while (count < maxFields) {
        fields[count++] = cursor;
        separator = strchr(cursor, '|');
        if (separator == NULL) {
            break;
        }
        *separator = '\0';
        cursor = separator + 1;
    }

    if (strchr(cursor, '|') != NULL) {
        return maxFields + 1;
    }

    return count;
}

static int isSeenCardName(const SeenCardName *seenCards, size_t count, const char *cardName)
{
    size_t i = 0;

    if (seenCards == NULL || cardName == NULL) {
        return 0;
    }

    for (i = 0; i < count; i++) {
        if (strcmp(seenCards[i].cardName, cardName) == 0) {
            return 1;
        }
    }

    return 0;
}

static DataResult appendSeenCardName(SeenCardName **seenCards,
                                     size_t *count,
                                     size_t *capacity,
                                     const char *cardName)
{
    SeenCardName *newItems = NULL;
    size_t newCapacity = 0;

    if (seenCards == NULL || count == NULL || capacity == NULL || cardName == NULL) {
        return DATA_ERR_INVALID_ARG;
    }

    if (*count >= *capacity) {
        newCapacity = (*capacity == 0) ? 16 : (*capacity * 2);
        newItems = (SeenCardName *)realloc(*seenCards, newCapacity * sizeof(SeenCardName));
        if (newItems == NULL) {
            return DATA_ERR_NO_MEMORY;
        }
        *seenCards = newItems;
        *capacity = newCapacity;
    }

    copyText((*seenCards)[*count].cardName, sizeof((*seenCards)[*count].cardName), cardName);
    (*count)++;
    return DATA_OK;
}

static int validateCardNameField(const char *cardName, char *reason, size_t reasonSize)
{
    if (cardName == NULL || cardName[0] == '\0') {
        copyText(reason, reasonSize, "卡号为空");
        return 0;
    }
    if (strlen(cardName) > CARD_NAME_MAX_LEN) {
        copyText(reason, reasonSize, "卡号长度超过限制");
        return 0;
    }
    return 1;
}

static int validatePasswordField(const char *password, char *reason, size_t reasonSize)
{
    if (password == NULL || password[0] == '\0') {
        copyText(reason, reasonSize, "密码为空");
        return 0;
    }
    if (strlen(password) > CARD_PWD_MAX_LEN) {
        copyText(reason, reasonSize, "密码长度超过限制");
        return 0;
    }
    return 1;
}

static int validateFields(char *fields[], CardFileIssue *issue, int lineNo, const char *rawLine)
{
    int status = 0;
    int useCount = 0;
    int del = 0;
    int32_t amount = 0;
    time_t parsedTime = 0;
    char reason[CARD_FILE_ISSUE_REASON_LEN];

    if (!validateCardNameField(fields[0], reason, sizeof(reason))) {
        fillIssue(issue, lineNo, CARD_FILE_ISSUE_CARD_NAME, fields[0], reason, rawLine);
        return 0;
    }
    if (!validatePasswordField(fields[1], reason, sizeof(reason))) {
        fillIssue(issue, lineNo, CARD_FILE_ISSUE_PASSWORD, fields[0], reason, rawLine);
        return 0;
    }

    if (dataParseIntField(fields[2], &status) != 0 || status < CARD_STATUS_OFFLINE || status > CARD_STATUS_INVALID) {
        fillIssue(issue, lineNo, CARD_FILE_ISSUE_STATUS, fields[0], "卡状态字段非法", rawLine);
        return 0;
    }

    if (dataStringToTime(fields[3], &parsedTime) != DATA_OK || dataStringToTime(fields[4], &parsedTime) != DATA_OK ||
        dataStringToTime(fields[6], &parsedTime) != DATA_OK) {
        fillIssue(issue, lineNo, CARD_FILE_ISSUE_TIME, fields[0], "时间字段解析失败", rawLine);
        return 0;
    }

    if (dataParseInt32Field(fields[5], &amount) != 0 || amount < 0) {
        fillIssue(issue, lineNo, CARD_FILE_ISSUE_AMOUNT, fields[0], "累计使用金额字段非法", rawLine);
        return 0;
    }
    if (dataParseIntField(fields[7], &useCount) != 0 || useCount < 0) {
        fillIssue(issue, lineNo, CARD_FILE_ISSUE_AMOUNT, fields[0], "使用次数字段非法", rawLine);
        return 0;
    }
    if (dataParseInt32Field(fields[8], &amount) != 0 || amount < 0) {
        fillIssue(issue, lineNo, CARD_FILE_ISSUE_AMOUNT, fields[0], "余额字段非法", rawLine);
        return 0;
    }
    if (dataParseIntField(fields[9], &del) != 0 || (del != 0 && del != 1)) {
        fillIssue(issue, lineNo, CARD_FILE_ISSUE_DELETE_FLAG, fields[0], "删除标识字段非法", rawLine);
        return 0;
    }

    return 1;
}

const char *dataGetCardFileIssueTypeName(CardFileIssueType type)
{
    switch (type) {
    case CARD_FILE_ISSUE_FIELD_COUNT:
        return "FIELD_COUNT_ERROR";
    case CARD_FILE_ISSUE_CARD_NAME:
        return "CARD_NAME_ERROR";
    case CARD_FILE_ISSUE_PASSWORD:
        return "PASSWORD_ERROR";
    case CARD_FILE_ISSUE_STATUS:
        return "STATUS_ERROR";
    case CARD_FILE_ISSUE_TIME:
        return "TIME_ERROR";
    case CARD_FILE_ISSUE_AMOUNT:
        return "AMOUNT_ERROR";
    case CARD_FILE_ISSUE_DELETE_FLAG:
        return "DELETE_FLAG_ERROR";
    case CARD_FILE_ISSUE_DUPLICATE:
        return "DUPLICATE_CARD";
    case CARD_FILE_ISSUE_LINE_TOO_LONG:
        return "LINE_TOO_LONG";
    default:
        return "UNKNOWN_ERROR";
    }
}

DataResult dataCheckCardFile(CardFileHealthReport *report,
                             CardFileIssue *issues,
                             size_t capacity,
                             size_t *actualCount,
                             size_t *requiredCount)
{
    FILE *fp = NULL;
    char line[CARD_FILE_RAW_LINE_LEN];
    int lineNo = 0;
    SeenCardName *seenCards = NULL;
    size_t seenCount = 0;
    size_t seenCapacity = 0;
    DataResult ret = DATA_OK;

    if (report == NULL || actualCount == NULL || requiredCount == NULL) {
        return DATA_ERR_INVALID_ARG;
    }

    memset(report, 0, sizeof(*report));
    *actualCount = 0;
    *requiredCount = 0;

    fp = fopen(CARD_DATA_FILE_PATH, "r");
    if (fp == NULL) {
        return DATA_ERR_FILE_NOT_FOUND;
    }

    while (fgets(line, sizeof(line), fp) != NULL) {
        char rawLine[CARD_FILE_RAW_LINE_LEN];
        char parseLine[CARD_FILE_RAW_LINE_LEN];
        char *fields[CARD_FILE_FIELD_COUNT];
        int fieldCount = 0;
        CardFileIssue issue;
        int isInvalid = 0;

        lineNo++;
        report->totalLines++;
        copyText(rawLine, sizeof(rawLine), line);

        if (strchr(line, '\n') == NULL && !feof(fp)) {
            int ch = 0;
            fillIssue(&issue, lineNo, CARD_FILE_ISSUE_LINE_TOO_LONG, "", "记录行超过最大长度", rawLine);
            recordIssue(issues, capacity, actualCount, requiredCount, &issue);
            report->invalidLines++;
            report->lineTooLongCount++;
            do {
                ch = fgetc(fp);
            } while (ch != '\n' && ch != EOF);
            continue;
        }

        dataTrimLineEnding(rawLine);
        if (isBlankLine(rawLine)) {
            report->totalLines--;
            continue;
        }

        copyText(parseLine, sizeof(parseLine), rawLine);
        fieldCount = splitFields(parseLine, fields, CARD_FILE_FIELD_COUNT);
        if (fieldCount != CARD_FILE_FIELD_COUNT) {
            fillIssue(&issue, lineNo, CARD_FILE_ISSUE_FIELD_COUNT, "", "字段数量不是 10", rawLine);
            recordIssue(issues, capacity, actualCount, requiredCount, &issue);
            report->invalidLines++;
            continue;
        }

        if (!validateFields(fields, &issue, lineNo, rawLine)) {
            recordIssue(issues, capacity, actualCount, requiredCount, &issue);
            report->invalidLines++;
            continue;
        }

        if (isSeenCardName(seenCards, seenCount, fields[0])) {
            fillIssue(&issue, lineNo, CARD_FILE_ISSUE_DUPLICATE, fields[0], "卡号重复", rawLine);
            recordIssue(issues, capacity, actualCount, requiredCount, &issue);
            report->invalidLines++;
            report->duplicateCards++;
            continue;
        }

        ret = appendSeenCardName(&seenCards, &seenCount, &seenCapacity, fields[0]);
        if (ret != DATA_OK) {
            free(seenCards);
            fclose(fp);
            return ret;
        }

        isInvalid = 0;
        if (!isInvalid) {
            report->validLines++;
        }
    }

    if (ferror(fp)) {
        ret = DATA_ERR_FILE_OPEN;
    }

    if (fclose(fp) != 0) {
        ret = DATA_ERR_FILE_OPEN;
    }

    free(seenCards);
    return ret;
}

DataResult dataExportCardFileIssues(const CardFileIssue *issues, size_t count, const char *path)
{
    FILE *fp = NULL;
    size_t i = 0;
    DataResult ret = DATA_OK;

    if (path == NULL || *path == '\0') {
        return DATA_ERR_INVALID_ARG;
    }

    ret = dataEnsureDataDirByFilePath(path);
    if (ret != DATA_OK) {
        return ret;
    }

    fp = fopen(path, "w");
    if (fp == NULL) {
        return DATA_ERR_FILE_OPEN;
    }

    for (i = 0; i < count; i++) {
        if (fprintf(fp,
                    "Line %d | %s | %s | raw=%s\n",
                    issues[i].lineNo,
                    dataGetCardFileIssueTypeName(issues[i].type),
                    issues[i].reason,
                    issues[i].rawLine) < 0) {
            ret = DATA_ERR_FILE_OPEN;
            break;
        }
    }

    if (fclose(fp) != 0) {
        ret = DATA_ERR_FILE_OPEN;
    }

    return ret;
}

DataResult dataExportCardFileHealthReport(const CardFileHealthReport *report,
                                          const CardFileIssue *issues,
                                          size_t count,
                                          const char *path)
{
    FILE *fp = NULL;
    size_t i = 0;
    DataResult ret = DATA_OK;

    if (report == NULL || path == NULL || *path == '\0') {
        return DATA_ERR_INVALID_ARG;
    }

    ret = dataEnsureDataDirByFilePath(path);
    if (ret != DATA_OK) {
        return ret;
    }

    fp = fopen(path, "w");
    if (fp == NULL) {
        return DATA_ERR_FILE_OPEN;
    }

    if (fprintf(fp,
                "cards.txt 健康检查报告\n"
                "文件路径：%s\n"
                "扫描行数：%d\n"
                "有效记录：%d\n"
                "异常记录：%d\n"
                "重复卡号：%d\n"
                "超长记录：%d\n\n"
                "异常详情：\n",
                CARD_DATA_FILE_PATH,
                report->totalLines,
                report->validLines,
                report->invalidLines,
                report->duplicateCards,
                report->lineTooLongCount) < 0) {
        fclose(fp);
        return DATA_ERR_FILE_OPEN;
    }

    for (i = 0; i < count; i++) {
        if (fprintf(fp,
                    "Line %d | %s | %s\n",
                    issues[i].lineNo,
                    dataGetCardFileIssueTypeName(issues[i].type),
                    issues[i].reason) < 0) {
            ret = DATA_ERR_FILE_OPEN;
            break;
        }
    }

    if (fclose(fp) != 0) {
        ret = DATA_ERR_FILE_OPEN;
    }

    return ret;
}
