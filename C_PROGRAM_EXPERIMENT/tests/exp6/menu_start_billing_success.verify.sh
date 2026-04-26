#!/usr/bin/env bash
set -euo pipefail

gcc -std=c11 -Wall -Wextra -pedantic -Iinclude \
    tests/exp6/helpers/menu_start_billing_success_check.c \
    src/data/repository.c src/data/billing_repository.c \
    -o build/bin/menu_start_billing_success_check

build/bin/menu_start_billing_success_check
