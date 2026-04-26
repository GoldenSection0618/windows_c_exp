#!/usr/bin/env bash
set -euo pipefail
bash "$(dirname "$0")/helpers/reset_card_file.sh"
printf 'cardApi|pwd001|0|2026-01-01 00:00:00|2027-01-01 00:00:00|0|2026-01-01 00:00:00|0|20000|0\n' > data/cards.txt
