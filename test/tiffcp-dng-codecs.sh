#!/bin/bash
set -euo pipefail
. ${srcdir:-.}/common.sh
infile="${IMAGES}/TEST_CINEPI_LIBTIFF_DNG.dng"
for codec in none lzw zip packbits; do
  outfile="o-tiffcp-dng-${codec}.tiff"
  rm -f "$outfile"
  echo "${MEMCHECK[@]} ${TIFFCP} -c $codec $infile $outfile"
  "${MEMCHECK[@]}" "${TIFFCP}" -c $codec "$infile" "$outfile"
  status=$?
  if [ "$status" != 0 ]; then
    echo "Returned failed status $status!"
    exit $status
  fi
  echo "${MEMCHECK[@]} ${TIFFCMP} $outfile $infile"
  "${MEMCHECK[@]}" "${TIFFCMP}" "$outfile" "$infile"
  status=$?
  if [ "$status" != 0 ]; then
    echo "Returned failed status $status!"
    echo "\"$outfile\" differs from input"
    exit $status
  fi
  rm -f "$outfile"
done
