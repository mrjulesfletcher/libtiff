#!/bin/bash
set -euo pipefail
. ${srcdir:-.}/common.sh
infile="${IMAGES}/TEST_JPEG.jpg"
outfile="o-tiffcp-jpeg-invalid.tiff"
rm -f "$outfile"
echo "${MEMCHECK[@]} ${TIFFCP} -c none $infile $outfile"
if "${MEMCHECK[@]}" "${TIFFCP}" -c none "$infile" "$outfile"; then
  echo "Unexpected success converting JPEG file"
  exit 1
else
  echo "tiffcp failed as expected"
fi
