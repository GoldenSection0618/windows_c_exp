#!/usr/bin/env bash
set -euo pipefail

gcc -std=c11 -Wall -Wextra -pedantic -Iinclude \
    tests/exp6/helpers/menu_stop_billing_success_check.c \
    src/data/repository.c src/data/billing_repository.c \
    -o build/bin/menu_stop_billing_success_check

build/bin/menu_stop_billing_success_check

if ! awk -F'|' 'NR==1 && $1=="stopA1" && $3=="0" && $6+0>0 && $8=="1" && $9+0<10000 {found=1} END {exit found ? 0 : 1}' data/cards.txt; then
    echo "[verify] stop success card state not updated as expected" >&2
    exit 1
fi

if ! awk -F'|' 'NR==1 && $1=="stopA1" && $3!="0" && $4+0>0 && $5=="1" && $6=="0" {found=1} END {exit found ? 0 : 1}' data/billings.txt; then
    echo "[verify] stop success billing record not settled as expected" >&2
    exit 1
fi
