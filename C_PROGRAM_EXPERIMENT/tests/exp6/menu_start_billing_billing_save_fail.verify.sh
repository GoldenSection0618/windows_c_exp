#!/usr/bin/env bash
set -euo pipefail

if ! awk -F'|' 'NR==1 && $1=="startA8" && $3=="0" && $9=="10000" {found=1} END {exit found ? 0 : 1}' data/cards.txt; then
    echo "[verify] card rollback failed for billing-save-fail case" >&2
    exit 1
fi

if [ -s data/billings.txt ]; then
    echo "[verify] billing file should stay empty for billing-save-fail case" >&2
    exit 1
fi
