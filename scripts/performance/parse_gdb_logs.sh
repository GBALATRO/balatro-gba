#!/bin/bash

if [ -z "$1" ]; then
  echo "Usage: $0 <gdb-output-file>"
  exit 2
fi

# Parse only MI output lines that contain the quoted value (lines starting with ~)
# and extract the numeric value after the '='. Compute avg/min/max robustly.
awk '
/^~/ {
  if (match($0, /= *([0-9]+(\.[0-9]+)?)/, m)) {
    val = m[1] + 0
    sum += val
    n++
    if (n == 1 || val < min) min = val
    if (n == 1 || val > max) max = val
  }
}
END {
  if (n > 0) {
    printf "avg = %.2f\nmin = %s\nmax = %s\n", sum / n, min, max
  } else {
    print "no numeric data found"
    exit 1
  }
}
' "$1"