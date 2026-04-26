#!/usr/bin/env bash
set -euo pipefail

start_time="$(date -d '20 seconds ago' '+%Y-%m-%d %H:%M:%S')"
card_start_time="$(date -d '30 days ago' '+%Y-%m-%d %H:%M:%S')"
card_end_time="$(date -d '365 days' '+%Y-%m-%d %H:%M:%S')"
card_last_time="$(date -d '1 minute ago' '+%Y-%m-%d %H:%M:%S')"

mkdir -p data
cat > data/cards.txt <<DATA
stopA4|pwd001|1|${card_start_time}|${card_end_time}|0|${card_last_time}|0|10000|0
DATA
cat > data/billings.txt <<DATA
stopA4|${start_time}|0|0|0|0
DATA
