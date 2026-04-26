#ifndef CARD_STORAGE_FILE_H
#define CARD_STORAGE_FILE_H

#include "card_repository.h"

DataResult dataSaveCard(const Card *card);
int dataLoadCards(void);
int dataGetCardCount(void);
DataResult dataUpdateCard(const Card *card);

#endif
