#ifndef REVERSE_BITS_SSE_H
#define REVERSE_BITS_SSE_H

#include "tiffio.h"

#ifdef __cplusplus
extern "C" {
#endif

void TIFFReverseBitsSSE(uint8_t *cp, tmsize_t n);

#ifdef __cplusplus
}
#endif

#endif /* REVERSE_BITS_SSE_H */
