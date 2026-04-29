#ifndef CARD_FILE_BACKUP_H
#define CARD_FILE_BACKUP_H

#include <stddef.h>

#include "card_repository.h"

#ifdef __cplusplus
extern "C" {
#endif

#define CARD_FILE_BACKUP_PATH_LEN 260
#define CARD_FILE_BACKUP_DIR "data/backup"

DataResult dataBackupCardFile(char *outBackupPath, size_t outBackupPathSize);
DataResult dataRecoverCardFile(const char *backupPath);
DataResult dataCopyFileContent(const char *srcPath, const char *dstPath);

#ifdef __cplusplus
}
#endif

#endif
