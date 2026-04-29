#include "card_file_maintenance_ui.h"

#include "card_file_maintenance.h"
#include "common.h"
#include "menu.h"

#include <stdio.h>
#include <string.h>

static void outputCardFileMaintenanceMenu(void)
{
    printf("========== 卡数据文件维护 ==========\n");
    printf("1. 健康检查 cards.txt\n");
    printf("2. 导出健康检查报告\n");
    printf("3. 手动备份 cards.txt\n");
    printf("4. 从备份恢复 cards.txt\n");
    printf("0. 返回上级菜单\n");
}

static void printHealthResult(const CardFileMaintenanceResult *result)
{
    if (result == NULL) {
        return;
    }

    printf("========== cards.txt 健康检查结果 ==========\n");
    printf("文件路径：%s\n", "data/cards.txt");
    printf("扫描行数：%d\n", result->report.totalLines);
    printf("有效记录：%d\n", result->report.validLines);
    printf("异常记录：%d\n", result->report.invalidLines);
    printf("重复卡号：%d\n", result->report.duplicateCards);
    printf("超长记录：%d\n", result->report.lineTooLongCount);
    printf("状态：%s\n", result->report.invalidLines == 0 ? "正常" : "发现异常");
    printf("异常记录隔离文件：%s\n", CARD_FILE_ERROR_PATH);
}

static void handleHealthCheck(CardFileMaintenanceResult *cachedResult, int *hasCachedResult)
{
    BizResult result = BIZ_OK;

    if (cachedResult == NULL || hasCachedResult == NULL) {
        return;
    }

    bizFreeCardFileMaintenanceResult(cachedResult);
    *hasCachedResult = 0;

    result = bizCheckCardFileHealth(cachedResult);
    if (result != BIZ_OK) {
        printf("健康检查失败：%s\n", bizGetMessage(result));
        return;
    }

    *hasCachedResult = 1;
    printHealthResult(cachedResult);
}

static void handleExportReport(CardFileMaintenanceResult *cachedResult, int *hasCachedResult)
{
    BizResult result = BIZ_OK;

    if (cachedResult == NULL || hasCachedResult == NULL) {
        return;
    }

    if (!*hasCachedResult) {
        result = bizCheckCardFileHealth(cachedResult);
        if (result != BIZ_OK) {
            printf("导出失败：%s\n", bizGetMessage(result));
            return;
        }
        *hasCachedResult = 1;
    }

    result = bizExportCardFileHealthReport(cachedResult);
    if (result == BIZ_OK) {
        printf("健康检查报告已导出到：%s\n", CARD_FILE_HEALTH_REPORT_PATH);
    } else {
        printf("导出失败：%s\n", bizGetMessage(result));
    }
}

static void handleBackup(void)
{
    char backupPath[CARD_FILE_BACKUP_PATH_LEN];
    BizResult result = bizBackupCardFile(backupPath, sizeof(backupPath));

    if (result == BIZ_OK) {
        printf("备份成功：%s\n", backupPath);
    } else {
        printf("备份失败：%s\n", bizGetMessage(result));
    }
}

static void handleRecover(void)
{
    char backupPath[CARD_FILE_BACKUP_PATH_LEN];
    char confirm[INPUT_BUF_SIZE];
    BizResult result = BIZ_OK;

    if (readTextInput("请输入备份文件路径：", backupPath, sizeof(backupPath)) != 0 || backupPath[0] == '\0') {
        printf("备份文件路径输入无效。\n");
        return;
    }

    printf("该操作会用备份文件覆盖当前 data/cards.txt。\n");
    if (readTextInput("请输入 YES 确认恢复：", confirm, sizeof(confirm)) != 0 || strcmp(confirm, "YES") != 0) {
        printf("已取消恢复操作。\n");
        return;
    }

    result = bizRecoverCardFile(backupPath);
    if (result == BIZ_OK) {
        printf("恢复成功。\n");
    } else {
        printf("恢复失败：%s\n", bizGetMessage(result));
    }
}

void handleCardFileMaintenanceInteraction(void)
{
    int choice = -1;
    CardFileMaintenanceResult cachedResult;
    int hasCachedResult = 0;

    memset(&cachedResult, 0, sizeof(cachedResult));

    do {
        outputCardFileMaintenanceMenu();
        if (readChoiceInput("请输入菜单编号：", &choice) != 0) {
            printf("输入格式错误，请输入数字菜单编号（0~4）。\n");
            continue;
        }

        switch (choice) {
        case 1:
            handleHealthCheck(&cachedResult, &hasCachedResult);
            break;
        case 2:
            handleExportReport(&cachedResult, &hasCachedResult);
            break;
        case 3:
            handleBackup();
            break;
        case 4:
            handleRecover();
            break;
        case 0:
            break;
        default:
            printf("无效菜单编号，请输入 0~4。\n");
            break;
        }
    } while (choice != 0);

    bizFreeCardFileMaintenanceResult(&cachedResult);
}
