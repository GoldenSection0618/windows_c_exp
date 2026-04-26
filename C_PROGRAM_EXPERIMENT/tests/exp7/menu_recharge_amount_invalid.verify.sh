#!/usr/bin/env bash
set -euo pipefail

cmp -s data/cards.txt build/test_state/menu_recharge_amount_invalid.expected_cards.txt || {
    echo "[verify] cards file changed unexpectedly for invalid-amount recharge" >&2
    exit 1
}
[ ! -f data/money.txt ] || [ ! -s data/money.txt ] || {
    echo "[verify] money file should stay empty for invalid-amount recharge" >&2
    exit 1
}
