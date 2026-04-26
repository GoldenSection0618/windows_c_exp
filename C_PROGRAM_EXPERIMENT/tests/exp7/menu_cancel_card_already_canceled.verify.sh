#!/usr/bin/env bash
set -euo pipefail

cmp -s data/cards.txt build/test_state/menu_cancel_card_already_canceled.expected_cards.txt || {
    echo "[verify] cards file changed unexpectedly for canceled-card cancel" >&2
    exit 1
}
[ ! -f data/money.txt ] || [ ! -s data/money.txt ] || {
    echo "[verify] money file should stay empty for canceled-card cancel" >&2
    exit 1
}
