#!/usr/bin/env bash
set -euo pipefail

gcc -std=c11 -Wall -Wextra -pedantic -Iinclude \
    tests/exp6/helpers/billing_repository_api_check.c src/data/billing_repository.c \
    -o build/bin/billing_repository_api_check

build/bin/billing_repository_api_check
