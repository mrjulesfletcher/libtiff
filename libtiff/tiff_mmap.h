#ifndef TIFF_MMAP_H
#define TIFF_MMAP_H

#include "tiffio.h"

#ifdef __cplusplus
extern "C" {
#endif

extern tmsize_t tiff_map_size;
extern int tiff_posix_fadvise_flag;
extern int tiff_madvise_flag;

void TIFFSetMapSize(tmsize_t size);
void TIFFSetMapAdvice(int fadvise_flags, int madvise_flags);

#ifdef __cplusplus
}
#endif

#endif /* TIFF_MMAP_H */
