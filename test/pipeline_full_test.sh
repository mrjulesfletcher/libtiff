#!/bin/bash
set -euo pipefail
#
# Run a pipeline of libtiff tools: tiffcp -> tiffcrop -> tiff2rgba -> tiffdump
# Validate the final TIFF file with tiffinfo.
#
. ${srcdir:-.}/common.sh

input="${IMG_RGB_3C_8B}"
tmpdir=${TMPDIR:-/tmp}
step1="${tmpdir}/pipeline_step1.tiff"
step2="${tmpdir}/pipeline_step2.tiff"
step3="${tmpdir}/pipeline_step3.tiff"

f_test_convert "${TIFFCP} -c lzw" "${input}" "${step1}"
f_test_convert "${TIFFCROP} -U px -E top -X 20 -Y 20" "${step1}" "${step2}"
f_test_convert "${TIFF2RGBA}" "${step2}" "${step3}"
f_test_reader "${TIFFDUMP}" "${step3}"
f_tiffinfo_validate "${step3}"
rm -f "${step1}" "${step2}" "${step3}"
