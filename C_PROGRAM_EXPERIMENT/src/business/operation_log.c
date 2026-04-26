#include "operation_log.h"
#include "common.h"

#include <stdio.h>

void logOperation(const char *operation)
{
#if ENABLE_LOG
    printf("[系统日志] 操作记录：%s\n", operation);
#else
    (void)operation;
#endif
}
