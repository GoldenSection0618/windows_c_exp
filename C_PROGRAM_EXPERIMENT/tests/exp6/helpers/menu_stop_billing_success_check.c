#include "billing_repository.h"
#include "billing_storage_file.h"
#include "card_repository.h"
#include "card_storage_file.h"

#include <stdio.h>

int main(void)
{
    const Card *card = NULL;
    const Billing *billing = NULL;
    int count = 0;

    count = dataLoadCards();
    if (count != 1) {
        fprintf(stderr, "card load count mismatch: %d\n", count);
        return 1;
    }

    card = dataQueryCardByName("stopA1");
    if (card == NULL) {
        fprintf(stderr, "card not found after stop billing\n");
        return 1;
    }
    if (card->nStatus != CARD_STATUS_OFFLINE) {
        fprintf(stderr, "card status mismatch: %d\n", card->nStatus);
        return 1;
    }
    if (card->nBalanceCent >= 10000) {
        fprintf(stderr, "card balance did not decrease: %d\n", card->nBalanceCent);
        return 1;
    }
    if (card->nTotalUseCent <= 0) {
        fprintf(stderr, "card total use not updated: %d\n", card->nTotalUseCent);
        return 1;
    }
    if (card->nUseCount != 1) {
        fprintf(stderr, "card use count mismatch: %d\n", card->nUseCount);
        return 1;
    }

    count = dataLoadBillings();
    if (count != 1) {
        fprintf(stderr, "billing load count mismatch: %d\n", count);
        return 1;
    }

    billing = dataQueryLatestUnsettledBillingByCardName("stopA1");
    if (billing != NULL) {
        fprintf(stderr, "billing should already be settled\n");
        return 1;
    }

    dataCleanup();
    dataCleanupBillings();
    return 0;
}
