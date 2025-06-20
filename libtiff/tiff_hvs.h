#ifndef TIFF_HVS_H
#define TIFF_HVS_H

#include "tif_config.h"
#include <stdint.h>

typedef struct tiff TIFF;

#ifdef __cplusplus
extern "C" {
#endif

#ifdef HAVE_LIBDRM
extern int tiff_use_hvs;
int TIFFUseHVS(void);
void TIFFSetUseHVS(int enable);
int TIFFReadRGBAImageHVS(TIFF *tif, uint32_t width, uint32_t height,
                         uint32_t *raster, int orientation, int stopOnError);
#else
extern int tiff_use_hvs;
int TIFFUseHVS(void);
void TIFFSetUseHVS(int enable);
int TIFFReadRGBAImageHVS(TIFF *tif, uint32_t width, uint32_t height,
                         uint32_t *raster, int orientation, int stopOnError);
#endif

#ifdef __cplusplus
}
#endif

#endif /* TIFF_HVS_H */
