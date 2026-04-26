#!/usr/bin/env bash
set -euo pipefail

if ! awk -F'|' 'NR==1 && $1=="startA6" && $3=="0" && $9=="-1" {found=1} END {exit found ? 0 : 1}' data/cards.txt; then
    echo "[verify] card file changed unexpectedly for start balance-not-enough case" >&2
    exit 1
fi

if [ -s data/billings.txt ]; then
    echo "[verify] billing file should stay empty for start balance-not-enough case" >&2
    exit 1
fi
