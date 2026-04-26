#!/usr/bin/env bash
set -euo pipefail

cmp -s data/cards.txt build/test_state/menu_refund_balance_zero.expected_cards.txt || {
    echo "[verify] cards file changed unexpectedly for zero-balance refund" >&2
    exit 1
}
[ ! -f data/money.txt ] || [ ! -s data/money.txt ] || {
    echo "[verify] money file should stay empty for zero-balance refund" >&2
    exit 1
}
