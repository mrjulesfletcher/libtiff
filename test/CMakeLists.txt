# CMake build for libtiff
#
# Copyright © 2015 Open Microscopy Environment / University of Dundee
# Written by Roger Leigh <rleigh@codelibre.net>
#
# Permission to use, copy, modify, distribute, and sell this software and
# its documentation for any purpose is hereby granted without fee, provided
# that (i) the above copyright notices and this permission notice appear in
# all copies of the software and related documentation, and (ii) the names of
# Sam Leffler and Silicon Graphics may not be used in any advertising or
# publicity relating to the software without the specific, prior written
# permission of Sam Leffler and Silicon Graphics.
#
# THE SOFTWARE IS PROVIDED "AS-IS" AND WITHOUT WARRANTY OF ANY KIND,
# EXPRESS, IMPLIED OR OTHERWISE, INCLUDING WITHOUT LIMITATION, ANY
# WARRANTY OF MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE.
#
# IN NO EVENT SHALL SAM LEFFLER OR SILICON GRAPHICS BE LIABLE FOR
# ANY SPECIAL, INCIDENTAL, INDIRECT OR CONSEQUENTIAL DAMAGES OF ANY KIND,
# OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
# WHETHER OR NOT ADVISED OF THE POSSIBILITY OF DAMAGE, AND ON ANY THEORY OF
# LIABILITY, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE
# OF THIS SOFTWARE.

# Images for tiff2bw (8-bit RGB only)
set(RGBIMAGES
    images/palette-1c-8b.tiff
    images/rgb-3c-8b.tiff
    images/quad-lzw-compat.tiff
)

# This list should contain all of the TIFF files in the 'images'
# subdirectory which are intended to be used as input images for
# tests.  All of these files should use the extension ".tiff".
set(TIFFIMAGES
    images/custom_dir_EXIF_GPS.tiff
    images/logluv-3c-16b.tiff
    images/minisblack-1c-16b.tiff
    images/minisblack-1c-8b.tiff
    images/minisblack-2c-8b-alpha.tiff
    images/miniswhite-1c-1b.tiff
    images/palette-1c-1b.tiff
    images/palette-1c-4b.tiff
    images/palette-1c-8b.tiff
    images/rgb-3c-16b.tiff
    images/rgb-3c-8b.tiff
    images/quad-lzw-compat.tiff
    images/lzw-single-strip.tiff
    images/testfax4.tiff
    images/testfax3_bug_513.tiff
    images/32bpp-None.tiff
)
# TIFF images using old-jpeg.  Unused unless JPEG support is enabled.
set(TIFFIMAGES_OJPEG)

if(JPEG_SUPPORT)
    list(APPEND TIFFIMAGES images/quad-tile.jpg.tiff)
endif()

add_test(NAME parse_results_pytest
         COMMAND python3 -m pytest -q
         ${CMAKE_SOURCE_DIR}/scripts/tests/test_parse_results.py)

add_test(NAME parse_results_async_pytest
         COMMAND python3 -m pytest -q
         ${CMAKE_SOURCE_DIR}/scripts/tests/test_parse_results_async.py)

if (OJPEG_SUPPORT)
    list(APPEND TIFFIMAGES_OJPEG
        images/ojpeg_zackthecat_subsamp22_single_strip.tiff
        images/ojpeg_chewey_subsamp21_multi_strip.tiff
        images/ojpeg_single_strip_no_rowsperstrip.tiff
    )
endif()

set(BMPIMAGES
    images/palette-1c-8b.bmp
    images/rgb-3c-8b.bmp)

set(GIFIMAGES
    images/palette-1c-8b.gif)

set(PNMIMAGES
    images/minisblack-1c-8b.pgm
    images/miniswhite-1c-1b.pbm
    images/rgb-3c-16b.ppm
    images/rgb-3c-8b.ppm)

# All uncompressed image files
set(UNCOMPRESSEDIMAGES
    images/minisblack-1c-16b.tiff
    images/minisblack-1c-8b.tiff
    images/miniswhite-1c-1b.tiff
    images/palette-1c-1b.tiff
    images/palette-1c-4b.tiff
    images/palette-1c-8b.tiff
    images/rgb-3c-8b.tiff)

# This list should include all of the files in the 'images'
# subdirectory which are intended to be distributed.  This may include
# files which are not currently used by the tests.
set(IMAGES_EXTRA_DIST
    images/README.txt
    images/miniswhite-1c-1b.g3
    images/32bpp-None-jpeg.tiff
    images/TEST_CINEPI_LIBTIFF_DNG.dng
    images/TEST_JPEG.jpg
    ${BMPIMAGES}
    ${GIFIMAGES}
    ${PNMIMAGES}
    ${TIFFIMAGES})

set(noinst_HEADERS tifftest.h)

set(simple_tests)

# Test programs
add_executable(test_signed_tags ../placeholder.h)
target_sources(test_signed_tags PRIVATE test_signed_tags.c)
set_target_properties(test_signed_tags PROPERTIES LINKER_LANGUAGE CXX)
target_link_libraries(test_signed_tags PRIVATE tiff tiff_port)
list(APPEND simple_tests test_signed_tags)

add_executable(ascii_tag ../placeholder.h)
target_sources(ascii_tag PRIVATE ascii_tag.c)
set_target_properties(ascii_tag PROPERTIES LINKER_LANGUAGE CXX)
target_link_libraries(ascii_tag PRIVATE tiff tiff_port)
list(APPEND simple_tests ascii_tag)

add_executable(register_custom_tags ../placeholder.h)
target_sources(register_custom_tags PRIVATE register_custom_tags.c)
set_target_properties(register_custom_tags PROPERTIES LINKER_LANGUAGE CXX)
target_link_libraries(register_custom_tags PRIVATE tiff tiff_port)
list(APPEND simple_tests register_custom_tags)

add_executable(test_open_jpeg_dng ../placeholder.h)
target_sources(test_open_jpeg_dng PRIVATE test_open_jpeg_dng.c)
set_target_properties(test_open_jpeg_dng PROPERTIES LINKER_LANGUAGE CXX)
target_link_libraries(test_open_jpeg_dng PRIVATE tiff tiff_port)
list(APPEND simple_tests test_open_jpeg_dng)

add_executable(test_bigtiff_roundtrip ../placeholder.h)
target_sources(test_bigtiff_roundtrip PRIVATE test_bigtiff_roundtrip.c)
set_target_properties(test_bigtiff_roundtrip PROPERTIES LINKER_LANGUAGE CXX)
target_link_libraries(test_bigtiff_roundtrip PRIVATE tiff tiff_port)
list(APPEND simple_tests test_bigtiff_roundtrip)

add_executable(long_tag ../placeholder.h)
target_sources(long_tag PRIVATE long_tag.c check_tag.c)
set_target_properties(long_tag PROPERTIES LINKER_LANGUAGE CXX)
target_link_libraries(long_tag PRIVATE tiff tiff_port)
list(APPEND simple_tests long_tag)

add_executable(short_tag ../placeholder.h)
target_sources(short_tag PRIVATE short_tag.c check_tag.c)
set_target_properties(short_tag PROPERTIES LINKER_LANGUAGE CXX)
target_link_libraries(short_tag PRIVATE tiff tiff_port)
list(APPEND simple_tests short_tag)

add_executable(strip_rw ../placeholder.h)
target_sources(strip_rw PRIVATE strip_rw.c strip.c test_arrays.c test_arrays.h)
set_target_properties(strip_rw PROPERTIES LINKER_LANGUAGE CXX)
target_link_libraries(strip_rw PRIVATE tiff tiff_port)
list(APPEND simple_tests strip_rw)

if(USE_IO_URING)
  add_executable(uring_rw ../placeholder.h)
  target_sources(uring_rw PRIVATE uring_rw.c)
  set_target_properties(uring_rw PROPERTIES LINKER_LANGUAGE CXX)
  target_link_libraries(uring_rw PRIVATE tiff tiff_port)
  list(APPEND simple_tests uring_rw)
endif()

add_executable(rewrite ../placeholder.h)
target_sources(rewrite PRIVATE rewrite_tag.c)
set_target_properties(rewrite PROPERTIES LINKER_LANGUAGE CXX)
target_link_libraries(rewrite PRIVATE tiff tiff_port)
list(APPEND simple_tests rewrite)

if(JPEG_SUPPORT)
  add_executable(raw_decode ../placeholder.h)
  target_sources(raw_decode PRIVATE raw_decode.c)
  set_target_properties(raw_decode PROPERTIES LINKER_LANGUAGE CXX)
  target_link_libraries(raw_decode PRIVATE tiff tiff_port JPEG::JPEG)
  list(APPEND simple_tests raw_decode)
endif()

add_executable(custom_dir ../placeholder.h)
target_sources(custom_dir PRIVATE custom_dir.c)
set_target_properties(custom_dir PROPERTIES LINKER_LANGUAGE CXX)
target_link_libraries(custom_dir PRIVATE tiff tiff_port)
list(APPEND simple_tests custom_dir)

# Uses private functions from static library
if(NOT BUILD_SHARED_LIBS)
  add_executable(rational_precision2double ../placeholder.h)
  target_sources(rational_precision2double PRIVATE rational_precision2double.c)
  set_target_properties(rational_precision2double PROPERTIES LINKER_LANGUAGE CXX)
  target_link_libraries(rational_precision2double PRIVATE tiff tiff_port)
  list(APPEND simple_tests rational_precision2double)
  
  add_executable(test_write_read_tags ../placeholder.h)
  target_sources(test_write_read_tags PRIVATE test_write_read_tags.c)
  set_target_properties(test_write_read_tags PROPERTIES LINKER_LANGUAGE CXX)
  target_link_libraries(test_write_read_tags PRIVATE tiff)
  list(APPEND simple_tests test_write_read_tags)
  
  add_executable(test_transferfunction_write_read ../placeholder.h)
  target_sources(test_transferfunction_write_read PRIVATE test_transferfunction_write_read.c)
  set_target_properties(test_transferfunction_write_read PROPERTIES LINKER_LANGUAGE CXX)
  target_link_libraries(test_transferfunction_write_read PRIVATE tiff)
  list(APPEND simple_tests test_transferfunction_write_read)
endif()

add_executable(custom_dir_EXIF_231 ../placeholder.h)
target_sources(custom_dir_EXIF_231 PRIVATE custom_dir_EXIF_231.c)
set_target_properties(custom_dir_EXIF_231 PROPERTIES LINKER_LANGUAGE CXX)
target_link_libraries(custom_dir_EXIF_231 PRIVATE tiff tiff_port)
list(APPEND simple_tests custom_dir_EXIF_231)

add_executable(defer_strile_loading ../placeholder.h)
target_sources(defer_strile_loading PRIVATE defer_strile_loading.c)
set_target_properties(defer_strile_loading PROPERTIES LINKER_LANGUAGE CXX)
target_link_libraries(defer_strile_loading PRIVATE tiff tiff_port)
list(APPEND simple_tests defer_strile_loading)

add_executable(defer_strile_writing ../placeholder.h)
target_sources(defer_strile_writing PRIVATE defer_strile_writing.c)
set_target_properties(defer_strile_writing PROPERTIES LINKER_LANGUAGE CXX)
target_link_libraries(defer_strile_writing PRIVATE tiff tiff_port)
list(APPEND simple_tests defer_strile_writing)

add_executable(test_directory ../placeholder.h)
target_sources(test_directory PRIVATE test_directory.c)
set_target_properties(test_directory PROPERTIES LINKER_LANGUAGE CXX)
target_link_libraries(test_directory PRIVATE tiff tiff_port)
list(APPEND simple_tests test_directory)

add_executable(test_IFD_enlargement ../placeholder.h)
target_sources(test_IFD_enlargement PRIVATE test_IFD_enlargement.c)
set_target_properties(test_IFD_enlargement PROPERTIES LINKER_LANGUAGE CXX)
target_link_libraries(test_IFD_enlargement PRIVATE tiff tiff_port)
list(APPEND simple_tests test_IFD_enlargement)

add_executable(testtypes ../placeholder.h)
target_sources(testtypes PRIVATE testtypes.c)
set_target_properties(testtypes PROPERTIES LINKER_LANGUAGE CXX)
target_link_libraries(testtypes PRIVATE tiff tiff_port)
list(APPEND simple_tests testtypes)

add_executable(test_open_options ../placeholder.h)
target_sources(test_open_options PRIVATE test_open_options.c)
set_target_properties(test_open_options PROPERTIES LINKER_LANGUAGE CXX)
target_link_libraries(test_open_options PRIVATE tiff tiff_port)
list(APPEND simple_tests test_open_options)

add_executable(test_append_to_strip ../placeholder.h)
target_sources(test_append_to_strip PRIVATE test_append_to_strip.c)
set_target_properties(test_append_to_strip PROPERTIES LINKER_LANGUAGE CXX)
target_link_libraries(test_append_to_strip PRIVATE tiff tiff_port)
list(APPEND simple_tests test_append_to_strip)

add_executable(test_seek_partial ../placeholder.h)
target_sources(test_seek_partial PRIVATE test_seek_partial.c)
set_target_properties(test_seek_partial PROPERTIES LINKER_LANGUAGE CXX)
target_link_libraries(test_seek_partial PRIVATE tiff tiff_port)
list(APPEND simple_tests test_seek_partial)

add_executable(test_ifd_loop_detection ../placeholder.h)
target_sources(test_ifd_loop_detection PRIVATE test_ifd_loop_detection.c)
set_target_properties(test_ifd_loop_detection PROPERTIES LINKER_LANGUAGE CXX)
target_link_libraries(test_ifd_loop_detection PRIVATE tiff tiff_port)
target_compile_definitions(test_ifd_loop_detection PRIVATE SOURCE_DIR=\"${CMAKE_CURRENT_SOURCE_DIR}\")
list(APPEND simple_tests test_ifd_loop_detection)

add_executable(test_invalid_callbacks ../placeholder.h)
target_sources(test_invalid_callbacks PRIVATE test_invalid_callbacks.c)
set_target_properties(test_invalid_callbacks PROPERTIES LINKER_LANGUAGE CXX)
target_link_libraries(test_invalid_callbacks PRIVATE tiff tiff_port)
list(APPEND simple_tests test_invalid_callbacks)

add_executable(test_client_open_stream ../placeholder.h)
target_sources(test_client_open_stream PRIVATE test_client_open_stream.c)
set_target_properties(test_client_open_stream PROPERTIES LINKER_LANGUAGE CXX)
target_link_libraries(test_client_open_stream PRIVATE tiff tiff_port)
list(APPEND simple_tests test_client_open_stream)

add_executable(swab_neon_test ../placeholder.h)
target_sources(swab_neon_test PRIVATE swab_neon_test.c)
set_target_properties(swab_neon_test PROPERTIES LINKER_LANGUAGE CXX)
target_link_libraries(swab_neon_test PRIVATE tiff tiff_port)
list(APPEND simple_tests swab_neon_test)

add_executable(assemble_strip_neon_test ../placeholder.h)
target_sources(assemble_strip_neon_test PRIVATE assemble_strip_neon_test.c)
set_target_properties(assemble_strip_neon_test PROPERTIES LINKER_LANGUAGE CXX)
target_link_libraries(assemble_strip_neon_test PRIVATE tiff tiff_port)
list(APPEND simple_tests assemble_strip_neon_test)

add_executable(assemble_strip_neon_alloc_fail ../placeholder.h)
target_sources(assemble_strip_neon_alloc_fail PRIVATE assemble_strip_neon_alloc_fail.c)
set_target_properties(assemble_strip_neon_alloc_fail PROPERTIES LINKER_LANGUAGE CXX)
target_link_libraries(assemble_strip_neon_alloc_fail PRIVATE tiff tiff_port failalloc)
list(APPEND simple_tests assemble_strip_neon_alloc_fail)

add_executable(gray_flip_neon_test ../placeholder.h)
target_sources(gray_flip_neon_test PRIVATE gray_flip_neon_test.c)
set_target_properties(gray_flip_neon_test PROPERTIES LINKER_LANGUAGE CXX)
target_link_libraries(gray_flip_neon_test PRIVATE tiff tiff_port)
list(APPEND simple_tests gray_flip_neon_test)

add_executable(ycbcr_neon_test ../placeholder.h)
target_sources(ycbcr_neon_test PRIVATE ycbcr_neon_test.c)
set_target_properties(ycbcr_neon_test PROPERTIES LINKER_LANGUAGE CXX)
target_link_libraries(ycbcr_neon_test PRIVATE tiff tiff_port)
list(APPEND simple_tests ycbcr_neon_test)

add_executable(memmove_simd_test ../placeholder.h)
target_sources(memmove_simd_test PRIVATE memmove_simd_test.c)
set_target_properties(memmove_simd_test PROPERTIES LINKER_LANGUAGE CXX)
target_link_libraries(memmove_simd_test PRIVATE tiff tiff_port)
list(APPEND simple_tests memmove_simd_test)

add_executable(reverse_bits_neon_test ../placeholder.h)
target_sources(reverse_bits_neon_test PRIVATE reverse_bits_neon_test.c)
set_target_properties(reverse_bits_neon_test PROPERTIES LINKER_LANGUAGE CXX)
target_link_libraries(reverse_bits_neon_test PRIVATE tiff tiff_port)
list(APPEND simple_tests reverse_bits_neon_test)

add_executable(bayer_pack_test ../placeholder.h)
target_sources(bayer_pack_test PRIVATE bayer_pack_test.c)
set_target_properties(bayer_pack_test PROPERTIES LINKER_LANGUAGE CXX)
target_link_libraries(bayer_pack_test PRIVATE tiff tiff_port)
list(APPEND simple_tests bayer_pack_test)

add_executable(rgb_pack_neon_test ../placeholder.h)
target_sources(rgb_pack_neon_test PRIVATE rgb_pack_neon_test.c)
set_target_properties(rgb_pack_neon_test PROPERTIES LINKER_LANGUAGE CXX)
target_link_libraries(rgb_pack_neon_test PRIVATE tiff tiff_port)
list(APPEND simple_tests rgb_pack_neon_test)

add_executable(bayer_neon_test ../placeholder.h)
target_sources(bayer_neon_test PRIVATE bayer_neon_test.c)
set_target_properties(bayer_neon_test PROPERTIES LINKER_LANGUAGE CXX)
target_link_libraries(bayer_neon_test PRIVATE tiff tiff_port)
list(APPEND simple_tests bayer_neon_test)

add_executable(dng_simd_compare ../placeholder.h)
target_sources(dng_simd_compare PRIVATE dng_simd_compare.c)
set_target_properties(dng_simd_compare PROPERTIES LINKER_LANGUAGE CXX)
target_link_libraries(dng_simd_compare PRIVATE tiff tiff_port)
list(APPEND simple_tests dng_simd_compare)

add_executable(predictor_sse41_test ../placeholder.h)
target_sources(predictor_sse41_test PRIVATE predictor_sse41_test.c)
set_target_properties(predictor_sse41_test PROPERTIES LINKER_LANGUAGE CXX)
target_link_libraries(predictor_sse41_test PRIVATE tiff tiff_port)
list(APPEND simple_tests predictor_sse41_test)
add_executable(bayer_simd_benchmark ../placeholder.h)
target_sources(bayer_simd_benchmark PRIVATE bayer_simd_benchmark.c)
set_target_properties(bayer_simd_benchmark PROPERTIES LINKER_LANGUAGE CXX)
target_link_libraries(bayer_simd_benchmark PRIVATE tiff tiff_port)
list(APPEND simple_tests bayer_simd_benchmark)

add_executable(pmull_hash_benchmark ../placeholder.h)
target_sources(pmull_hash_benchmark PRIVATE pmull_hash_benchmark.c)
set_target_properties(pmull_hash_benchmark PROPERTIES LINKER_LANGUAGE CXX)
target_link_libraries(pmull_hash_benchmark PRIVATE tiff tiff_port)
list(APPEND simple_tests pmull_hash_benchmark)
add_executable(swab_benchmark ../placeholder.h)
target_sources(swab_benchmark PRIVATE swab_benchmark.c)
set_target_properties(swab_benchmark PROPERTIES LINKER_LANGUAGE CXX)
target_link_libraries(swab_benchmark PRIVATE tiff tiff_port)
list(APPEND simple_tests swab_benchmark)

add_executable(predictor_threadpool_benchmark ../placeholder.h)
target_sources(predictor_threadpool_benchmark PRIVATE predictor_threadpool_benchmark.c)
set_target_properties(predictor_threadpool_benchmark PROPERTIES LINKER_LANGUAGE CXX)
target_link_libraries(predictor_threadpool_benchmark PRIVATE tiff tiff_port)
list(APPEND simple_tests predictor_threadpool_benchmark)

add_executable(predictor_threadpool_resize ../placeholder.h)
target_sources(predictor_threadpool_resize PRIVATE predictor_threadpool_resize.c)
set_target_properties(predictor_threadpool_resize PROPERTIES LINKER_LANGUAGE CXX)
target_link_libraries(predictor_threadpool_resize PRIVATE tiff tiff_port)
list(APPEND simple_tests predictor_threadpool_resize)

if(USE_IO_URING)
  add_executable(pack_uring_benchmark ../placeholder.h)
  target_sources(pack_uring_benchmark PRIVATE pack_uring_benchmark.c)
  set_target_properties(pack_uring_benchmark PROPERTIES LINKER_LANGUAGE CXX)
  target_link_libraries(pack_uring_benchmark PRIVATE tiff tiff_port)
  list(APPEND simple_tests pack_uring_benchmark)
endif()

add_executable(packbits_literal_run ../placeholder.h)
target_sources(packbits_literal_run PRIVATE packbits_literal_run.c)
set_target_properties(packbits_literal_run PROPERTIES LINKER_LANGUAGE CXX)
target_link_libraries(packbits_literal_run PRIVATE tiff tiff_port)
list(APPEND simple_tests packbits_literal_run)

add_executable(threadpool_stress ../placeholder.h)
target_sources(threadpool_stress PRIVATE threadpool_stress.c)
set_target_properties(threadpool_stress PROPERTIES LINKER_LANGUAGE CXX)
target_link_libraries(threadpool_stress PRIVATE tiff tiff_port)
list(APPEND simple_tests threadpool_stress)

if(USE_IO_URING)
  add_executable(uring_thread_stress ../placeholder.h)
  target_sources(uring_thread_stress PRIVATE uring_thread_stress.c)
  set_target_properties(uring_thread_stress PROPERTIES LINKER_LANGUAGE CXX)
  target_link_libraries(uring_thread_stress PRIVATE tiff tiff_port)
  list(APPEND simple_tests uring_thread_stress)
endif()

add_executable(concurrent_rw ../placeholder.h)
target_sources(concurrent_rw PRIVATE concurrent_rw.c)
set_target_properties(concurrent_rw PROPERTIES LINKER_LANGUAGE CXX)
target_link_libraries(concurrent_rw PRIVATE tiff tiff_port)
list(APPEND simple_tests concurrent_rw)

add_library(failalloc STATIC failalloc.c)

add_executable(threadpool_alloc_fail ../placeholder.h)
target_sources(threadpool_alloc_fail PRIVATE threadpool_alloc_fail.c)
target_link_libraries(threadpool_alloc_fail PRIVATE tiff tiff_port failalloc)
list(APPEND simple_tests threadpool_alloc_fail)

add_executable(threadpool_init_fail ../placeholder.h)
target_sources(threadpool_init_fail PRIVATE threadpool_init_fail.c)
target_link_libraries(threadpool_init_fail PRIVATE tiff tiff_port failalloc)
list(APPEND simple_tests threadpool_init_fail)

add_executable(grow_strips_alloc_fail ../placeholder.h)
target_sources(grow_strips_alloc_fail PRIVATE grow_strips_alloc_fail.c)
target_link_libraries(grow_strips_alloc_fail PRIVATE tiff tiff_port failalloc)
list(APPEND simple_tests grow_strips_alloc_fail)

add_executable(open_dng_alloc_fail ../placeholder.h)
target_sources(open_dng_alloc_fail PRIVATE open_dng_alloc_fail.c)
target_link_libraries(open_dng_alloc_fail PRIVATE tiff tiff_port failalloc)
list(APPEND simple_tests open_dng_alloc_fail)

add_executable(tiffstream_api ../placeholder.h)
target_sources(tiffstream_api PRIVATE tiffstream_api.cpp)
set_target_properties(tiffstream_api PROPERTIES LINKER_LANGUAGE CXX)
target_link_libraries(tiffstream_api PRIVATE tiffxx tiff tiff_port)
list(APPEND simple_tests tiffstream_api)

add_executable(tiff_fdopen_async ../placeholder.h)
target_sources(tiff_fdopen_async PRIVATE tiff_fdopen_async.cpp)
set_target_properties(tiff_fdopen_async PROPERTIES LINKER_LANGUAGE CXX)
target_link_libraries(tiff_fdopen_async PRIVATE tiffxx tiff tiff_port)
list(APPEND simple_tests tiff_fdopen_async)

if(WEBP_SUPPORT AND EMSCRIPTEN)
  # Emscripten is pretty finnicky about linker flags.
  # It needs --shared-memory if and only if atomics or bulk-memory is used.
  foreach(target
          ascii_tag
          long_tag
          short_tag
          strip_rw
          rewrite
          custom_dir
          defer_strile_loading
          defer_strile_writing
          )
    target_link_options(${target} PUBLIC "-Wl,--shared-memory")
  endforeach()
  if(JPEG_SUPPORT)
    target_link_options(raw_decode PUBLIC "-Wl,--shared-memory")
  endif()
endif()

set(TEST_OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/output")
file(MAKE_DIRECTORY "${TEST_OUTPUT}")

set(tiff_test_extra_args "-DLIBTIFF=$<TARGET_FILE:tiff>")

if(tiff-tools)
  list(APPEND tiff_test_extra_args "-DTIFFCP=$<TARGET_FILE:tiffcp>")
  list(APPEND tiff_test_extra_args "-DTIFFINFO=$<TARGET_FILE:tiffinfo>")
  list(APPEND tiff_test_extra_args "-DTIFFCMP=$<TARGET_FILE:tiffcmp>")
  list(APPEND tiff_test_extra_args "-DTIFFSPLIT=$<TARGET_FILE:tiffsplit>")
  list(APPEND tiff_test_extra_args "-DRGB2YCBCR=$<TARGET_FILE:rgb2ycbcr>")
  list(APPEND tiff_test_extra_args "-DRAW2TIFF=$<TARGET_FILE:raw2tiff>")
  if(tiff-contrib)
    list(APPEND tiff_test_extra_args "-DADDTIFFO=$<TARGET_FILE:addtiffo>")
  endif()
endif()

if(WIN32)
  list(APPEND tiff_test_extra_args "-DWIN32=${WIN32}")
endif()
if(CYGWIN)
  list(APPEND tiff_test_extra_args "-DCYGWIN=${CYGWIN}")
endif()

function(tiff_test_convert name command1 command2 command3 infile outfile validate)
  add_test(NAME "${name}"
           COMMAND "${CMAKE_COMMAND}"
           "-DCONVERT_COMMAND1=${command1}"
           "-DCONVERT_COMMAND2=${command2}"
           "-DCONVERT_COMMAND3=${command3}"
           "-DINFILE=${infile}"
           "-DOUTFILE=${outfile}"
           "-DVALIDATE=${validate}"
           ${tiff_test_extra_args}
           -P "${CMAKE_CURRENT_SOURCE_DIR}/TiffTest.cmake")
endfunction()

function(tiff_test_stdout name suffix command infile outfile)
  if (suffix)
     set(name "${name}-${suffix}")
  endif()
  add_test(NAME "${name}"
           COMMAND "${CMAKE_COMMAND}"
           "-DSTDOUT_COMMAND=${command}"
           "-DINFILE=${infile}"
           "-DOUTFILE=${outfile}"
           ${tiff_test_extra_args}
           -P "${CMAKE_CURRENT_SOURCE_DIR}/TiffTest.cmake")
endfunction()

function(tiff_test_stdout_noargs name command)
  add_test(NAME "${name}"
          COMMAND "${CMAKE_COMMAND}"
          "-DSTDOUT_COMMAND=$<TARGET_FILE:${command}>"
          ${tiff_test_extra_args}
          -P "${CMAKE_CURRENT_SOURCE_DIR}/TiffTest.cmake"
          WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")
  set_tests_properties("${name}" PROPERTIES ENVIRONMENT
          "srcdir=${CMAKE_CURRENT_SOURCE_DIR}")
endfunction()

function(tiff_test_reader name command infile)
  add_test(NAME "${name}"
           COMMAND "${CMAKE_COMMAND}"
           "-DREADER_COMMAND=${command}"
           "-DINFILE=${infile}"
           ${tiff_test_extra_args}
           -P "${CMAKE_CURRENT_SOURCE_DIR}/TiffTest.cmake")
endfunction()

function(add_convert_test_multi_out_in commandname1 commandname2 commandname3
      categoryname commandargs1 commandargs2 commandargs3
      image validate)
  string(REPLACE " " "^" escaped_commandargs1 "${commandargs1}")
  string(REPLACE " " "^" escaped_commandargs2 "${commandargs2}")
  string(REPLACE " " "^" escaped_commandargs3 "${commandargs3}")
  get_filename_component(name "${image}" NAME)
  get_filename_component(base "${image}" NAME_WE)
  set(testname "${commandname1}-${categoryname}-${base}")
  if(commandname1)
    set(command1
        "$<TARGET_FILE:${commandname1}>^${escaped_commandargs1}")
  else()
    set(command1)
  endif()
  if(commandname2)
    set(command2
        "$<TARGET_FILE:${commandname2}>^${escaped_commandargs2}")
  else()
    set(command2)
  endif()
  if(commandname3)
    set(command3
        "$<TARGET_FILE:${commandname3}>^${escaped_commandargs3}")
  else()
    set(command3)
  endif()
  set(infile "${TEST_OUTPUT}/${image}")
  set(outfile "${TEST_OUTPUT}/${commandname1}-${categoryname}-${base}.tiff")
  tiff_test_convert("${testname}" "${command1}" "${command2}" "${command3}"
                    "${infile}" "${outfile}" "${validate}")
endfunction()

function(add_convert_test_multi commandname1 commandname2 commandname3
      categoryname commandargs1 commandargs2 commandargs3
      image validate)
  string(REPLACE " " "^" escaped_commandargs1 "${commandargs1}")
  string(REPLACE " " "^" escaped_commandargs2 "${commandargs2}")
  string(REPLACE " " "^" escaped_commandargs3 "${commandargs3}")
  get_filename_component(name "${image}" NAME)
  get_filename_component(base "${image}" NAME_WE)
  set(testname "${commandname1}-${categoryname}-${base}")
  if(commandname1)
    set(command1
        "$<TARGET_FILE:${commandname1}>^${escaped_commandargs1}")
  else()
    set(command1)
  endif()
  if(commandname2)
    set(command2
        "$<TARGET_FILE:${commandname2}>^${escaped_commandargs2}")
  else()
    set(command2)
  endif()
  if(commandname3)
    set(command3
        "$<TARGET_FILE:${commandname3}>^${escaped_commandargs3}")
  else()
    set(command3)
  endif()
  set(infile "${CMAKE_CURRENT_SOURCE_DIR}/${image}")
  set(outfile "${TEST_OUTPUT}/${commandname1}-${categoryname}-${base}.tiff")
  tiff_test_convert("${testname}" "${command1}" "${command2}" "${command3}"
                    "${infile}" "${outfile}" "${validate}")
endfunction()

function(add_convert_test commandname
      categoryname commandargs
      image validate)
    add_convert_test_multi("${commandname}" "" ""
                           "${categoryname}" "${commandargs}" "" ""
                           "${image}" "${validate}")
endfunction()

function(add_convert_tests_multi commandname1 commandname2 commandname3
      categoryname
      commandargs1 commandargs2 commandargs3
      images validate)
  foreach(file ${${images}})
    add_convert_test_multi("${commandname1}" "${commandname2}"
                           "${commandname3}" "${categoryname}"
                           "${commandargs1}" "${commandargs2}"
                           "${commandargs3}" "${file}" "${validate}")
  endforeach()
endfunction()

function(add_convert_tests commandname categoryname commandargs images validate)
  foreach(file ${${images}})
    add_convert_test("${commandname}" "${categoryname}"
                     "${commandargs}" "${file}" "${validate}")
  endforeach()
endfunction()

function(add_stdout_test commandname suffix commandargs image)
  string(REPLACE " " "^" escaped_commandargs "${commandargs}")
  get_filename_component(name "${image}" NAME)
  get_filename_component(base "${image}" NAME_WE)
  set(testname "${commandname}-${base}")
  set(command "$<TARGET_FILE:${commandname}>^${escaped_commandargs}")
  set(infile "${CMAKE_CURRENT_SOURCE_DIR}/${image}")
  set(outfile "${TEST_OUTPUT}/${commandname}-${name}")
  string(REGEX REPLACE "\\.tiff\$" "" name "${name}")
  tiff_test_stdout("${testname}" "${suffix}" "${command}" "${infile}" "${outfile}")
endfunction()

function(add_reader_test commandname commandargs image)
  string(REPLACE " " "^" escaped_commandargs "${commandargs}")
  get_filename_component(name "${image}" NAME)
  get_filename_component(base "${image}" NAME_WE)
  set(testname "${commandname}-${base}")
  set(command "$<TARGET_FILE:${commandname}>^${escaped_commandargs}")
  set(infile "${CMAKE_CURRENT_SOURCE_DIR}/${image}")
  tiff_test_reader("${testname}" "${command}" "${infile}")
endfunction()

# Plain executables with no arguments
foreach(target IN LISTS simple_tests)
  tiff_test_stdout_noargs("${target}" "${target}")
endforeach()

# Disable flaky test until memory issues are fixed
set_tests_properties(test_open_options PROPERTIES DISABLED TRUE)

if(tiff-tools)
  # PPM
  add_convert_test(ppm2tiff miniswhite "" "images/miniswhite-1c-1b.pbm" TRUE)
  add_convert_test(ppm2tiff minisblack "" "images/minisblack-1c-8b.pgm" TRUE)
  add_convert_test(ppm2tiff rgb        "" "images/rgb-3c-16b.ppm"       TRUE)
  add_convert_test(ppm2tiff rgb        "" "images/rgb-3c-8b.ppm"        TRUE)

  # tiffcp
  # Reading / writing TIFF images with different encoding schemes.
  add_convert_tests(tiffcp TIFFIMG    ""                         TIFFIMAGES TRUE)

  # Convert some images to another coding scheme.
  add_convert_test(tiffcp   g3         "-c g3"         "images/miniswhite-1c-1b.tiff" FALSE)
  add_convert_test(tiffcp   g31d       "-c g3:1d"      "images/miniswhite-1c-1b.tiff" FALSE)
  add_convert_test(tiffcp   g31dfill   "-c g3:1d:fill" "images/miniswhite-1c-1b.tiff" FALSE)
  add_convert_test(tiffcp   g32d       "-c g3:2d"      "images/miniswhite-1c-1b.tiff" FALSE)
  add_convert_test(tiffcp   g32dfill   "-c g3:2d:fill" "images/miniswhite-1c-1b.tiff" FALSE)
  add_convert_test(tiffcp   g4         "-c g4"         "images/miniswhite-1c-1b.tiff" FALSE)
  add_convert_test_multi(tiffcp tiffcp "" logluv "-c none" "-c sgilog" ""
                         "images/logluv-3c-16b.tiff"    FALSE)
  add_convert_test_multi(tiffcp thumbnail "" thumbnail "-c g3:1d" "" ""
                         "images/miniswhite-1c-1b.tiff"    FALSE)
  add_convert_test(tiffcp   none       "-c none"       "images/quad-lzw-compat.tiff" FALSE)
  add_convert_test(tiffcp   noner1     "-c none -r 1"  "images/lzw-single-strip.tiff" FALSE)
  add_convert_test(tiffcp   float64_lzw_2_le "-c lzw:2"       "images/test_float64_predictor2_le_lzw.tif" FALSE)
  add_convert_test(tiffcp   float64_lzw_2_be "-c lzw:2"       "images/test_float64_predictor2_be_lzw.tif" FALSE)
  if(WEBP_SUPPORT)
    add_convert_test(tiffcp   webp_lossless_rgba_alpha_fully_opaque "-c none"    "images/webp_lossless_rgba_alpha_fully_opaque.tif" FALSE)
  endif()
  
  add_convert_test(tiff2rgba 32BPP    "-B 255 -n"     "images/32bpp-None.tiff" TRUE)
  if(JPEG_SUPPORT)
    add_convert_test(tiffcp 32BPP-JPEG "-c jpeg"      "images/32bpp-None-jpeg.tiff" TRUE)
  endif()

  if(JBIG_SUPPORT)
    add_convert_test(tiffcp   jbig         "-c jbig"         "images/lzw-single-strip.tiff" TRUE)
  endif()
  if(JPEGLS_SUPPORT)
    add_convert_test(tiffcp   jpegls       "-c jpegls"      "images/miniswhite-1c-1b.tiff" TRUE)
  endif()

  if(JPEG_SUPPORT)
    foreach(codec none lzw zip packbits)
      string(REPLACE ";" "_" codecname "${codec}")
      add_test(NAME "tiffcp-dng-${codecname}"
               COMMAND "${CMAKE_COMMAND}"
               "-DCODEC=${codec}"
               "-DINFILE=${CMAKE_CURRENT_SOURCE_DIR}/images/TEST_CINEPI_LIBTIFF_DNG.dng"
               "-DOUTFILE=${TEST_OUTPUT}/tiffcp-dng-${codecname}.tiff"
               ${tiff_test_extra_args}
               -P "${CMAKE_CURRENT_SOURCE_DIR}/TiffCpCodecTest.cmake")
    endforeach()
  endif()

  add_test(NAME "tiffcp-jpeg-invalid"
           COMMAND "${CMAKE_COMMAND}"
           "-DCODEC=none"
           "-DINFILE=${CMAKE_CURRENT_SOURCE_DIR}/images/TEST_JPEG.jpg"
           "-DOUTFILE=${TEST_OUTPUT}/tiffcp-jpeg-invalid.tiff"
           ${tiff_test_extra_args}
           -P "${CMAKE_CURRENT_SOURCE_DIR}/TiffCpCodecTest.cmake")
  set_tests_properties("tiffcp-jpeg-invalid" PROPERTIES WILL_FAIL TRUE)

  add_test(NAME "tiffcp-invalid-compression"
           COMMAND "${CMAKE_COMMAND}"
           "-DCODEC=bogus"
           "-DINFILE=${CMAKE_CURRENT_SOURCE_DIR}/images/TEST_CINEPI_LIBTIFF_DNG.dng"
           "-DOUTFILE=${TEST_OUTPUT}/tiffcp-invalid-compression.tiff"
           ${tiff_test_extra_args}
           -P "${CMAKE_CURRENT_SOURCE_DIR}/TiffCpCodecTest.cmake")
  set_tests_properties("tiffcp-invalid-compression" PROPERTIES WILL_FAIL TRUE)
  
  # Old-JPEG tests
  #--- tiffcp does not support subsampled images, as the OldJPEG ones are ---
  #    ToDo: a test for OldJPEG is missing.
  #add_convert_tests(test_tiffcp OldJPEG    ""                         TIFFIMAGES_OJPEG TRUE)

  # tiffdump
  add_reader_test(tiffdump "" "images/miniswhite-1c-1b.tiff")

  # tiffinfo
  add_reader_test(tiffinfo "-c -D -d -j -s" "images/custom_dir_EXIF_GPS.tiff")
  add_reader_test(tiffinfo "-c -D -d -j -s" "images/minisblack-1c-16b.tiff")
  add_reader_test(tiffinfo " " "images/tiff_with_subifd_chain.tif")

  # tiffcp split/join
  foreach(image ${UNCOMPRESSEDIMAGES})
    list(APPEND ESCAPED_UNCOMPRESSED "${CMAKE_CURRENT_SOURCE_DIR}/${image}")
  endforeach()
  string(REPLACE ";" "^" ESCAPED_UNCOMPRESSED "${ESCAPED_UNCOMPRESSED}")
  add_test(NAME "tiffcp-split"
           COMMAND "${CMAKE_COMMAND}"
           "-DTESTFILES=${ESCAPED_UNCOMPRESSED}"
           "-DCONJOINED=${TEST_OUTPUT}/tiffcp-split-conjoined.tiff"
           "-DSPLITFILE=${TEST_OUTPUT}/tiffcp-split-split-"
           ${tiff_test_extra_args}
           -P "${CMAKE_CURRENT_SOURCE_DIR}/TiffSplitTest.cmake")
  add_test(NAME "tiffcp-split-join"
           COMMAND "${CMAKE_COMMAND}"
           "-DTESTFILES=${ESCAPED_UNCOMPRESSED}"
           "-DCONJOINED=${TEST_OUTPUT}/tiffcp-split-join-conjoined.tiff"
           "-DSPLITFILE=${TEST_OUTPUT}/tiffcp-split-join-split-"
           "-DRECONJOINED=${TEST_OUTPUT}/tiffcp-split-join-reconjoined.tiff"
           ${tiff_test_extra_args}
           -P "${CMAKE_CURRENT_SOURCE_DIR}/TiffSplitTest.cmake")

  # PDF
  add_stdout_test(tiff2pdf "" "" "images/miniswhite-1c-1b.tiff" TRUE)

  # PS
  add_stdout_test(tiff2ps "ps-1" "-a -p -1" "images/miniswhite-1c-1b.tiff" TRUE)
  add_stdout_test(tiff2ps "ps-2" "-a -p -2" "images/miniswhite-1c-1b.tiff" TRUE)
  add_stdout_test(tiff2ps "ps-3" "-a -p -3" "images/miniswhite-1c-1b.tiff" TRUE)
  add_stdout_test(tiff2ps "eps-1" "-e -1" "images/miniswhite-1c-1b.tiff" TRUE)

  # BW
  add_convert_tests(tiff2bw default      ""                         RGBIMAGES TRUE)

  # RGBA
  add_convert_tests(tiff2rgba default    ""                         TIFFIMAGES TRUE)
  # RGBA (old-jpeg)
  add_convert_tests(tiff2rgba default    ""                         TIFFIMAGES_OJPEG TRUE)
  # Test rotations
  add_convert_tests(tiffcrop  R90        "-R90"                     TIFFIMAGES TRUE)
  # Test flip (mirror)
  add_convert_tests(tiffcrop  doubleflip "-F both"                  TIFFIMAGES TRUE)
  # Test extracting a section 60 pixels wide and 60 pixels high
  add_convert_tests(tiffcrop  extract    "-U px -E top -X 60 -Y 60" TIFFIMAGES TRUE)
  # Test extracting the first and fourth quarters from the left side.
  add_convert_tests(tiffcrop  extractz14 "-E left -Z1:4,2:4"        TIFFIMAGES TRUE)

  add_test(NAME "pipeline-full"
           COMMAND "${CMAKE_CURRENT_SOURCE_DIR}/pipeline_full_test.sh")
  set_tests_properties("pipeline-full" PROPERTIES ENVIRONMENT
          "srcdir=${CMAKE_CURRENT_SOURCE_DIR}")

  # addtiffo default overview levels
  add_test(NAME "addtiffo-default-small"
           COMMAND "${CMAKE_COMMAND}"
           "-DIMAGE=${CMAKE_CURRENT_SOURCE_DIR}/images/rgb-3c-8b.tiff"
           "-DEXPECT_GT1=FALSE"
           "-DOUTDIR=${TEST_OUTPUT}"
           ${tiff_test_extra_args}
           -P "${CMAKE_CURRENT_SOURCE_DIR}/AddtiffoTest.cmake")

  add_test(NAME "addtiffo-default-large"
           COMMAND "${CMAKE_COMMAND}"
           "-DIMAGE=${TEST_OUTPUT}/generated_large_8b.tiff"
           "-DGENERATOR_PY=${CMAKE_CURRENT_SOURCE_DIR}/gen_large_gray_tiff.py"
           "-DEXPECT_GT1=TRUE"
           "-DOUTDIR=${TEST_OUTPUT}"
           ${tiff_test_extra_args}
           -P "${CMAKE_CURRENT_SOURCE_DIR}/AddtiffoTest.cmake")

  add_test(NAME "addtiffo-subsampling-combinations"
           COMMAND "${CMAKE_COMMAND}"
           "-DOUTDIR=${TEST_OUTPUT}"
           ${tiff_test_extra_args}
           -P "${CMAKE_CURRENT_SOURCE_DIR}/AddtiffoSubsampledTest.cmake")
endif()
