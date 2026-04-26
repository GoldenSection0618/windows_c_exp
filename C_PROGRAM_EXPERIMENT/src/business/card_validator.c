#include "card_validator.h"

#include "common.h"
#include "model.h"

#include <ctype.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>

static void trimInPlace(char *text)
{
    size_t len = 0;
    size_t start = 0;
    size_t end = 0;

    if (text == NULL) {
        return;
    }

    len = strlen(text);
    if (len == 0) {
        return;
    }

    while (start < len && isspace((unsigned char)text[start])) {
        start++;
    }

    if (start == len) {
        text[0] = '\0';
        return;
    }

    end = len;
    while (end > start && isspace((unsigned char)text[end - 1])) {
        end--;
    }

    if (start > 0) {
        memmove(text, text + start, end - start);
    }
    text[end - start] = '\0';
}

int validatorNormalizeInput(const char *input, char *buffer, size_t size)
{
    if (input == NULL || buffer == NULL || size == 0) {
        return -1;
    }

    if (snprintf(buffer, size, "%s", input) >= (int)size) {
        return -1;
    }

    trimInPlace(buffer);
    return 0;
}

int validatorIsValidCardName(const char *cardName)
{
    size_t len = 0;

    if (cardName == NULL) {
        return 0;
    }

    len = strlen(cardName);
    if (!(len > 0 && len <= CARD_NAME_MAX_LEN)) {
        return 0;
    }

    while (*cardName != '\0') {
        unsigned char ch = (unsigned char)*cardName;

        if (!(isalnum(ch) || ch == '_' || ch == '@' || ch == '#' || ch == '$' || ch == '%' || ch == '!')) {
            return 0;
        }
        cardName++;
    }

    return 1;
}

int validatorIsValidPassword(const char *password)
{
    size_t len = 0;

    if (password == NULL) {
        return 0;
    }

    len = strlen(password);
    if (!(len > 0 && len <= CARD_PWD_MAX_LEN)) {
        return 0;
    }

    while (*password != '\0') {
        unsigned char ch = (unsigned char)*password;

        if (!(isalnum(ch) || ch == '_' || ch == '@' || ch == '#' || ch == '$' || ch == '%' || ch == '!')) {
            return 0;
        }
        password++;
    }

    return 1;
}

MoneyParseResult validatorParseMoneyToCent(const char *text, int32_t *amountCent)
{
    const char *p = text;
    int64_t yuan = 0;
    int32_t frac = 0;
    int fracDigits = 0;
    int64_t totalCent = 0;
    int digit = 0;

    if (text == NULL || amountCent == NULL) {
        return MONEY_PARSE_INVALID;
    }

    if (*p == '\0') {
        return MONEY_PARSE_INVALID;
    }

    while (*p >= '0' && *p <= '9') {
        digit = *p - '0';
        if (yuan > (INT64_MAX - digit) / 10) {
            return MONEY_PARSE_TOO_LARGE;
        }
        yuan = yuan * 10 + digit;
        p++;
    }

    if (p == text) {
        return MONEY_PARSE_INVALID;
    }

    if (*p == '.') {
        p++;
        while (*p >= '0' && *p <= '9') {
            if (fracDigits >= 2) {
                return MONEY_PARSE_INVALID;
            }
            frac = (int32_t)(frac * 10 + (*p - '0'));
            fracDigits++;
            p++;
        }
    }

    if (*p != '\0') {
        return MONEY_PARSE_INVALID;
    }

    if (fracDigits == 1) {
        frac *= 10;
    }

    if (yuan > (INT64_MAX - frac) / 100) {
        return MONEY_PARSE_TOO_LARGE;
    }
    totalCent = yuan * 100 + frac;
    if (totalCent < 0) {
        return MONEY_PARSE_INVALID;
    }
    if (totalCent >= MAX_BALANCE_CENT || totalCent > INT32_MAX) {
        return MONEY_PARSE_TOO_LARGE;
    }

    *amountCent = (int32_t)totalCent;
    return MONEY_PARSE_OK;
}
