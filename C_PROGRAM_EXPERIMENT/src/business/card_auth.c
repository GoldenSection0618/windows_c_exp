#include "business_internal.h"

#include "card_repository.h"
#include "card_storage_file.h"
#include "card_validator.h"
#include "common.h"

#include <stdio.h>
#include <string.h>

BizResult bizLoadCardByName(const char *cardNameInput, Card *outCard)
{
    char cardName[INPUT_BUF_SIZE];
    const Card *card = NULL;
    int readResult = 0;

    if (outCard == NULL) {
        return BIZ_ERR_SYSTEM;
    }

    if (validatorNormalizeInput(cardNameInput, cardName, sizeof(cardName)) != 0 ||
        !validatorIsValidCardName(cardName)) {
        return BIZ_ERR_INVALID_CARD_NAME;
    }

    readResult = dataLoadCards();
    if (readResult < 0) {
        return mapDataResult((DataResult)readResult);
    }

    card = dataQueryCardByName(cardName);
    if (card == NULL || card->nDel != 0) {
        return BIZ_ERR_CARD_NOT_FOUND;
    }

    *outCard = *card;
    return BIZ_OK;
}

BizResult bizLoadCardByCredentialInternal(const char *cardNameInput, const char *passwordInput, Card *outCard)
{
    char password[INPUT_BUF_SIZE];
    Card card;
    BizResult result = BIZ_OK;

    if (validatorNormalizeInput(passwordInput, password, sizeof(password)) != 0 ||
        !validatorIsValidPassword(password)) {
        return BIZ_ERR_INVALID_PASSWORD;
    }

    result = bizLoadCardByName(cardNameInput, &card);
    if (result != BIZ_OK) {
        return result;
    }

    if (strcmp(card.aPwd, password) != 0) {
        return BIZ_ERR_WRONG_PASSWORD;
    }

    if (outCard != NULL) {
        *outCard = card;
    }
    return BIZ_OK;
}

BizResult bizGetCardPasswordByName(const char *cardNameInput, char *passwordBuffer, size_t passwordBufferSize)
{
    Card card;
    BizResult result = BIZ_OK;

    if (passwordBuffer == NULL || passwordBufferSize == 0) {
        return BIZ_ERR_SYSTEM;
    }

    result = bizLoadCardByName(cardNameInput, &card);
    if (result != BIZ_OK) {
        return result;
    }

    if (snprintf(passwordBuffer, passwordBufferSize, "%s", card.aPwd) < 0) {
        return BIZ_ERR_SYSTEM;
    }

    return BIZ_OK;
}
