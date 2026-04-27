#include "business.h"

#include "card_repository.h"
#include "card_storage_file.h"
#include "card_validator.h"
#include "common.h"

#include <stdio.h>
#include <string.h>

static BizResult sessionMapDataResult(DataResult result)
{
    switch (result) {
    case DATA_OK:
        return BIZ_OK;
    case DATA_ERR_DUPLICATE:
        return BIZ_ERR_DUPLICATE_CARD;
    case DATA_ERR_NO_MEMORY:
        return BIZ_ERR_NO_MEMORY;
    case DATA_ERR_FILE_OPEN:
        return BIZ_ERR_FILE_OPEN;
    case DATA_ERR_FILE_NOT_FOUND:
        return BIZ_ERR_FILE_NOT_FOUND;
    case DATA_ERR_RECORD_FORMAT:
        return BIZ_ERR_RECORD_FORMAT;
    case DATA_ERR_TIME_PARSE:
        return BIZ_ERR_TIME_PARSE;
    case DATA_ERR_NOT_FOUND:
        return BIZ_ERR_CARD_NOT_FOUND;
    default:
        return BIZ_ERR_SYSTEM;
    }
}

static int isAdminCredential(const char *account, const char *password)
{
    const char adminAccount[] = {'r', 'o', 'o', 't', '\0'};
    const char adminPassword[] = {'r', 'o', 'o', 't', '\0'};
    return strcmp(account, adminAccount) == 0 && strcmp(password, adminPassword) == 0;
}

static BizResult loadCardForRole(const char *cardNameInput, Card *card)
{
    char cardName[INPUT_BUF_SIZE];
    int loadResult = 0;
    const Card *found = NULL;

    if (card == NULL) {
        return BIZ_ERR_SYSTEM;
    }

    if (validatorNormalizeInput(cardNameInput, cardName, sizeof(cardName)) != 0 ||
        !validatorIsValidCardName(cardName)) {
        return BIZ_ERR_INVALID_CARD_NAME;
    }

    loadResult = dataLoadCards();
    if (loadResult < 0) {
        return sessionMapDataResult((DataResult)loadResult);
    }

    found = dataQueryCardByName(cardName);
    if (found == NULL || found->nDel != 0) {
        return BIZ_ERR_CARD_NOT_FOUND;
    }

    *card = *found;
    return BIZ_OK;
}

void bizInitSession(LoginSession *session)
{
    if (session == NULL) {
        return;
    }

    memset(session, 0, sizeof(*session));
    session->role = LOGIN_ROLE_NONE;
    session->loggedIn = 0;
}

void bizLogout(LoginSession *session)
{
    bizInitSession(session);
}

int bizIsAdminSession(const LoginSession *session)
{
    return session != NULL && session->loggedIn != 0 && session->role == LOGIN_ROLE_ADMIN;
}

int bizIsUserSession(const LoginSession *session)
{
    return session != NULL && session->loggedIn != 0 && session->role == LOGIN_ROLE_USER;
}

BizResult bizAdminLogin(const char *accountInput, const char *passwordInput, LoginSession *session)
{
    char account[INPUT_BUF_SIZE];
    char password[INPUT_BUF_SIZE];

    if (session == NULL) {
        return BIZ_ERR_SYSTEM;
    }

    if (validatorNormalizeInput(accountInput, account, sizeof(account)) != 0 ||
        validatorNormalizeInput(passwordInput, password, sizeof(password)) != 0 ||
        !isAdminCredential(account, password)) {
        return BIZ_ERR_WRONG_PASSWORD;
    }

    bizInitSession(session);
    session->role = LOGIN_ROLE_ADMIN;
    session->loggedIn = 1;
    return BIZ_OK;
}

BizResult bizUserRegister(const char *cardNameInput, const char *passwordInput, Card *createdCard)
{
    return bizAddCard(cardNameInput, passwordInput, "100", createdCard);
}

BizResult bizUserLogin(const char *cardNameInput, const char *passwordInput, LoginSession *session)
{
    char password[INPUT_BUF_SIZE];
    Card card;
    BizResult result = BIZ_OK;

    if (session == NULL) {
        return BIZ_ERR_SYSTEM;
    }

    if (validatorNormalizeInput(passwordInput, password, sizeof(password)) != 0 ||
        !validatorIsValidPassword(password)) {
        return BIZ_ERR_INVALID_PASSWORD;
    }

    result = loadCardForRole(cardNameInput, &card);
    if (result != BIZ_OK) {
        return result;
    }
    if (strcmp(card.aPwd, password) != 0) {
        return BIZ_ERR_WRONG_PASSWORD;
    }
    if (card.nStatus == CARD_STATUS_CANCELED) {
        return BIZ_ERR_CARD_CANCELED_FOR_START;
    }

    bizInitSession(session);
    session->role = LOGIN_ROLE_USER;
    session->loggedIn = 1;
    snprintf(session->cardName, sizeof(session->cardName), "%s", card.aCardName);
    snprintf(session->password, sizeof(session->password), "%s", card.aPwd);
    return BIZ_OK;
}

BizResult bizAdminQueryCard(const LoginSession *session, const char *cardNameInput, Card *queriedCard)
{
    if (!bizIsAdminSession(session)) {
        return BIZ_ERR_SYSTEM;
    }
    return bizQueryCard(cardNameInput, queriedCard);
}

BizResult bizAdminStopBilling(const LoginSession *session,
                              const char *cardNameInput,
                              time_t requestTime,
                              SettleInfo *settleInfo)
{
    Card card;
    BizResult result = BIZ_OK;

    if (!bizIsAdminSession(session)) {
        return BIZ_ERR_SYSTEM;
    }

    result = loadCardForRole(cardNameInput, &card);
    if (result != BIZ_OK) {
        return result;
    }
    return bizStopBilling(card.aCardName, card.aPwd, requestTime, settleInfo);
}

BizResult bizAdminRecharge(const LoginSession *session,
                           const char *cardNameInput,
                           const char *amountInput,
                           Money *rechargeRecord,
                           Card *updatedCard)
{
    Card card;
    BizResult result = BIZ_OK;

    if (!bizIsAdminSession(session)) {
        return BIZ_ERR_SYSTEM;
    }

    result = loadCardForRole(cardNameInput, &card);
    if (result != BIZ_OK) {
        return result;
    }
    return bizRecharge(card.aCardName, card.aPwd, amountInput, rechargeRecord, updatedCard);
}

BizResult bizAdminRefundByAmount(const LoginSession *session,
                                 const char *cardNameInput,
                                 const char *amountInput,
                                 Money *refundRecord,
                                 Card *updatedCard)
{
    Card card;
    BizResult result = BIZ_OK;

    if (!bizIsAdminSession(session)) {
        return BIZ_ERR_SYSTEM;
    }

    result = loadCardForRole(cardNameInput, &card);
    if (result != BIZ_OK) {
        return result;
    }
    return bizRefundByAmount(card.aCardName, card.aPwd, amountInput, refundRecord, updatedCard);
}

BizResult bizAdminGetBillingStatistics(const LoginSession *session,
                                       const char *yearInput,
                                       BillingStatistics *statistics)
{
    if (!bizIsAdminSession(session)) {
        return BIZ_ERR_SYSTEM;
    }
    return bizGetBillingStatistics(yearInput, statistics);
}

BizResult bizUserQueryBalance(const LoginSession *session, Card *queriedCard)
{
    if (!bizIsUserSession(session)) {
        return BIZ_ERR_SYSTEM;
    }
    return bizQueryCard(session->cardName, queriedCard);
}

BizResult bizUserStartBilling(const LoginSession *session, time_t requestTime, LogonInfo *logonInfo)
{
    if (!bizIsUserSession(session)) {
        return BIZ_ERR_SYSTEM;
    }
    return bizStartBilling(session->cardName, session->password, requestTime, logonInfo);
}

BizResult bizUserStopBilling(const LoginSession *session, time_t requestTime, SettleInfo *settleInfo)
{
    if (!bizIsUserSession(session)) {
        return BIZ_ERR_SYSTEM;
    }
    return bizStopBilling(session->cardName, session->password, requestTime, settleInfo);
}

BizResult bizUserRecharge(const LoginSession *session,
                          const char *amountInput,
                          Money *rechargeRecord,
                          Card *updatedCard)
{
    if (!bizIsUserSession(session)) {
        return BIZ_ERR_SYSTEM;
    }
    return bizRecharge(session->cardName, session->password, amountInput, rechargeRecord, updatedCard);
}

BizResult bizUserRefundByAmount(const LoginSession *session,
                                const char *amountInput,
                                Money *refundRecord,
                                Card *updatedCard)
{
    if (!bizIsUserSession(session)) {
        return BIZ_ERR_SYSTEM;
    }
    return bizRefundByAmount(session->cardName, session->password, amountInput, refundRecord, updatedCard);
}

BizResult bizUserCancelCardWithPassword(const LoginSession *session,
                                        const char *cardNameInput,
                                        const char *passwordInput,
                                        Money *refundRecord,
                                        Card *updatedCard)
{
    char cardName[INPUT_BUF_SIZE];
    char password[INPUT_BUF_SIZE];

    if (!bizIsUserSession(session)) {
        return BIZ_ERR_SYSTEM;
    }

    if (validatorNormalizeInput(cardNameInput, cardName, sizeof(cardName)) != 0 ||
        !validatorIsValidCardName(cardName) ||
        validatorNormalizeInput(passwordInput, password, sizeof(password)) != 0 ||
        !validatorIsValidPassword(password)) {
        return BIZ_ERR_WRONG_PASSWORD;
    }

    if (strcmp(cardName, session->cardName) != 0 || strcmp(password, session->password) != 0) {
        return BIZ_ERR_WRONG_PASSWORD;
    }

    return bizCancelCard(cardName, password, refundRecord, updatedCard);
}
