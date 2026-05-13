#!/usr/bin/env bash
# Dump the ELF symbol table to nm_output.txt for the TypeScript test setup to parse.
#
# Uses the devkitARM arm-none-eabi-nm if available, otherwise any in PATH.
# To run without devkitARM installed locally, execute this script inside the
# gbalatro docker container (e.g. via `docker compose run --rm gbalatro ...`).
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
REPO_ROOT="${SCRIPT_DIR}/../.."
ELF="${REPO_ROOT}/build/balatro-gba.elf"
OUT="${SCRIPT_DIR}/../nm_output.txt"

if [ ! -f "$ELF" ]; then
    echo "ERROR: ELF file not found at $ELF — build the ROM first" >&2
    exit 1
fi

NM_FLAGS="--print-size"

# Prefer the devkitARM cross compiler (DEVKITARM is already required for `make`).
# Fall back to any arm-none-eabi-nm in PATH, which may be a different version
# but is usually close enough for symbol extraction.
TMP="${OUT}.$$"
if [ -n "${DEVKITARM-}" ] && [ -x "${DEVKITARM}/bin/arm-none-eabi-nm" ]; then
    "${DEVKITARM}/bin/arm-none-eabi-nm" $NM_FLAGS "$ELF" > "$TMP"
elif command -v arm-none-eabi-nm &>/dev/null; then
    arm-none-eabi-nm $NM_FLAGS "$ELF" > "$TMP"
else
    echo "ERROR: arm-none-eabi-nm not found. Install devkitARM (and set \$DEVKITARM), or run this script inside the gbalatro docker container." >&2
    exit 1
fi

# Atomic rename to prevent corruption when multiple processes run concurrently.
mv "$TMP" "$OUT"

echo "Wrote nm output to $OUT"
