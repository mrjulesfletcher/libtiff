include(${CMAKE_CURRENT_LIST_DIR}/TiffTestCommon.cmake)
string(REPLACE "^" ";" CODEC "${CODEC}")
set(cmd "${TIFFCP};-c;${CODEC}")

test_convert("${cmd}" "${INFILE}" "${OUTFILE}")
tiff_compare("${OUTFILE}" "${INFILE}")
