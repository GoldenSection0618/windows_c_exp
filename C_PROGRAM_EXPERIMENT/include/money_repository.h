#ifndef MONEY_REPOSITORY_H
#define MONEY_REPOSITORY_H

#include "card_repository.h"
#include "money_storage.h"

int dataAddMoney(const Money *money);
const Money *dataQueryLatestMoneyByCardName(const char *cardName);
void dataCleanupMoneys(void);

#endif
