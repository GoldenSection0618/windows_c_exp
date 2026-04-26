#ifndef MONEY_STORAGE_FILE_H
#define MONEY_STORAGE_FILE_H

#include "money_repository.h"

DataResult dataSaveMoney(const Money *money);
int dataLoadMoneys(void);
int dataGetMoneyCount(void);

#endif
