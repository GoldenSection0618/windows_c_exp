#!/usr/bin/env bash
set -euo pipefail

bash "$(dirname "$0")/../exp3/helpers/reset_card_file.sh"
printf 'otherQ1|pwd001|0|2026-01-01 00:00:00|2027-01-01 00:00:00|0|2026-01-01 00:00:00|0|10000|0\n' > data/cards.txt
