#!/bin/bash
set -euo pipefail
. ${srcdir:-.}/common.sh

jpeg="${IMAGES}/TEST_JPEG.jpg"
dng="${IMAGES}/TEST_CINEPI_LIBTIFF_DNG.dng"
tmpdir=${TMPDIR:-/tmp}

expect_fail() {
  if "$@"; then
    echo "Unexpected success running $*"
    exit 1
  else
    echo "Command failed as expected: $*"
  fi
}

# tiffinfo
expect_fail "${TIFFINFO}" "${jpeg}"
f_test_reader "${TIFFINFO}" "${dng}"

# tiffdump
expect_fail "${TIFFDUMP}" "${jpeg}"
f_test_reader "${TIFFDUMP}" "${dng}"

# tiffcp
outfile="${tmpdir}/tools-jpeg-dng.tiff"
expect_fail "${TIFFCP}" "${jpeg}" "${outfile}"
f_test_convert "${TIFFCP}" "${dng}" "${outfile}"
"${TIFFCMP}" "${outfile}" "${dng}"
rm -f "${outfile}"

# tiffcrop
outfile="${tmpdir}/tools-jpeg-dng-crop.tiff"
expect_fail "${TIFFCROP}" "${jpeg}" "${outfile}"
f_test_convert "${TIFFCROP} -U px -E top -X 10 -Y 10" "${dng}" "${outfile}"
f_tiffinfo_validate "${outfile}"
rm -f "${outfile}"

# tiffset
outfile="${tmpdir}/tools-jpeg-dng-set.tiff"
cp "${dng}" "${outfile}"
expect_fail "${TIFFSET}" -s 270 test "${jpeg}"
"${TIFFSET}" -s 270 "desc" "${outfile}"
"${TIFFINFO}" "${outfile}" | grep -q "desc"
rm -f "${outfile}"

# tiffsplit
outfile_base="${tmpdir}/tools-jpeg-dng-split-"
expect_fail "${TIFFSPLIT}" "${jpeg}" "${outfile_base}"
"${TIFFSPLIT}" "${dng}" "${outfile_base}"
f_tiffinfo_validate "${outfile_base}0.tif"
rm -f "${outfile_base}"*.tif

# tiff2pdf
outfile="${tmpdir}/tools-jpeg-dng.pdf"
expect_fail "${TIFF2PDF}" -o "${outfile}" "${jpeg}"
"${TIFF2PDF}" -o "${outfile}" "${dng}"
[ -s "${outfile}" ]
rm -f "${outfile}"

echo "All JPEG and DNG tool tests passed"

