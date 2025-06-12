# Thread pool support
option(threadpool "enable internal thread pool" OFF)
set(TIFF_THREADPOOL ${threadpool})
if(TIFF_THREADPOOL)
    find_package(Threads REQUIRED)
    list(APPEND tiff_libs_private_list "${CMAKE_THREAD_LIBS_INIT}")
    add_compile_definitions(TIFF_USE_THREADPOOL=1)
endif()
