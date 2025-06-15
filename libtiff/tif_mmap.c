#include "tiffiop.h"
#include "tiff_mmap.h"
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#ifdef HAVE_FCNTL_H
#include <fcntl.h>
#endif
#ifdef HAVE_MMAP
#include <sys/mman.h>
#endif

/* Default mapping size: 0 means map full file */
tmsize_t tiff_map_size = 0;
int tiff_posix_fadvise_flag =
#ifdef HAVE_POSIX_FADVISE
    POSIX_FADV_SEQUENTIAL;
#else
    0;
#endif
int tiff_madvise_flag =
#ifdef HAVE_MADVISE
    MADV_SEQUENTIAL;
#else
    0;
#endif

void TIFFSetMapSize(tmsize_t size) { tiff_map_size = size; }
void TIFFSetMapAdvice(int fadvise_flags, int madvise_flags)
{
    tiff_posix_fadvise_flag = fadvise_flags;
    tiff_madvise_flag = madvise_flags;
}
