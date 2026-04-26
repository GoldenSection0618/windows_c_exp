#!/usr/bin/env bash
set -euo pipefail

if [ ! -f data/cards.txt ]; then
    echo "[verify] cards file not created" >&2
    exit 1
fi

if [ "$(wc -l < data/cards.txt)" -ne 1 ]; then
    echo "[verify] cards file line count mismatch" >&2
    exit 1
fi

if ! awk -F'|' 'NR==1 && NF==10 && $1=="card001" && $2=="pass01" && $3=="0" && $6=="0" && $8=="0" && $9=="10000" && $10=="0" {found=1} END {exit found ? 0 : 1}' data/cards.txt; then
    echo "[verify] cards file content mismatch" >&2
    exit 1
fi
