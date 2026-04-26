#!/usr/bin/env bash
set -euo pipefail

input_file="${2:-}"
card_start_time="$(date -d '30 days ago' '+%Y-%m-%d %H:%M:%S')"
card_end_time="$(date -d '365 days' '+%Y-%m-%d %H:%M:%S')"
card_last_time="$(date -d '1 day ago' '+%Y-%m-%d %H:%M:%S')"
billing_start_old="$(date -d '2 days ago' '+%Y-%m-%d %H:%M:%S')"
billing_end_old="$(date -d '2 days ago 10 minutes' '+%Y-%m-%d %H:%M:%S')"
billing_start_new="$(date -d '40 minutes ago' '+%Y-%m-%d %H:%M:%S')"
billing_end_new="$(date -d '20 minutes ago' '+%Y-%m-%d %H:%M:%S')"
query_start="$(date -d '1 day ago' '+%Y-%m-%d %H:%M:%S')"
query_end="$(date '+%Y-%m-%d %H:%M:%S')"

mkdir -p data
cat > data/cards.txt <<DATA
billrange1|pwd001|0|${card_start_time}|${card_end_time}|0|${card_last_time}|0|10000|0
DATA
cat > data/billings.txt <<DATA
billrange1|${billing_start_old}|${billing_end_old}|50|1|0
billrange1|${billing_start_new}|${billing_end_new}|90|1|0
DATA

if [ -n "${input_file}" ]; then
    cat > "${input_file}" <<DATA
7
1
billrange1
2
${query_start}
${query_end}
0
DATA
fi
