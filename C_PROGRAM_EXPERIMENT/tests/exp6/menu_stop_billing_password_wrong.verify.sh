#!/usr/bin/env bash
set -euo pipefail

if ! awk -F'|' 'NR==1 && $1=="stopA4" && $3=="1" && $9=="10000" {found=1} END {exit found ? 0 : 1}' data/cards.txt; then
    echo "[verify] card file changed unexpectedly for wrong-password stop case" >&2
    exit 1
fi

if ! awk -F'|' 'NR==1 && $1=="stopA4" && $3=="0" && $4=="0" && $5=="0" && $6=="0" {found=1} END {exit found ? 0 : 1}' data/billings.txt; then
    echo "[verify] billing file changed unexpectedly for wrong-password stop case" >&2
    exit 1
fi
