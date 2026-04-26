#!/usr/bin/env bash
set -euo pipefail

cmp -s data/cards.txt build/test_state/menu_recharge_password_wrong.expected_cards.txt || {
    echo "[verify] cards file changed unexpectedly for wrong-password recharge" >&2
    exit 1
}
[ ! -f data/money.txt ] || [ ! -s data/money.txt ] || {
    echo "[verify] money file should stay empty for wrong-password recharge" >&2
    exit 1
}
