#ifndef TIFF_TOOL_UTIL_H
#define TIFF_TOOL_UTIL_H

#include "tiffio.h"

/* Wrapper for tool error messages */
#define TIFF_TOOL_ERROR(module, fmt, ...)                                      \
    TIFFErrorExt(0, module, fmt, ##__VA_ARGS__)

#endif /* TIFF_TOOL_UTIL_H */
