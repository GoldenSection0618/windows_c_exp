#!/usr/bin/env bash

set -euo pipefail

if [ "$#" -ne 5 ]; then
    echo "Usage: $0 <binary> <test_dir> <suites_file> <output_dir> <single_runner>" >&2
    exit 2
fi

binary="$1"
test_dir="$2"
suites_file="$3"
output_dir="$4"
single_runner="$5"

if [ ! -f "$binary" ]; then
    echo "[test-batch] error: binary not found: $binary" >&2
    exit 1
fi

if [ ! -d "$test_dir" ]; then
    echo "[test-batch] error: test dir not found: $test_dir" >&2
    exit 1
fi

if [ ! -f "$suites_file" ]; then
    echo "[test-batch] error: suites file not found: $suites_file" >&2
    exit 1
fi

if [ ! -f "$single_runner" ]; then
    echo "[test-batch] error: runner not found: $single_runner" >&2
    exit 1
fi

mkdir -p "$output_dir"

failed=0

while IFS= read -r suite || [ -n "$suite" ]; do
    suite="$(echo "$suite" | sed 's/^[[:space:]]*//; s/[[:space:]]*$//')"
    if [ -z "$suite" ] || [[ "$suite" == \#* ]]; then
        continue
    fi

    input_file="$test_dir/$suite.input"
    expect_file="$test_dir/$suite.expect.tsv"
    output_file="$output_dir/test_output_$suite.txt"

    echo "[test-batch] suite=$suite"
    if ! bash "$single_runner" "$binary" "$input_file" "$expect_file" "$output_file"; then
        failed=1
    fi
done < "$suites_file"

if [ "$failed" -ne 0 ]; then
    echo "[test-batch] failed. suites file: $suites_file" >&2
    exit 1
fi

echo "[test-batch] all suites passed ($suites_file)"
