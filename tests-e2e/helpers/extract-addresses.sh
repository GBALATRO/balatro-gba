#!/usr/bin/env bash
# Extract symbol addresses from the ELF file into addresses.json.
# Tries local arm-none-eabi-nm first, falls back to Docker.
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
REPO_ROOT="${SCRIPT_DIR}/../.."
ELF="${REPO_ROOT}/build/balatro-gba.elf"
OUT="${SCRIPT_DIR}/../addresses.json"

if [ ! -f "$ELF" ]; then
  echo "ERROR: ELF file not found at $ELF — build the ROM first" >&2
  exit 1
fi

SYMBOLS="game_state hand_state play_state score chips mult hands discards rng_seed hand_type ante round money current_blind"

# Run nm on the ELF with --print-size. Tries local binary first, falls back to Docker.
NM_FLAGS="--print-size"

get_nm_output() {
  if command -v arm-none-eabi-nm &>/dev/null; then
    arm-none-eabi-nm $NM_FLAGS "$ELF"
  elif [ -x "/opt/devkitpro/devkitARM/bin/arm-none-eabi-nm" ]; then
    /opt/devkitpro/devkitARM/bin/arm-none-eabi-nm $NM_FLAGS "$ELF"
  elif command -v docker &>/dev/null; then
    docker run --rm -v "${REPO_ROOT}:/balatro-gba" gbalatro:dev \
      /opt/devkitpro/devkitARM/bin/arm-none-eabi-nm $NM_FLAGS /balatro-gba/build/balatro-gba.elf
  else
    echo "ERROR: arm-none-eabi-nm not found and docker is not installed." >&2
    exit 1
  fi
}

# Run nm once and reuse the output
NM_OUTPUT=$(get_nm_output)

# Write to a temp file, then atomically rename.
# This prevents corruption when multiple processes run concurrently.
TMP="${OUT}.$$"

echo "{" > "$TMP"
FIRST=true
for sym in $SYMBOLS; do
  # --print-size output: "address size type name"
  LINE=$(echo "$NM_OUTPUT" | grep " ${sym}$")
  if [ -n "$LINE" ]; then
    ADDR=$(echo "$LINE" | awk '{print "0x"$1}')
    SIZE=$(echo "$LINE" | awk '{print "0x"$2}')
    SIZE_DEC=$(printf "%d" "$SIZE")
    if [ "$FIRST" = true ]; then
      FIRST=false
    else
      echo "," >> "$TMP"
    fi
    printf '  "%s": { "address": "%s", "size": %d }' "$sym" "$ADDR" "$SIZE_DEC" >> "$TMP"
  fi
done
echo "" >> "$TMP"
echo "}" >> "$TMP"

mv "$TMP" "$OUT"

echo "Extracted addresses to $OUT"
