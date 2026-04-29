#ifndef CARD_FILE_MAINTENANCE_H
#define CARD_FILE_MAINTENANCE_H

#include <stddef.h>

#include "business.h"
#include "card_file_backup.h"
#include "card_file_health.h"

#ifdef __cplusplus
extern "C" {
#endif

#define CARD_FILE_ERROR_PATH "data/cards_error.txt"
#define CARD_FILE_HEALTH_REPORT_PATH "data/cards_health_report.txt"

typedef struct CardFileMaintenanceResult {
    CardFileHealthReport report;
    CardFileIssue *issues;
    size_t issueCount;
} CardFileMaintenanceResult;

BizResult bizCheckCardFileHealth(CardFileMaintenanceResult *result);
BizResult bizExportCardFileHealthReport(const CardFileMaintenanceResult *result);
BizResult bizBackupCardFile(char *outBackupPath, size_t outBackupPathSize);
BizResult bizRecoverCardFile(const char *backupPath);
void bizFreeCardFileMaintenanceResult(CardFileMaintenanceResult *result);

#ifdef __cplusplus
}
#endif

#endif
