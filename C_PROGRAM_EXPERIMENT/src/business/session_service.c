#include "business.h"
#include "business_internal.h"

#include "card_validator.h"
#include "common.h"
#include "billing_repository.h"
#include "card_storage_file.h"
#include "money_repository.h"

#include <stdio.h>
#include <string.h>

static void clearSession(LoginSession *session)
{
    if (session == NULL) {
        return;
    }

    memset(session, 0, sizeof(*session));
    session->role = LOGIN_ROLE_NONE;
    session->loggedIn = 0;
}


static int isAdminSessionValid(const LoginSession *session)
{
    return session != NULL && session->loggedIn != 0 && session->role == LOGIN_ROLE_ADMIN;
}


static int isUserSessionValid(const LoginSession *session)
{
    return session != NULL && session->loggedIn != 0 && session->role == LOGIN_ROLE_USER;
}


void bizInitSession(LoginSession *session)
{
    clearSession(session);
}


void bizLogout(LoginSession *session)
{
    clearSession(session);
}


int bizIsAdminSession(const LoginSession *session)
{
    return isAdminSessionValid(session);
}


int bizIsUserSession(const LoginSession *session)
{
    return isUserSessionValid(session);
}


BizResult bizAdminLogin(const char *accountInput, const char *passwordInput, LoginSession *session)
{
    char account[INPUT_BUF_SIZE];
    char password[INPUT_BUF_SIZE];

    if (session == NULL) {
        return BIZ_ERR_SYSTEM;
    }

    if (validatorNormalizeInput(accountInput, account, sizeof(account)) != 0 ||
        validatorNormalizeInput(passwordInput, password, sizeof(password)) != 0) {
        return BIZ_ERR_WRONG_PASSWORD;
    }

    if (strcmp(account, "root") != 0 || strcmp(password, "root") != 0) {
        return BIZ_ERR_WRONG_PASSWORD;
    }

    clearSession(session);
    session->role = LOGIN_ROLE_ADMIN;
    session->loggedIn = 1;
    return BIZ_OK;
}


BizResult bizUserLogin(const char *cardNameInput, const char *passwordInput, LoginSession *session)
{
    Card card;
    BizResult result = BIZ_OK;

    if (session == NULL) {
        return BIZ_ERR_SYSTEM;
    }

    result = bizLoadCardByCredentialInternal(cardNameInput, passwordInput, &card);
    if (result != BIZ_OK) {
        return result == BIZ_ERR_INVALID_PASSWORD ? BIZ_ERR_WRONG_PASSWORD : result;
    }

    if (card.nStatus == CARD_STATUS_CANCELED) {
        return BIZ_ERR_CARD_CANCELED_FOR_LOGIN;
    }
    if (card.nStatus == CARD_STATUS_ONLINE) {
        return BIZ_ERR_CARD_ONLINE_FOR_LOGIN;
    }

    clearSession(session);
    session->role = LOGIN_ROLE_USER;
    session->loggedIn = 1;
    snprintf(session->cardName, sizeof(session->cardName), "%s", card.aCardName);
    return BIZ_OK;
}


void bizShutdown(void)
{
    dataCleanup();
    dataCleanupBillings();
    dataCleanupMoneys();
}
