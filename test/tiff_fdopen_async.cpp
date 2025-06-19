#include "tiffio.hxx"
#include <cstdio>
#include <cstdlib>
#include <fcntl.h>
#include <future>
#include <unistd.h>

static int check_via_fd(const char *path)
{
    int fd = open(path, O_RDONLY);
    if (fd < 0)
    {
        perror("open");
        return 1;
    }
    TIFF *tif = TIFFFdOpen(fd, path, "r");
    if (!tif)
    {
        close(fd);
        fprintf(stderr, "TIFFFdOpen failed\n");
        return 1;
    }
    uint32_t w = 0, h = 0;
    int ok = TIFFGetField(tif, TIFFTAG_IMAGEWIDTH, &w) &&
             TIFFGetField(tif, TIFFTAG_IMAGELENGTH, &h);
    TIFFClose(tif);
    return ok && w > 0 && h > 0 ? 0 : 1;
}

int main()
{
    const char *srcdir_env = getenv("srcdir");
    std::string srcdir = srcdir_env ? srcdir_env : ".";
    std::string path = srcdir + "/images/rgb-3c-8b.tiff";

    auto f1 = std::async(std::launch::async, check_via_fd, path.c_str());
    auto f2 = std::async(std::launch::async, check_via_fd, path.c_str());
    auto f3 = std::async(std::launch::async, check_via_fd, path.c_str());
    auto f4 = std::async(std::launch::async, check_via_fd, path.c_str());

    return f1.get() || f2.get() || f3.get() || f4.get();
}
