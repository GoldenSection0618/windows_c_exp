#ifndef CARD_VALIDATOR_H
#define CARD_VALIDATOR_H

#include <stddef.h>
#include <stdint.h>

typedef enum MoneyParseResult {
    MONEY_PARSE_OK = 0,
    MONEY_PARSE_INVALID = -1,
    MONEY_PARSE_TOO_LARGE = -2
} MoneyParseResult;

int validatorNormalizeInput(const char *input, char *buffer, size_t size);
int validatorIsValidCardName(const char *cardName);
int validatorIsValidPassword(const char *password);
MoneyParseResult validatorParseMoneyToCent(const char *text, int32_t *amountCent);

#endif
