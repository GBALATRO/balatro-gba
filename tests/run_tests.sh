#!/bin/bash

set -eu

run_test() {
    name="$1"

    echo "==============================================================================="
    echo "Running test for: $1"
    echo "==============================================================================="

    cd "$name" 2>&1 > /dev/null
    make clean > /dev/null
    make > /dev/null
    ./build/"$name"_test
    cd - 2>&1 > /dev/null
}

run_test bitset
run_test list
run_test pool
run_test util
