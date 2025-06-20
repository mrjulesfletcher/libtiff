#include "tiffio.hxx"
#include <array>
#include <cstdio>
#include <cstdlib>
#include <fcntl.h>
#include <future>
#include <memory>
#include <string>
#include <unistd.h>

static int check_via_fd(const char *path)
{
    const int fd = open(path, O_RDONLY);
    if (fd < 0)
    {
        perror("open");
        return 1;
    }
    std::unique_ptr<TIFF, decltype(&TIFFClose)> tif(TIFFFdOpen(fd, path, "r"),
                                                    &TIFFClose);
    if (!tif)
    {
        close(fd);
        fprintf(stderr, "TIFFFdOpen failed\n");
        return 1;
    }
    uint32_t w{};
    uint32_t h{};
    const bool ok = TIFFGetField(tif.get(), TIFFTAG_IMAGEWIDTH, &w) &&
                    TIFFGetField(tif.get(), TIFFTAG_IMAGELENGTH, &h);
    return ok && w > 0 && h > 0 ? 0 : 1;
}

int main()
{
    const char *srcdir_env = getenv("srcdir");
    const std::string srcdir = srcdir_env ? srcdir_env : ".";
    const std::string path = srcdir + "/images/rgb-3c-8b.tiff";

    std::array<std::future<int>, 4> futures{
        std::async(std::launch::async, check_via_fd, path.c_str()),
        std::async(std::launch::async, check_via_fd, path.c_str()),
        std::async(std::launch::async, check_via_fd, path.c_str()),
        std::async(std::launch::async, check_via_fd, path.c_str())};

    int ret = 0;
    for (auto &future_res : futures)
    {
        ret |= future_res.get();
    }
    return ret;
}
