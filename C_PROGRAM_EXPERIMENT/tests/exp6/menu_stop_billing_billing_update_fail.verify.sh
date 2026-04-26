#!/usr/bin/env bash
set -euo pipefail

if ! awk -F'|' 'NR==1 && $1=="stopA7" && $3=="1" && $9=="10000" {found=1} END {exit found ? 0 : 1}' data/cards.txt; then
    echo "[verify] card rollback failed for billing-update-fail case" >&2
    exit 1
fi

if ! awk -F'|' 'NR==1 && $1=="stopA7" && $3=="0" && $4=="0" && $5=="0" && $6=="0" {found=1} END {exit found ? 0 : 1}' data/billings.txt; then
    echo "[verify] billing file changed unexpectedly for billing-update-fail case" >&2
    exit 1
fi
