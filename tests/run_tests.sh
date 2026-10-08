#!/bin/sh
set -e
cd "$(dirname "$0")/.."
gcc -O2 -Wall -Wextra -std=c11 -Itests -o tests/test_wm tests/test_wm.c src/wm.c src/config.c
./tests/test_wm
