#!/usr/bin/env bash
set -euo pipefail

if [ -f data/cards.txt ] && [ -s data/cards.txt ]; then
    echo "[verify] oversized amount should not persist data" >&2
    exit 1
fi
