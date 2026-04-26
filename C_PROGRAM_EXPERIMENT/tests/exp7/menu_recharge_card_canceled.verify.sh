#!/usr/bin/env bash
set -euo pipefail

cmp -s data/cards.txt build/test_state/menu_recharge_card_canceled.expected_cards.txt || {
    echo "[verify] cards file changed unexpectedly for canceled-card recharge" >&2
    exit 1
}
[ ! -f data/money.txt ] || [ ! -s data/money.txt ] || {
    echo "[verify] money file should stay empty for canceled-card recharge" >&2
    exit 1
}
