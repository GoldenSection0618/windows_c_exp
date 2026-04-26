#include "billing_repository.h"
#include "billing_storage_file.h"

#include <stdio.h>
#include <string.h>

static Billing makeBilling(const char *cardName,
                           time_t tStart,
                           time_t tEnd,
                           int amountCent,
                           int status)
{
    Billing billing;

    memset(&billing, 0, sizeof(billing));
    snprintf(billing.aCardName, sizeof(billing.aCardName), "%s", cardName);
    billing.tStart = tStart;
    billing.tEnd = tEnd;
    billing.nAmountCent = amountCent;
    billing.nStatus = status;
    billing.nDel = 0;
    return billing;
}

int main(void)
{
    Billing settled;
    Billing unsettled;
    Billing other;
    const Billing *found = NULL;
    Billing updated;
    int loadCount = 0;
    int fileCount = 0;

    dataCleanupBillings();

    settled = makeBilling("billA", (time_t)1000, (time_t)1500, 300, 1);
    unsettled = makeBilling("billA", (time_t)2000, (time_t)0, 0, 0);
    other = makeBilling("billB", (time_t)3000, (time_t)0, 0, 0);

    if (dataSaveBilling(&settled) != DATA_OK ||
        dataSaveBilling(&unsettled) != DATA_OK ||
        dataSaveBilling(&other) != DATA_OK) {
        fprintf(stderr, "save billing records failed\n");
        return 1;
    }

    fileCount = dataGetBillingCount();
    if (fileCount != 3) {
        fprintf(stderr, "billing file count mismatch: got %d want 3\n", fileCount);
        return 1;
    }

    loadCount = dataLoadBillings();
    if (loadCount != 3) {
        fprintf(stderr, "load billings count mismatch: got %d want 3\n", loadCount);
        return 1;
    }

    found = dataQueryLatestUnsettledBillingByCardName("billA");
    if (found == NULL || found->tStart != (time_t)2000 || found->nStatus != 0 || found->tEnd != (time_t)0) {
        fprintf(stderr, "latest unsettled billing for billA not found as expected\n");
        return 1;
    }

    updated = *found;
    updated.tEnd = (time_t)2600;
    updated.nAmountCent = 660;
    updated.nStatus = 1;

    if (dataUpdateBilling(&updated) != DATA_OK) {
        fprintf(stderr, "update billing failed\n");
        return 1;
    }

    dataCleanupBillings();
    loadCount = dataLoadBillings();
    if (loadCount != 3) {
        fprintf(stderr, "reload after update count mismatch: got %d want 3\n", loadCount);
        return 1;
    }

    found = dataQueryLatestUnsettledBillingByCardName("billA");
    if (found != NULL) {
        fprintf(stderr, "billA should have no unsettled billing after update\n");
        return 1;
    }

    found = dataQueryLatestUnsettledBillingByCardName("billB");
    if (found == NULL || found->tStart != (time_t)3000 || found->nStatus != 0) {
        fprintf(stderr, "billB unsettled billing lookup failed\n");
        return 1;
    }

    dataCleanupBillings();
    return 0;
}
