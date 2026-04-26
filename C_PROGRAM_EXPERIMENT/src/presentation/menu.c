#include "menu.h"
#include "business.h"
#include "card_ui.h"
#include "common.h"

#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void discardRemainingInputLine(void)
{
    int ch = 0;

    do {
        ch = getchar();
    } while (ch != '\n' && ch != EOF);
}

int readTextInput(const char *prompt, char *buffer, size_t size)
{
    char *newLine = NULL;

    if (prompt == NULL || buffer == NULL || size == 0) {
        return -1;
    }

    printf("%s", prompt);
    if (fgets(buffer, (int)size, stdin) == NULL) {
        return -1;
    }

    newLine = strchr(buffer, '\n');
    if (newLine == NULL) {
        discardRemainingInputLine();
        return -1;
    }

    *newLine = '\0';
    return 0;
}

void outputMenu(void)
{
    printf("%s\n", STUDENT_INFO);
    printf("========== %s ==========\n", SYSTEM_NAME);
    printf("1. 添加卡\n");
    printf("2. 查询卡\n");
    printf("3. 上机\n");
    printf("4. 下机\n");
    printf("5. 充值\n");
    printf("6. 退费\n");
    printf("7. 查询统计\n");
    printf("8. 注销卡\n");
    printf("0. 退出系统\n");
}

int readChoiceInput(const char *prompt, int *choice)
{
    char buffer[INPUT_BUF_SIZE];
    char *endptr = NULL;
    long value = 0;

    if (choice == NULL) {
        return -1;
    }

    if (readTextInput(prompt, buffer, sizeof(buffer)) != 0) {
        return -1;
    }

    errno = 0;
    value = strtol(buffer, &endptr, 10);

    if (endptr == buffer || errno == ERANGE) {
        return -1;
    }

    while (*endptr != '\0' && isspace((unsigned char)*endptr)) {
        endptr++;
    }

    if (*endptr != '\0') {
        return -1;
    }

    if (value < INT_MIN || value > INT_MAX) {
        return -1;
    }

    *choice = (int)value;
    return 0;
}

int readMenuChoice(int *choice)
{
    return readChoiceInput("请输入菜单编号：", choice);
}

void showMenuInputFormatError(void)
{
    printf("输入格式错误，请输入数字菜单编号（0~8）。\n");
}

void dispatchMenuChoice(int choice)
{
    switch (choice) {
    case 1:
        printf("你选择了：添加卡\n");
        handleAddCardInteraction();
        break;
    case 2:
        printf("你选择了：查询卡\n");
        handleQueryCardInteraction();
        break;
    case 3:
        printf("你选择了：上机\n");
        handleStartBillingInteraction();
        break;
    case 4:
        printf("你选择了：下机\n");
        handleStopBillingInteraction();
        break;
    case 5:
        printf("你选择了：充值\n");
        handleRechargeInteraction();
        break;
    case 6:
        printf("你选择了：退费\n");
        handleRefundInteraction();
        break;
    case 7:
        printf("你选择了：查询统计\n");
        handleBillingQueryInteraction();
        break;
    case 8:
        printf("你选择了：注销卡\n");
        handleCancelCardInteraction();
        break;
    case 0:
        printf("系统已退出。\n");
        break;
    default:
        printf("无效菜单编号，请输入 0~8。\n");
        break;
    }
}
