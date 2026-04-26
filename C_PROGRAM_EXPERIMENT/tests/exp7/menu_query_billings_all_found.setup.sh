#!/usr/bin/env bash
set -euo pipefail

card_start_time="$(date -d '30 days ago' '+%Y-%m-%d %H:%M:%S')"
card_end_time="$(date -d '365 days' '+%Y-%m-%d %H:%M:%S')"
card_last_time="$(date -d '1 day ago' '+%Y-%m-%d %H:%M:%S')"
billing_start_old="$(date -d '3 hours ago' '+%Y-%m-%d %H:%M:%S')"
billing_end_old="$(date -d '2 hours ago' '+%Y-%m-%d %H:%M:%S')"
billing_start_new="$(date -d '50 minutes ago' '+%Y-%m-%d %H:%M:%S')"

mkdir -p data
cat > data/cards.txt <<DATA
billqry01|pwd001|0|${card_start_time}|${card_end_time}|0|${card_last_time}|0|10000|0
DATA
cat > data/billings.txt <<DATA
billqry01|${billing_start_old}|${billing_end_old}|120|1|0
billqry01|${billing_start_new}|0|0|0|0
DATA
