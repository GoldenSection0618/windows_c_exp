#ifndef DATA_FILE_UTILS_H
#define DATA_FILE_UTILS_H

#include <stddef.h>
#include <stdint.h>
#include <time.h>

#include "card_repository.h"

void dataTrimLineEnding(char *text);
int dataParseIntField(const char *text, int *value);
int dataParseInt32Field(const char *text, int32_t *value);
DataResult dataParseDateTime(const char *text, time_t *outTime);
DataResult dataStringToTime(const char *text, time_t *outTime);
void dataFormatTimeString(time_t value, char *buffer, size_t size);
void dataFormatOptionalTimeString(time_t value, char *buffer, size_t size);
DataResult dataEnsureDataDirByFilePath(const char *filePath);

#endif
