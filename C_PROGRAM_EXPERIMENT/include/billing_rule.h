#ifndef BILLING_RULE_H
#define BILLING_RULE_H

#include "model.h"

Rate billingRuleGetDefaultRate(void);
int billingRuleCalculateAmount(time_t tStart,
                               time_t tEnd,
                               const Rate *rate,
                               int *durationMinutes,
                               int32_t *amountCent);

#endif
