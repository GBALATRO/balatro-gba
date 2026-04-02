#!/usr/bin/env bash

set -e

make
timestamp=$(date +%Y%m%d_%H%M%S)
arg=${1:-"build"}
dir="saved_builds/${arg}_${timestamp}"
mkdir -p "$dir"
cp build/balatro-gba.elf build/balatro-gba.gba build/balatro-gba.map "$dir/"
echo "Build saved to $dir"