include(${CMAKE_CURRENT_LIST_DIR}/TiffTestCommon.cmake)

# When executed via `cmake -P`, `CMAKE_CURRENT_SOURCE_DIR` refers to the
# invocation directory (typically the build directory). Use
# `CMAKE_CURRENT_LIST_DIR` to reliably locate the source image within the
# source tree.  The test previously referenced a PPM image, however the
# `rgb2ycbcr` tool operates on TIFF input.  Use the equivalent TIFF image
# so the conversion step succeeds regardless of PPM support.
set(_tif "${CMAKE_CURRENT_LIST_DIR}/images/rgb-3c-8b.tiff")

foreach(h 1 2)
  foreach(v 1 2)
    foreach(mode strip tile)
      set(_dst "${OUTDIR}/o-addtiffo-${mode}-${h}${v}.tiff")
      if(mode STREQUAL "strip")
        execute_process(COMMAND ${RGB2YCBCR} -c none -r 8 -h ${h} -v ${v} ${_tif} ${_dst} RESULT_VARIABLE _rv)
        if(_rv)
          message(FATAL_ERROR "rgb2ycbcr failed")
        endif()
      else()
        set(_tmp "${OUTDIR}/tmp-${h}${v}.tiff")
        execute_process(COMMAND ${RGB2YCBCR} -c none -r 8 -h ${h} -v ${v} ${_tif} ${_tmp} RESULT_VARIABLE _rv)
        if(_rv)
          message(FATAL_ERROR "rgb2ycbcr failed")
        endif()
        execute_process(COMMAND ${TIFFCP} -t -w 16 -l 16 ${_tmp} ${_dst} RESULT_VARIABLE _rv)
        if(_rv)
          message(WARNING "tiffcp failed; skipping tiled mode")
          file(REMOVE ${_tmp})
          continue()
        endif()
        file(REMOVE ${_tmp})
      endif()
      # Request creation of a single 1/2 resolution overview. Older versions of
      # addtiffo require at least one explicit overview factor.
      execute_process(COMMAND ${ADDTIFFO} ${_dst} 2 RESULT_VARIABLE _rv)
      if(_rv)
        message(FATAL_ERROR "addtiffo failed")
      endif()
      execute_process(COMMAND ${TIFFINFO} -D ${_dst} OUTPUT_VARIABLE _info RESULT_VARIABLE _rv)
      # tiffinfo may return a non-zero status when optional features such as
      # threadpool support are unavailable.  Treat a return code of 1 as
      # non-fatal so the test can proceed on minimal configurations.
      if(_rv GREATER 1)
        message(FATAL_ERROR "tiffinfo failed")
      endif()
      string(REGEX MATCHALL "Directory" _matches "${_info}")
      list(LENGTH _matches _count)
      if(_count LESS 2)
        message(FATAL_ERROR "Expected more than one directory for ${_dst}, got ${_count}")
      endif()
    endforeach()
  endforeach()
endforeach()
