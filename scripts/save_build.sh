#!/usr/bin/env bash

# NOTE: This script is meant to be run from
# the root repository directory to work as intended.

set -e

usage() {
    echo "Usage: $(basename "$0") <rom-file> [build-label]"
    echo "  <rom-file>    Path to the .gba ROM file (e.g. build/balatro-gba.gba)"
    echo "  [build-label] Optional label for the saved build directory (default: \"build\")"
    exit 1
}

if [ $# -lt 1 ]; then
    usage
fi

ROM_FILE="$1"

make

if [ ! -f "$ROM_FILE" ]; then
    echo "Error: ROM file not found or is not a regular file: $ROM_FILE"
    usage
fi

# Derive the game base name from the ROM file (strip directory and .gba extension)
GAME_NAME="$(basename "$ROM_FILE" .gba)"
timestamp=$(date +%Y%m%d_%H%M%S)
arg="${2:-build}"
dir="saved_builds/${arg}_${timestamp}"

mkdir -p "$dir"

failed=0
for fe in elf gba map; do
    src="$(dirname "$ROM_FILE")/${GAME_NAME}.${fe}"
    if [ -f "$src" ]; then
        cp "$src" "$dir/"
    else
        echo "Warning: $src not found"
        failed=1
    fi
done

if [ $failed -eq 1 ]; then
    echo "Some files are missing, partially saved to $dir"
else
    echo "Build files saved to $dir"
fi

exit $failed
