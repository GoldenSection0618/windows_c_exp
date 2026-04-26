#include "card_repository.h"

#include <stdio.h>
#include <string.h>

static Card makeCard(const char *cardName, int balanceCent)
{
    Card card;

    memset(&card, 0, sizeof(card));
    snprintf(card.aCardName, sizeof(card.aCardName), "%s", cardName);
    snprintf(card.aPwd, sizeof(card.aPwd), "pwd001");
    card.nBalanceCent = balanceCent;
    card.nStatus = CARD_STATUS_OFFLINE;
    return card;
}

static int expectDelete(const char *cardName, DataResult expected)
{
    DataResult result = dataDeleteCardByName(cardName);

    if (result != expected) {
        fprintf(stderr, "delete %s result mismatch: got %d want %d\n", cardName == NULL ? "(null)" : cardName, result, expected);
        return 1;
    }
    return 0;
}

int main(void)
{
    Card card = makeCard("headCard", 1000);

    if (expectDelete("missingOnEmpty", DATA_ERR_NOT_FOUND) != 0) {
        return 1;
    }

    if (dataAddCard(&card) != DATA_OK) {
        fprintf(stderr, "add headCard failed\n");
        return 1;
    }
    card = makeCard("midCard", 2000);
    if (dataAddCard(&card) != DATA_OK) {
        fprintf(stderr, "add midCard failed\n");
        return 1;
    }
    card = makeCard("tailCard", 3000);
    if (dataAddCard(&card) != DATA_OK) {
        fprintf(stderr, "add tailCard failed\n");
        return 1;
    }

    if (expectDelete("headCard", DATA_OK) != 0) {
        return 1;
    }
    if (dataQueryCardByName("headCard") != NULL) {
        fprintf(stderr, "headCard still exists after delete\n");
        return 1;
    }
    if (dataQueryCardByName("midCard") == NULL || dataQueryCardByName("tailCard") == NULL) {
        fprintf(stderr, "list broken after deleting head\n");
        return 1;
    }

    if (expectDelete("tailCard", DATA_OK) != 0) {
        return 1;
    }
    if (dataQueryCardByName("tailCard") != NULL) {
        fprintf(stderr, "tailCard still exists after delete\n");
        return 1;
    }
    if (dataQueryCardByName("midCard") == NULL) {
        fprintf(stderr, "midCard missing after deleting tail\n");
        return 1;
    }

    card = makeCard("tailCard2", 4000);
    if (dataAddCard(&card) != DATA_OK) {
        fprintf(stderr, "add tailCard2 failed\n");
        return 1;
    }
    card = makeCard("tailCard3", 5000);
    if (dataAddCard(&card) != DATA_OK) {
        fprintf(stderr, "add tailCard3 failed\n");
        return 1;
    }

    if (expectDelete("tailCard2", DATA_OK) != 0) {
        return 1;
    }
    if (dataQueryCardByName("tailCard2") != NULL) {
        fprintf(stderr, "tailCard2 still exists after delete\n");
        return 1;
    }
    if (dataQueryCardByName("midCard") == NULL || dataQueryCardByName("tailCard3") == NULL) {
        fprintf(stderr, "list broken after deleting middle node\n");
        return 1;
    }

    if (expectDelete("missingCard", DATA_ERR_NOT_FOUND) != 0) {
        return 1;
    }

    if (expectDelete("midCard", DATA_OK) != 0) {
        return 1;
    }
    if (expectDelete("tailCard3", DATA_OK) != 0) {
        return 1;
    }
    if (dataQueryCardByName("midCard") != NULL || dataQueryCardByName("tailCard3") != NULL) {
        fprintf(stderr, "remaining nodes still exist after delete\n");
        return 1;
    }
    if (expectDelete("missingAfterClear", DATA_ERR_NOT_FOUND) != 0) {
        return 1;
    }

    card = makeCard("afterClear", 6000);
    if (dataAddCard(&card) != DATA_OK) {
        fprintf(stderr, "add afterClear failed\n");
        return 1;
    }
    if (dataQueryCardByName("afterClear") == NULL) {
        fprintf(stderr, "afterClear missing after re-add\n");
        return 1;
    }

    dataCleanup();
    return 0;
}
