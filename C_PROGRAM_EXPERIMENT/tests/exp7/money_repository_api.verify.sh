#!/usr/bin/env bash
set -euo pipefail

gcc -std=c11 -Wall -Wextra -pedantic -Iinclude \
    tests/exp7/helpers/money_repository_api_check.c \
    src/data/money_repository.c src/platform/platform.c \
    -o build/bin/money_repository_api_check

build/bin/money_repository_api_check
