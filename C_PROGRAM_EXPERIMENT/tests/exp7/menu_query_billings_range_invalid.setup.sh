#!/usr/bin/env bash
set -euo pipefail

card_start_time="$(date -d '30 days ago' '+%Y-%m-%d %H:%M:%S')"
card_end_time="$(date -d '365 days' '+%Y-%m-%d %H:%M:%S')"
card_last_time="$(date -d '1 day ago' '+%Y-%m-%d %H:%M:%S')"
billing_start="$(date -d '40 minutes ago' '+%Y-%m-%d %H:%M:%S')"
billing_end="$(date -d '20 minutes ago' '+%Y-%m-%d %H:%M:%S')"

mkdir -p data
cat > data/cards.txt <<DATA
billrange3|pwd001|0|${card_start_time}|${card_end_time}|0|${card_last_time}|0|10000|0
DATA
cat > data/billings.txt <<DATA
billrange3|${billing_start}|${billing_end}|90|1|0
DATA
