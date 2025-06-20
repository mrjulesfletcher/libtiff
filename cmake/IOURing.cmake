# io_uring optional support
option(io-uring "Use io_uring for async I/O" ON)
set(USE_IO_URING ${io-uring})

if(USE_IO_URING)
    find_path(IOURING_INCLUDE_DIR liburing.h)
    find_library(IOURING_LIBRARY uring)
    if(IOURING_INCLUDE_DIR AND IOURING_LIBRARY)
        list(APPEND TIFF_INCLUDES ${IOURING_INCLUDE_DIR})
        list(APPEND tiff_libs_private_list "${IOURING_LIBRARY}")
    else()
        message(WARNING "io_uring requested but liburing not found -- disabling")
        set(USE_IO_URING OFF CACHE BOOL "Use io_uring for async I/O" FORCE)
    endif()
endif()
