/*
 * 文件：data_file_utils.h
 * 作用：声明数据文件读写中的公共解析、时间格式化和目录保障工具。
 * 说明：这些工具服务 repository 层，不改变任何数据文件字段顺序或分隔格式。
 */
#ifndef DATA_FILE_UTILS_H
#define DATA_FILE_UTILS_H

#include <stddef.h>
#include <stdint.h>
#include <time.h>

#include "card_repository.h"

/* 去除文本行末尾的换行符，保留其他字段内容。 */
void dataTrimLineEnding(char *text);

/* 将整数字段解析为 int。 */
int dataParseIntField(const char *text, int *value);

/* 将金额等整数字段解析为 int32_t。 */
int dataParseInt32Field(const char *text, int32_t *value);

/* 按 repository 文件格式解析时间字段。 */
DataResult dataParseDateTime(const char *text, time_t *outTime);

/* 字符串转 time_t，失败时返回数据层错误码。 */
DataResult dataStringToTime(const char *text, time_t *outTime);

/* 将 time_t 格式化为数据文件中的时间字符串。 */
void dataFormatTimeString(time_t value, char *buffer, size_t size);

/* 格式化可为空的时间字段，0 值按约定输出。 */
void dataFormatOptionalTimeString(time_t value, char *buffer, size_t size);

/* 根据文件路径确保 data 目录存在。 */
DataResult dataEnsureDataDirByFilePath(const char *filePath);

#endif
