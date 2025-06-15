#ifndef REVERSE_BITS_NEON_H
#define REVERSE_BITS_NEON_H

#include "tiffio.h"

#ifdef __cplusplus
extern "C"
{
#endif

    void TIFFReverseBitsNeon(uint8_t *cp, tmsize_t n);

#ifdef __cplusplus
}
#endif

#endif /* REVERSE_BITS_NEON_H */
