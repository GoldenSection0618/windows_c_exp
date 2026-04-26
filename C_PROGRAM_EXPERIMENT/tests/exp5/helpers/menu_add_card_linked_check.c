#include "card_repository.h"
#include "card_storage_file.h"

#include <stdio.h>
#include <string.h>

int main(void)
{
    const Card *card = NULL;
    int count = 0;

    count = dataLoadCards();
    if (count != 1) {
        fprintf(stderr, "readCard count mismatch: %d\n", count);
        return 1;
    }

    card = dataQueryCardByName("linked01");
    if (card == NULL) {
        fprintf(stderr, "linked01 not found after add\n");
        return 1;
    }

    if (strcmp(card->aPwd, "pwd001") != 0) {
        fprintf(stderr, "password mismatch\n");
        return 1;
    }
    if (card->nStatus != CARD_STATUS_OFFLINE) {
        fprintf(stderr, "status mismatch: %d\n", card->nStatus);
        return 1;
    }
    if (card->nBalanceCent != 10000) {
        fprintf(stderr, "balance mismatch: %d\n", card->nBalanceCent);
        return 1;
    }
    if (card->nTotalUseCent != 0 || card->nUseCount != 0 || card->nDel != 0) {
        fprintf(stderr, "initial field mismatch\n");
        return 1;
    }

    dataCleanup();
    return 0;
}
