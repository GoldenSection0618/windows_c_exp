#include "card_repository.h"
#include "card_storage_file.h"
#include "money_repository.h"
#include "money_storage_file.h"

#include <stdio.h>
#include <stdlib.h>

int main(int argc, char *argv[])
{
    const Card *card = NULL;
    const Money *money = NULL;
    const char *cardName = NULL;
    int expectedStatus = 0;
    int count = 0;

    if (argc != 3) {
        fprintf(stderr, "usage: %s <cardName> <expectedStatus>\n", argv[0]);
        return 1;
    }

    cardName = argv[1];
    expectedStatus = atoi(argv[2]);

    count = dataLoadCards();
    if (count != 1) {
        fprintf(stderr, "card load count mismatch: %d\n", count);
        return 1;
    }

    card = dataQueryCardByName(cardName);
    if (card == NULL) {
        fprintf(stderr, "card not found after refund\n");
        return 1;
    }
    if (card->nBalanceCent != 0) {
        fprintf(stderr, "card balance should be zero after refund: %d\n", card->nBalanceCent);
        return 1;
    }
    if (card->nStatus != expectedStatus) {
        fprintf(stderr, "card status mismatch: got %d want %d\n", card->nStatus, expectedStatus);
        return 1;
    }

    count = dataLoadMoneys();
    if (count != 1) {
        fprintf(stderr, "money load count mismatch: %d\n", count);
        return 1;
    }

    money = dataQueryLatestMoneyByCardName(cardName);
    if (money == NULL) {
        fprintf(stderr, "money record not found after refund\n");
        return 1;
    }
    if (money->nStatus != 1 || money->nDel != 0) {
        fprintf(stderr, "money status or delete flag mismatch\n");
        return 1;
    }
    if (money->nMoneyCent <= 0) {
        fprintf(stderr, "refund amount should be positive\n");
        return 1;
    }

    dataCleanup();
    dataCleanupMoneys();
    return 0;
}
