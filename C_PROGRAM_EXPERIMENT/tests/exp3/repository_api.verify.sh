#!/usr/bin/env bash
set -euo pipefail

gcc -std=c11 -Wall -Wextra -pedantic -Iinclude \
    tests/exp3/helpers/repository_api_check.c src/data/repository.c \
    -o build/bin/repository_api_check

build/bin/repository_api_check

if ! awk -F'|' 'NR==1 && $1=="cardApi" && $6=="500" && $8=="2" && $9=="20500" {found=1} END {exit found ? 0 : 1}' data/cards.txt; then
    echo "[verify] updateCard rewrite mismatch" >&2
    exit 1
fi
