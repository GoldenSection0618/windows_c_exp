#!/usr/bin/env bash
set -euo pipefail

binary="$1"
query_input="build/exp5_persist_query.input"
query_output="build/exp5_persist_query.out"

cat > "$query_input" <<'EOD'
2
1
persistL1
0
EOD

"$binary" < "$query_input" > "$query_output"

if ! grep -qF '查询结果：' "$query_output"; then
    echo "[verify] persistence query missing result" >&2
    exit 1
fi

if ! grep -qF 'persistL1' "$query_output"; then
    echo "[verify] persistence query missing card name" >&2
    exit 1
fi

gcc -std=c11 -Wall -Wextra -pedantic -Iinclude \
    tests/exp5/helpers/menu_persist_linked_flow_check.c src/data/repository.c \
    -o build/bin/menu_persist_linked_flow_check

build/bin/menu_persist_linked_flow_check
