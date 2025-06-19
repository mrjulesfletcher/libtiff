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
    const char *srcdir = getenv("srcdir");
    if (!srcdir)
        srcdir = ".";
    char path[1024];
    snprintf(path, sizeof(path), "%s/images/rgb-3c-8b.tiff", srcdir);

    auto f1 = std::async(std::launch::async, check_via_fd, path);
    auto f2 = std::async(std::launch::async, check_via_fd, path);
    auto f3 = std::async(std::launch::async, check_via_fd, path);
    auto f4 = std::async(std::launch::async, check_via_fd, path);

    return f1.get() || f2.get() || f3.get() || f4.get();
}
