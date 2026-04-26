#!/usr/bin/env bash
set -euo pipefail

if ! awk -F'|' 'NR==1 && $1=="stopA3" && $3=="0" && $6+0>0 && $8=="1" && $9+0<0 {found=1} END {exit found ? 0 : 1}' data/cards.txt; then
    echo "[verify] card file not settled into arrears state as expected" >&2
    exit 1
fi

if ! awk -F'|' 'NR==1 && $1=="stopA3" && $3!="0" && $4+0>0 && $5=="1" && $6=="0" {found=1} END {exit found ? 0 : 1}' data/billings.txt; then
    echo "[verify] billing file not settled for arrears stop-billing case" >&2
    exit 1
fi
