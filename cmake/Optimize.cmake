# Optimization options: PGO, LTO and BOLT
#
# Copyright © 2024 OpenAI
#
# Permission to use, copy, modify, distribute, and sell this software and
# its documentation for any purpose is hereby granted without fee, provided
# that the above copyright notices and this permission notice appear in all
# copies of the software and related documentation.
#
# THE SOFTWARE IS PROVIDED "AS-IS" AND WITHOUT WARRANTY OF ANY KIND,
# EXPRESS, IMPLIED OR OTHERWISE, INCLUDING WITHOUT LIMITATION, ANY
# WARRANTY OF MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE.
#
# IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY SPECIAL, INCIDENTAL,
# INDIRECT OR CONSEQUENTIAL DAMAGES OF ANY KIND, OR ANY DAMAGES WHATSOEVER
# RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER OR NOT ADVISED OF THE
# POSSIBILITY OF DAMAGE, AND ON ANY THEORY OF LIABILITY, ARISING OUT OF OR IN
# CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.

option(enable-pgo "Enable profile guided optimisation" OFF)
option(enable-lto "Enable link time optimisation" OFF)
option(enable-bolt "Enable LLVM BOLT optimisation" OFF)

# When enable-pgo is active, two modes are possible:
# 1) Instrumentation build to produce profiling data
# 2) Optimised build using existing profile data
# LIBTIFF_PGO_PROFILE should point to a .profdata file for mode 2.

if(enable-pgo)
    if(NOT LIBTIFF_PGO_PROFILE)
        message(STATUS "Configuring PGO instrumentation build")
        set(PGO_FLAGS "-fprofile-generate")
    else()
        message(STATUS "Configuring PGO use build with profile ${LIBTIFF_PGO_PROFILE}")
        set(PGO_FLAGS "-fprofile-use=${LIBTIFF_PGO_PROFILE}" "-fprofile-correction")
    endif()
    foreach(lang C CXX)
        set(CMAKE_${lang}_FLAGS "${CMAKE_${lang}_FLAGS} ${PGO_FLAGS}")
    endforeach()
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${PGO_FLAGS}")
    set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} ${PGO_FLAGS}")
endif()

if(enable-lto)
    include(CheckIPOSupported)
    check_ipo_supported(RESULT LTO_SUPPORTED OUTPUT LTO_ERROR)
    if(LTO_SUPPORTED)
        set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)
    else()
        message(WARNING "LTO not supported: ${LTO_ERROR}")
    endif()
endif()

if(enable-bolt)
    find_program(LLVM_BOLT_EXECUTABLE NAMES llvm-bolt bolt)
    find_program(PERF2BOLT_EXECUTABLE NAMES perf2bolt)
    find_program(PERF_EXECUTABLE NAMES perf)
    set(BOLT_TARGET "${BOLT_TARGET}")
    if(NOT BOLT_TARGET)
        set(BOLT_TARGET "${CMAKE_BINARY_DIR}/libtiff/libtiff.so")
    endif()
    if(LLVM_BOLT_EXECUTABLE AND PERF2BOLT_EXECUTABLE AND PERF_EXECUTABLE)
        add_custom_target(bolt
            COMMAND ${PERF_EXECUTABLE} record -o ${CMAKE_BINARY_DIR}/perf.data -- ${BOLT_TARGET}
            COMMAND ${PERF2BOLT_EXECUTABLE} ${BOLT_TARGET} -p ${CMAKE_BINARY_DIR}/perf.data -o ${CMAKE_BINARY_DIR}/perf.fdata
            COMMAND ${LLVM_BOLT_EXECUTABLE} ${BOLT_TARGET} -o ${BOLT_TARGET}.bolt -data=${CMAKE_BINARY_DIR}/perf.fdata
                --reorder-blocks=ext-tsp --reorder-functions=hfsort --split-functions --split-all-cold
            COMMENT "Running perf and applying BOLT optimisation" VERBATIM)
    else()
        message(WARNING "BOLT requested but required tools not found")
    endif()
endif()
