#!/usr/bin/env bash
set -euo pipefail

gcc -std=c11 -Wall -Wextra -pedantic -Iinclude \
    tests/exp4/helpers/repository_delete_api_check.c src/data/repository.c \
    -o build/bin/repository_delete_api_check

build/bin/repository_delete_api_check
