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

    card = dataQueryCardByName("startA1");
    if (card == NULL) {
        fprintf(stderr, "card not found after start billing\n");
        return 1;
    }
    if (card->nStatus != CARD_STATUS_ONLINE) {
        fprintf(stderr, "card status mismatch: %d\n", card->nStatus);
        return 1;
    }
    if (card->tLast == (time_t)0 || card->tLast == card->tStart) {
        fprintf(stderr, "card last-use time not updated on start billing\n");
        return 1;
    }

    count = dataLoadBillings();
    if (count != 1) {
        fprintf(stderr, "billing load count mismatch: %d\n", count);
        return 1;
    }

    billing = dataQueryLatestUnsettledBillingByCardName("startA1");
    if (billing == NULL) {
        fprintf(stderr, "unsettled billing not found\n");
        return 1;
    }
    if (billing->nStatus != 0 || billing->nAmountCent != 0 || billing->tEnd != (time_t)0 || billing->nDel != 0) {
        fprintf(stderr, "billing field mismatch\n");
        return 1;
    }

    dataCleanup();
    dataCleanupBillings();
    return 0;
}
