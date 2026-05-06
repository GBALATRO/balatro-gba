#!/bin/bash

awk '
{
  sum += $3
  n++

  if (n == 1 || $3 < min) min = $3
  if (n == 1 || $3 > max) max = $3
}

END {
  printf "avg = %.2f\n", sum / n
  print  "min =", min
  print  "max =", max
}
' $1