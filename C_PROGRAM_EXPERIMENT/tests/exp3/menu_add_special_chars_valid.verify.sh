#!/usr/bin/env bash
set -euo pipefail

if ! awk -F'|' 'NR==1 && NF==10 && $1=="card_001@" && $2=="p_1!" {found=1} END {exit found ? 0 : 1}' data/cards.txt; then
    echo "[verify] allowed character set was not persisted correctly" >&2
    exit 1
fi
