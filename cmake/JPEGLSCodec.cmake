# Checks for JPEG-LS codec support

set(JPEGLS_SUPPORT FALSE)

find_package(CharLS QUIET)
if(NOT CharLS_FOUND)
  # Ubuntu packages ship a lowercase 'charlsConfig.cmake'
  find_package(charls CONFIG QUIET)
  if(charls_FOUND)
    set(CharLS_FOUND TRUE)
    set(CharLS_INCLUDE_DIRS ${charls_INCLUDE_DIRS})
    set(CharLS_LIBRARIES charls)
  endif()
endif()

option(jpegls "use CharLS (required for JPEG-LS compression)" ${CharLS_FOUND})
if (jpegls AND CharLS_FOUND)
    set(JPEGLS_SUPPORT TRUE)
endif()
