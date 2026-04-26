#!/usr/bin/env bash
set -euo pipefail

if ! awk -F'|' 'NR==1 && $1=="stopA2" && $3=="1" && $9=="10000" {found=1} END {exit found ? 0 : 1}' data/cards.txt; then
    echo "[verify] card file changed unexpectedly for no-unsettled case" >&2
    exit 1
fi

if [ -f data/billings.txt ] && [ -s data/billings.txt ]; then
    echo "[verify] billing file should stay empty for no-unsettled case" >&2
    exit 1
fi
