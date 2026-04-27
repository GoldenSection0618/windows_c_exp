#ifndef BUSINESS_INTERNAL_H
#define BUSINESS_INTERNAL_H

#include "business.h"
#include "card_repository.h"

BizResult mapDataResult(DataResult result);
BizResult bizLoadCardByName(const char *cardNameInput, Card *outCard);
BizResult bizLoadCardByCredentialInternal(const char *cardNameInput, const char *passwordInput, Card *outCard);
BizResult bizGetCardPasswordByName(const char *cardNameInput, char *passwordBuffer, size_t passwordBufferSize);

#endif
