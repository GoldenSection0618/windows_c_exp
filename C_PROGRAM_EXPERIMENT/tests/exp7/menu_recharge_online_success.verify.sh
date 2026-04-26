#!/usr/bin/env bash
set -euo pipefail

gcc -std=c11 -Wall -Wextra -pedantic -Iinclude \
    tests/exp7/helpers/menu_recharge_success_check.c \
    src/data/repository.c src/data/money_repository.c src/platform/platform.c \
    -o build/bin/menu_recharge_online_success_check

build/bin/menu_recharge_online_success_check rechg02 11500 1
