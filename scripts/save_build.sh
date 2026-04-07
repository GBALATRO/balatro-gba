#!/usr/bin/env bash

# NOTE: This script is meant to be run from
# the root repository directory to work as intended.

set -e

make

GAME_NAME="${GAME_NAME-balatro-gba}"
timestamp=$(date +%Y%m%d_%H%M%S)
arg=${1:-"build"}
dir="saved_builds/${arg}_${timestamp}"

mkdir -p "$dir"

failed=0
for fe in elf gba map; do
    if [ -f build/"$GAME_NAME".$fe ]; then
        cp build/"$GAME_NAME".$fe "$dir/"
    else
        echo "Warning: build/$GAME_NAME.$fe not found"
        failed=1
    fi
done

if [ $failed -eq 1 ]; then
    echo "Some files are missing, partially saved to $dir"
else
    echo "Build files saved to $dir"
fi

exit $failed

