#!/usr/bin/env bash
set -euo pipefail

gcc -std=c11 -Wall -Wextra -pedantic -Iinclude \
    tests/exp7/helpers/menu_refund_success_check.c \
    src/data/repository.c src/data/money_repository.c src/platform/platform.c \
    -o build/bin/menu_refund_success_check

build/bin/menu_refund_success_check refund01 0
