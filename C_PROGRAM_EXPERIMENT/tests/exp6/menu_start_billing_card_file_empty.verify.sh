#!/usr/bin/env bash
set -euo pipefail

if [ ! -f data/cards.txt ]; then
    echo "[verify] cards file should remain present for empty-card-file case" >&2
    exit 1
fi

if [ -s data/cards.txt ]; then
    echo "[verify] cards file should stay empty for empty-card-file case" >&2
    exit 1
fi

if [ -f data/billings.txt ] && [ -s data/billings.txt ]; then
    echo "[verify] billing file should stay empty for empty-card-file case" >&2
    exit 1
fi
