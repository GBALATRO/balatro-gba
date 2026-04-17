#!/usr/bin/env bash
# Extract symbol addresses from the ELF file into addresses.json.
# Uses the devkitARM arm-none-eabi-nm if available, otherwise any in PATH.
# To run without devkitARM installed locally, execute this script inside the
# gbalatro docker container (e.g. via `docker compose run --rm gbalatro ...`).
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

NM_FLAGS="--print-size"

# Prefer the devkitARM cross compiler (DEVKITARM is already required for `make`).
# Fall back to any arm-none-eabi-nm in PATH, which may be a different version
# but is usually close enough for symbol extraction.
get_nm_output() {
    if [ -n "${DEVKITARM-}" ] && [ -x "${DEVKITARM}/bin/arm-none-eabi-nm" ]; then
        "${DEVKITARM}/bin/arm-none-eabi-nm" $NM_FLAGS "$ELF"
    elif command -v arm-none-eabi-nm &>/dev/null; then
        arm-none-eabi-nm $NM_FLAGS "$ELF"
    else
        echo "ERROR: arm-none-eabi-nm not found. Install devkitARM (and set \$DEVKITARM), or run this script inside the gbalatro docker container." >&2
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
