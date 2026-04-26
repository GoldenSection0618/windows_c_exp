#!/usr/bin/env bash
set -euo pipefail

gcc -std=c11 -Wall -Wextra -pedantic -Iinclude \
    tests/exp5/helpers/repository_list_foundation_check.c src/data/repository.c \
    -o build/bin/repository_list_foundation_check

build/bin/repository_list_foundation_check
