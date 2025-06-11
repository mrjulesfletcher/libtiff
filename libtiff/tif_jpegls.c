#include "tiffiop.h"
#ifdef JPEGLS_SUPPORT

/* Placeholder for JPEG-LS codec using CharLS */
int TIFFInitJPEGLS(TIFF *tif, int scheme)
{
    static const char module[] = "TIFFInitJPEGLS";
    (void)scheme;
    TIFFErrorExtR(tif, module, "JPEG-LS compression not implemented");
    return 0;
}

#endif /* JPEGLS_SUPPORT */
