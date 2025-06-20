#include "tiff_hvs.h"
#include "tiffiop.h"

#ifdef HAVE_LIBDRM
#include <fcntl.h>
#include <unistd.h>
#include <xf86drm.h>
#include <xf86drmMode.h>

int tiff_use_hvs = 0;

int TIFFUseHVS(void) { return tiff_use_hvs; }

void TIFFSetUseHVS(int enable) { tiff_use_hvs = enable; }

int TIFFReadRGBAImageHVS(TIFF *tif, uint32_t width, uint32_t height,
                         uint32_t *raster, int orientation, int stopOnError)
{
    if (!tiff_use_hvs)
        return TIFFReadRGBAImageOriented(tif, width, height, raster, orientation,
                                         stopOnError);

    int fd = open("/dev/dri/card0", O_RDWR);
    if (fd < 0)
        return TIFFReadRGBAImageOriented(tif, width, height, raster, orientation,
                                         stopOnError);

    drmModeRes *res = drmModeGetResources(fd);
    if (!res)
    {
        close(fd);
        return TIFFReadRGBAImageOriented(tif, width, height, raster,
                                         orientation, stopOnError);
    }

    /* TODO: Build DRM atomic request to route the buffer through the HVS */

    drmModeFreeResources(res);
    close(fd);

    /* Fallback to software implementation until hardware path is completed */
    return TIFFReadRGBAImageOriented(tif, width, height, raster, orientation,
                                     stopOnError);
}

#else /* HAVE_LIBDRM */

int tiff_use_hvs = 0;

int TIFFUseHVS(void) { return 0; }

void TIFFSetUseHVS(int enable) { (void)enable; }

int TIFFReadRGBAImageHVS(TIFF *tif, uint32_t width, uint32_t height,
                         uint32_t *raster, int orientation, int stopOnError)
{
    return TIFFReadRGBAImageOriented(tif, width, height, raster, orientation,
                                     stopOnError);
}

#endif /* HAVE_LIBDRM */
