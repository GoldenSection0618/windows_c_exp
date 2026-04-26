#ifndef TIME_VALIDATOR_H
#define TIME_VALIDATOR_H

#include <time.h>

int timeValidatorParseDateTime(const char *input, time_t *outTime);

#endif
