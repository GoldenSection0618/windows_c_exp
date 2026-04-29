#include "card_file_maintenance.h"

#include "business_internal.h"

#include <stdlib.h>
#include <string.h>

BizResult bizCheckCardFileHealth(CardFileMaintenanceResult *result)
{
    size_t actualCount = 0;
    size_t requiredCount = 0;
    DataResult dataResult = DATA_OK;

    if (result == NULL) {
        return BIZ_ERR_SYSTEM;
    }

    memset(result, 0, sizeof(*result));

    dataResult = dataCheckCardFile(&result->report, NULL, 0, &actualCount, &requiredCount);
    if (dataResult != DATA_OK) {
        return mapDataResult(dataResult);
    }

    if (requiredCount > 0) {
        result->issues = (CardFileIssue *)malloc(requiredCount * sizeof(CardFileIssue));
        if (result->issues == NULL) {
            return BIZ_ERR_NO_MEMORY;
        }

        memset(&result->report, 0, sizeof(result->report));
        actualCount = 0;
        dataResult = dataCheckCardFile(&result->report, result->issues, requiredCount, &actualCount, &requiredCount);
        if (dataResult != DATA_OK) {
            bizFreeCardFileMaintenanceResult(result);
            return mapDataResult(dataResult);
        }
        result->issueCount = actualCount;
    }

    dataResult = dataExportCardFileIssues(result->issues, result->issueCount, CARD_FILE_ERROR_PATH);
    if (dataResult != DATA_OK) {
        bizFreeCardFileMaintenanceResult(result);
        return mapDataResult(dataResult);
    }

    return BIZ_OK;
}

BizResult bizExportCardFileHealthReport(const CardFileMaintenanceResult *result)
{
    DataResult dataResult = DATA_OK;

    if (result == NULL) {
        return BIZ_ERR_SYSTEM;
    }

    dataResult = dataExportCardFileHealthReport(&result->report,
                                                result->issues,
                                                result->issueCount,
                                                CARD_FILE_HEALTH_REPORT_PATH);
    return mapDataResult(dataResult);
}

BizResult bizBackupCardFile(char *outBackupPath, size_t outBackupPathSize)
{
    return mapDataResult(dataBackupCardFile(outBackupPath, outBackupPathSize));
}

BizResult bizRecoverCardFile(const char *backupPath)
{
    return mapDataResult(dataRecoverCardFile(backupPath));
}

void bizFreeCardFileMaintenanceResult(CardFileMaintenanceResult *result)
{
    if (result == NULL) {
        return;
    }

    free(result->issues);
    memset(result, 0, sizeof(*result));
}
