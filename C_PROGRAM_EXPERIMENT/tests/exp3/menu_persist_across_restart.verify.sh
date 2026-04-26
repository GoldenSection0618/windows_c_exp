#!/usr/bin/env bash
set -euo pipefail

binary="$1"
query_input="build/exp3_persist_query.input"
query_output="build/exp3_persist_query.out"

cat > "$query_input" <<'EOD'
2
persist01
0
EOD

"$binary" < "$query_input" > "$query_output"

if ! grep -qF '查询结果：' "$query_output"; then
    echo "[verify] persistence query missing result" >&2
    exit 1
fi

if ! grep -qF 'persist01' "$query_output"; then
    echo "[verify] persistence query missing card name" >&2
    exit 1
fi
