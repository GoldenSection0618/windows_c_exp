#include "card_repository.h"

#include <stdio.h>
#include <string.h>

static Card makeCard(const char *cardName, const char *password, int balanceCent)
{
    Card card;

    memset(&card, 0, sizeof(card));
    snprintf(card.aCardName, sizeof(card.aCardName), "%s", cardName);
    snprintf(card.aPwd, sizeof(card.aPwd), "%s", password);
    card.nBalanceCent = balanceCent;
    card.nStatus = CARD_STATUS_OFFLINE;
    card.nDel = 0;
    return card;
}

static int expectDelete(const char *cardName, DataResult expected)
{
    DataResult result = dataDeleteCardByName(cardName);

    if (result != expected) {
        fprintf(stderr, "delete %s result mismatch: got %d want %d\n",
                cardName == NULL ? "(null)" : cardName,
                result,
                expected);
        return 1;
    }
    return 0;
}

int main(void)
{
    Card cardA;
    Card cardB;
    Card cardC;
    Card cardD;

    dataCleanup();

    if (dataQueryCardByName("missing") != NULL) {
        fprintf(stderr, "missing card unexpectedly found in empty list\n");
        return 1;
    }
    if (expectDelete("missing", DATA_ERR_NOT_FOUND) != 0) {
        return 1;
    }

    cardA = makeCard("cardA", "pwd001", 1000);
    if (dataAddCard(&cardA) != DATA_OK) {
        fprintf(stderr, "add cardA failed\n");
        return 1;
    }
    if (dataQueryCardByName("cardA") == NULL) {
        fprintf(stderr, "cardA missing after first insert\n");
        return 1;
    }

    cardB = makeCard("cardB", "pwd002", 2000);
    cardC = makeCard("cardC", "pwd003", 3000);
    if (dataAddCard(&cardB) != DATA_OK || dataAddCard(&cardC) != DATA_OK) {
        fprintf(stderr, "tail insertion failed\n");
        return 1;
    }
    if (dataQueryCardByName("cardA") == NULL ||
        dataQueryCardByName("cardB") == NULL ||
        dataQueryCardByName("cardC") == NULL) {
        fprintf(stderr, "tail insertion lookup failed\n");
        return 1;
    }
    if (dataAddCard(&cardB) != DATA_ERR_DUPLICATE) {
        fprintf(stderr, "duplicate insert was not rejected\n");
        return 1;
    }

    if (expectDelete("cardA", DATA_OK) != 0) {
        return 1;
    }
    if (dataQueryCardByName("cardA") != NULL) {
        fprintf(stderr, "cardA still exists after deleting head\n");
        return 1;
    }
    if (dataQueryCardByName("cardB") == NULL || dataQueryCardByName("cardC") == NULL) {
        fprintf(stderr, "remaining nodes broken after deleting head\n");
        return 1;
    }

    cardD = makeCard("cardD", "pwd004", 4000);
    if (dataAddCard(&cardD) != DATA_OK) {
        fprintf(stderr, "add cardD failed\n");
        return 1;
    }

    if (expectDelete("cardC", DATA_OK) != 0) {
        return 1;
    }
    if (dataQueryCardByName("cardC") != NULL) {
        fprintf(stderr, "cardC still exists after deleting middle node\n");
        return 1;
    }
    if (dataQueryCardByName("cardB") == NULL || dataQueryCardByName("cardD") == NULL) {
        fprintf(stderr, "remaining nodes broken after deleting middle node\n");
        return 1;
    }

    if (expectDelete("cardD", DATA_OK) != 0) {
        return 1;
    }
    if (dataQueryCardByName("cardD") != NULL) {
        fprintf(stderr, "cardD still exists after deleting tail\n");
        return 1;
    }
    if (dataQueryCardByName("cardB") == NULL) {
        fprintf(stderr, "cardB missing after deleting tail\n");
        return 1;
    }

    dataCleanup();
    if (dataQueryCardByName("cardB") != NULL) {
        fprintf(stderr, "list not cleared by dataCleanup\n");
        return 1;
    }

    cardA = makeCard("afterClear", "pwd005", 5000);
    if (dataAddCard(&cardA) != DATA_OK) {
        fprintf(stderr, "reinsert after cleanup failed\n");
        return 1;
    }
    if (dataQueryCardByName("afterClear") == NULL) {
        fprintf(stderr, "afterClear missing after reinsertion\n");
        return 1;
    }

    dataCleanup();
    return 0;
}
