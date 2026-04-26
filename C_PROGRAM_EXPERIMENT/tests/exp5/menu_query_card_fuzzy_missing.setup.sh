#!/usr/bin/env bash
set -euo pipefail

bash "$(dirname "$0")/../exp3/helpers/reset_card_file.sh"
cat > data/cards.txt <<'EOD'
otherC1|pwd001|0|2026-01-01 00:00:00|2027-01-01 00:00:00|0|2026-01-01 00:00:00|0|10000|0
otherC2|pwd002|0|2026-01-02 00:00:00|2027-01-02 00:00:00|0|2026-01-02 00:00:00|0|20000|0
EOD
