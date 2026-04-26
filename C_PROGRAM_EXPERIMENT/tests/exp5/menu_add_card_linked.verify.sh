#!/usr/bin/env bash
set -euo pipefail

gcc -std=c11 -Wall -Wextra -pedantic -Iinclude \
    tests/exp5/helpers/menu_add_card_linked_check.c src/data/repository.c \
    -o build/bin/menu_add_card_linked_check

build/bin/menu_add_card_linked_check
