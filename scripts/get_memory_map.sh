#!/usr/bin/env bash

set -euo pipefail

usage() {
    echo "Usage: $(basename "$0") <elf-file> [pool-def-file]"
    echo "  <elf-file>       Path to the built .elf file (e.g. build/balatro-gba.elf)"
    echo "  [pool-def-file]  Path to the mempool definition header"
    echo "                   (default: ./include/def_balatro_mempool.h)"
    exit 1
}

if [ $# -lt 1 ]; then
    usage
fi

ELF_FILE="$1"
POOL_DEF_FILE="${2-./include/def_balatro_mempool.h}"
READELF="${READELF:-arm-none-eabi-readelf}"
TOTAL_BYTES=0

if [ ! -f "$POOL_DEF_FILE" ]; then
  echo "Mempool definition file not found: $POOL_DEF_FILE" 
  usage
fi

if [ ! -f "$ELF_FILE" ]; then
    echo "ELF file not found or is not a regular file: $ELF_FILE"
    usage
fi

if ! command -v "$READELF" >/dev/null 2>&1; then
    echo "ERROR: missing tool: $READELF"
    usage
fi

print_line_break() {
    echo "--------------------------------------------------------------------"
}

get_pool_names() {
    grep POOL_ENTRY "$POOL_DEF_FILE" | sed -n 's@.*(\(.*\)).*@\1@p' | sed 's@,@@g' | cut -d ' ' -f 1
}

print_line_break
printf "%-16s| %-10s | %-10s | %-10s | %-10s \n" "Object" "address" "pool size" "func size" "bitmap size"
print_line_break

for name in $(get_pool_names); do
    output_pool="$(                                  \
            "$READELF" -sW "$ELF_FILE"             | \
            grep "${name}_"                        | \
            grep OBJECT                            | \
            grep storage                           | \
            sed -E 's@ +@ @g; s@^ @@'                \
        )"
    output_func="$(                                  \
            "$READELF" -sW "$ELF_FILE"             | \
            grep -E "pool_free|pool_get|pool_init" | \
            grep -E "${name}$"                     | \
            sed -E 's@ +@ @g; s@^ @@'              | \
            tr -d '\n'                               \
        )"
    output_bitset="$(                                \
            "$READELF" -sW "$ELF_FILE"             | \
            grep -E "${name}_bitset_w"             | \
            grep OBJECT                            | \
            sed -E 's@ +@ @g; s@^ @@'              | \
            tr -d '\n'                               \
        )"

    address="$(cut -d ' ' -f 2 <<< $output_pool)"
    pool_size="$(cut -d ' ' -f 3 <<< $output_pool)"
    func_size="$(cut -d ' ' -f 3 <<< $output_func)"
    bitset_size="$(cut -d ' ' -f 3 <<< $output_bitset)"

    TOTAL_BYTES=$(( TOTAL_BYTES + pool_size + func_size + bitset_size ))

    printf "%-16s| 0x%8s | %-10u | %-10u | %-10u \n" "$name" "$address" "$pool_size" "$func_size" "$bitset_size"
done

print_line_break
echo Total bytes used: $TOTAL_BYTES
