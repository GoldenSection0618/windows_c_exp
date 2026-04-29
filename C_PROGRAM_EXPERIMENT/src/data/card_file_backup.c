#include "card_file_backup.h"

#include "common.h"
#include "data_file_utils.h"
#include "platform.h"

#include <stdio.h>
#include <time.h>

static int isFileReadable(const char *path)
{
    FILE *fp = NULL;

    if (path == NULL || *path == '\0') {
        return 0;
    }

    fp = fopen(path, "rb");
    if (fp == NULL) {
        return 0;
    }

    fclose(fp);
    return 1;
}

DataResult dataCopyFileContent(const char *srcPath, const char *dstPath)
{
    FILE *src = NULL;
    FILE *dst = NULL;
    unsigned char buffer[4096];
    size_t readCount = 0;
    DataResult ret = DATA_OK;

    if (srcPath == NULL || dstPath == NULL || *srcPath == '\0' || *dstPath == '\0') {
        return DATA_ERR_INVALID_ARG;
    }

    src = fopen(srcPath, "rb");
    if (src == NULL) {
        return DATA_ERR_FILE_NOT_FOUND;
    }

    ret = dataEnsureDataDirByFilePath(dstPath);
    if (ret != DATA_OK) {
        fclose(src);
        return ret;
    }

    dst = fopen(dstPath, "wb");
    if (dst == NULL) {
        fclose(src);
        return DATA_ERR_FILE_OPEN;
    }

    while ((readCount = fread(buffer, 1, sizeof(buffer), src)) > 0) {
        if (fwrite(buffer, 1, readCount, dst) != readCount) {
            ret = DATA_ERR_FILE_OPEN;
            break;
        }
    }

    if (ferror(src)) {
        ret = DATA_ERR_FILE_OPEN;
    }

    if (fclose(dst) != 0) {
        ret = DATA_ERR_FILE_OPEN;
    }
    if (fclose(src) != 0) {
        ret = DATA_ERR_FILE_OPEN;
    }

    return ret;
}

DataResult dataBackupCardFile(char *outBackupPath, size_t outBackupPathSize)
{
    time_t now = 0;
    struct tm *localValue = NULL;
    char timeBuffer[32];
    char backupPath[CARD_FILE_BACKUP_PATH_LEN];

    if (outBackupPath == NULL || outBackupPathSize == 0) {
        return DATA_ERR_INVALID_ARG;
    }
    outBackupPath[0] = '\0';

    if (!isFileReadable(CARD_DATA_FILE_PATH)) {
        return DATA_ERR_FILE_NOT_FOUND;
    }

    if (platformEnsureDirectoryExists("data") != 0) {
        return DATA_ERR_FILE_OPEN;
    }
    if (platformEnsureDirectoryExists(CARD_FILE_BACKUP_DIR) != 0) {
        return DATA_ERR_FILE_OPEN;
    }

    now = time(NULL);
    localValue = localtime(&now);
    if (localValue == NULL || strftime(timeBuffer, sizeof(timeBuffer), "%Y%m%d_%H%M%S", localValue) == 0) {
        return DATA_ERR_TIME_PARSE;
    }

    if (snprintf(backupPath, sizeof(backupPath), "%s/cards_%s.bak", CARD_FILE_BACKUP_DIR, timeBuffer) >=
        (int)sizeof(backupPath)) {
        return DATA_ERR_INVALID_ARG;
    }

    if (snprintf(outBackupPath, outBackupPathSize, "%s", backupPath) >= (int)outBackupPathSize) {
        return DATA_ERR_INVALID_ARG;
    }

    return dataCopyFileContent(CARD_DATA_FILE_PATH, backupPath);
}

DataResult dataRecoverCardFile(const char *backupPath)
{
    char restoreBackupPath[CARD_FILE_BACKUP_PATH_LEN];
    DataResult ret = DATA_OK;

    if (backupPath == NULL || *backupPath == '\0') {
        return DATA_ERR_INVALID_ARG;
    }
    if (!isFileReadable(backupPath)) {
        return DATA_ERR_FILE_NOT_FOUND;
    }

    if (isFileReadable(CARD_DATA_FILE_PATH)) {
        ret = dataBackupCardFile(restoreBackupPath, sizeof(restoreBackupPath));
        if (ret != DATA_OK) {
            return ret;
        }
    }

    return dataCopyFileContent(backupPath, CARD_DATA_FILE_PATH);
}
