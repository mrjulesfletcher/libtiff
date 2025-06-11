# Checks for JPEG-LS codec support

set(JPEGLS_SUPPORT FALSE)

find_package(CharLS)

option(jpegls "use CharLS (required for JPEG-LS compression)" ${CharLS_FOUND})
if (jpegls AND CharLS_FOUND)
    set(JPEGLS_SUPPORT TRUE)
endif()
