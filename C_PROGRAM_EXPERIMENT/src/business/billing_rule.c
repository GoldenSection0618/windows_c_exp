#include "billing_rule.h"

#include <limits.h>
#include <stdint.h>

Rate billingRuleGetDefaultRate(void)
{
    Rate rate;

    rate.starttime = 0;
    rate.endtime = 0;
    rate.unit = 1;
    rate.nChargeCent = 1;
    rate.ratetype = 0;
    rate.del = 0;
    return rate;
}

int billingRuleCalculateAmount(time_t tStart,
                               time_t tEnd,
                               const Rate *rate,
                               int *durationMinutes,
                               int32_t *amountCent)
{
    time_t durationSeconds = 0;
    int unitMinutes = 1;
    int roundedUnits = 0;
    int64_t computedAmount = 0;

    if (rate == NULL || durationMinutes == NULL || amountCent == NULL) {
        return -1;
    }

    if (rate->unit > 0) {
        unitMinutes = rate->unit;
    }

    if (tEnd <= tStart) {
        durationSeconds = 0;
    } else {
        durationSeconds = tEnd - tStart;
    }

    *durationMinutes = (int)((durationSeconds + 59) / 60);
    if (*durationMinutes < 1) {
        *durationMinutes = 1;
    }

    roundedUnits = (*durationMinutes + unitMinutes - 1) / unitMinutes;
    if (roundedUnits < 1) {
        roundedUnits = 1;
    }

    computedAmount = (int64_t)roundedUnits * (int64_t)rate->nChargeCent;
    if (computedAmount < 0 || computedAmount > INT32_MAX) {
        return -1;
    }

    *amountCent = (int32_t)computedAmount;
    return 0;
}
