#!/usr/bin/env bash
set -euo pipefail

if ! awk -F'|' 'NR==1 && $1=="stopA6" && $3=="0" && $5=="0" {old_ok=1} NR==2 && $1=="stopA6" && $3!="0" && $5=="1" {latest_ok=1} END {exit (old_ok && latest_ok) ? 0 : 1}' data/billings.txt; then
    echo "[verify] multiple-unsettled selection rule failed" >&2
    exit 1
fi
