#include "tiffio.hxx"
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <memory>

int main()
{
    const char *srcdir_env = getenv("srcdir");
    const std::string srcdir = srcdir_env ? srcdir_env : ".";
    const std::string path = srcdir + "/images/rgb-3c-8b.tiff";

    std::ifstream is(path, std::ios::binary);
    if (!is.is_open())
    {
        fprintf(stderr, "Cannot open %s\n", path.c_str());
        return 1;
    }

    std::unique_ptr<TIFF, decltype(&TIFFClose)> tif(
        TIFFStreamOpen("stream", &is), &TIFFClose);
    if (!tif)
    {
        fprintf(stderr, "TIFFStreamOpen failed\n");
        return 1;
    }

    uint32_t width{};
    uint32_t length{};
    if (!TIFFGetField(tif.get(), TIFFTAG_IMAGEWIDTH, &width) ||
        !TIFFGetField(tif.get(), TIFFTAG_IMAGELENGTH, &length))
    {
        fprintf(stderr, "Missing tags\n");
        return 1;
    }
    return (width > 0 && length > 0) ? 0 : 1;
}
