#ifndef CARD_FILE_HEALTH_H
#define CARD_FILE_HEALTH_H

#include <stddef.h>

#include "card_repository.h"
#include "model.h"

#ifdef __cplusplus
extern "C" {
#endif

#define CARD_FILE_ISSUE_REASON_LEN 128
#define CARD_FILE_RAW_LINE_LEN 256
#define CARD_FILE_PATH_LEN 260

typedef enum CardFileIssueType {
    CARD_FILE_ISSUE_FIELD_COUNT = 1,
    CARD_FILE_ISSUE_CARD_NAME,
    CARD_FILE_ISSUE_PASSWORD,
    CARD_FILE_ISSUE_STATUS,
    CARD_FILE_ISSUE_TIME,
    CARD_FILE_ISSUE_AMOUNT,
    CARD_FILE_ISSUE_DELETE_FLAG,
    CARD_FILE_ISSUE_DUPLICATE,
    CARD_FILE_ISSUE_LINE_TOO_LONG
} CardFileIssueType;

typedef struct CardFileIssue {
    int lineNo;
    CardFileIssueType type;
    char cardName[CARD_NAME_MAX_LEN + 1];
    char reason[CARD_FILE_ISSUE_REASON_LEN];
    char rawLine[CARD_FILE_RAW_LINE_LEN];
} CardFileIssue;

typedef struct CardFileHealthReport {
    int totalLines;
    int validLines;
    int invalidLines;
    int duplicateCards;
    int lineTooLongCount;
} CardFileHealthReport;

DataResult dataCheckCardFile(CardFileHealthReport *report,
                             CardFileIssue *issues,
                             size_t capacity,
                             size_t *actualCount,
                             size_t *requiredCount);

DataResult dataExportCardFileIssues(const CardFileIssue *issues,
                                    size_t count,
                                    const char *path);

DataResult dataExportCardFileHealthReport(const CardFileHealthReport *report,
                                          const CardFileIssue *issues,
                                          size_t count,
                                          const char *path);

const char *dataGetCardFileIssueTypeName(CardFileIssueType type);

#ifdef __cplusplus
}
#endif

#endif
