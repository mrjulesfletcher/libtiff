#include "tiffio.hxx"
#include <cstdio>
#include <cstdlib>
#include <fstream>

int main()
{
    const char *srcdir = getenv("srcdir");
    if (!srcdir)
        srcdir = ".";
    char path[1024];
    snprintf(path, sizeof(path), "%s/images/rgb-3c-8b.tiff", srcdir);
    std::ifstream is(path, std::ios::binary);
    if (!is.is_open())
    {
        fprintf(stderr, "Cannot open %s\n", path);
        return 1;
    }
    TIFF *tif = TIFFStreamOpen("stream", &is);
    if (!tif)
    {
        fprintf(stderr, "TIFFStreamOpen failed\n");
        return 1;
    }
    uint32_t width = 0, length = 0;
    if (!TIFFGetField(tif, TIFFTAG_IMAGEWIDTH, &width) ||
        !TIFFGetField(tif, TIFFTAG_IMAGELENGTH, &length))
    {
        fprintf(stderr, "Missing tags\n");
        TIFFClose(tif);
        return 1;
    }
    TIFFClose(tif);
    return (width > 0 && length > 0) ? 0 : 1;
}
