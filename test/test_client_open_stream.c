#include "tif_config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#include "tiffio.h"

typedef struct
{
    unsigned char *data;
    size_t size;
    size_t offset;
#ifdef HAVE_MMAP
    int map_called;
    int unmap_called;
#endif
} MemoryStream;

static tmsize_t mem_read(thandle_t fd, void *buf, tmsize_t size)
{
    MemoryStream *ms = (MemoryStream *)fd;
    if (ms->offset + size > ms->size)
        size = ms->size - ms->offset;
    memcpy(buf, ms->data + ms->offset, size);
    ms->offset += size;
    return size;
}

static tmsize_t mem_write(thandle_t fd, void *buf, tmsize_t size)
{
    (void)fd;
    (void)buf;
    (void)size;
    return (tmsize_t)-1;
}

static toff_t mem_seek(thandle_t fd, toff_t off, int whence)
{
    MemoryStream *ms = (MemoryStream *)fd;
    size_t new_off;
    switch (whence)
    {
    case SEEK_SET:
        new_off = off;
        break;
    case SEEK_CUR:
        new_off = ms->offset + off;
        break;
    case SEEK_END:
        new_off = ms->size + off;
        break;
    default:
        return (toff_t)-1;
    }
    if (new_off > ms->size)
        return (toff_t)-1;
    ms->offset = new_off;
    return ms->offset;
}

static int mem_close(thandle_t fd)
{
    MemoryStream *ms = (MemoryStream *)fd;
    free(ms->data);
    free(ms);
    return 0;
}

static toff_t mem_size(thandle_t fd)
{
    MemoryStream *ms = (MemoryStream *)fd;
    return (toff_t)ms->size;
}

#ifdef HAVE_MMAP
static int mem_map(thandle_t fd, void **pbase, toff_t *psize)
{
    MemoryStream *ms = (MemoryStream *)fd;
    *pbase = ms->data;
    *psize = ms->size;
    ms->map_called++;
    return 1;
}

static void mem_unmap(thandle_t fd, void *base, toff_t size)
{
    MemoryStream *ms = (MemoryStream *)fd;
    (void)base;
    (void)size;
    ms->unmap_called++;
}
#endif

static MemoryStream *memory_stream_from_file(const char *filename)
{
    FILE *f = fopen(filename, "rb");
    if (!f)
        return NULL;
    if (fseek(f, 0, SEEK_END) != 0)
    {
        fclose(f);
        return NULL;
    }
    long size_long = ftell(f);
    if (size_long < 0)
    {
        fclose(f);
        return NULL;
    }
    if (fseek(f, 0, SEEK_SET) != 0)
    {
        fclose(f);
        return NULL;
    }
    MemoryStream *ms = (MemoryStream *)calloc(1, sizeof(MemoryStream));
    if (!ms)
    {
        fclose(f);
        return NULL;
    }
    ms->size = (size_t)size_long;
    ms->data = (unsigned char *)malloc(ms->size);
    if (!ms->data)
    {
        free(ms);
        fclose(f);
        return NULL;
    }
    if (fread(ms->data, 1, ms->size, f) != ms->size)
    {
        free(ms->data);
        free(ms);
        fclose(f);
        return NULL;
    }
    fclose(f);
    ms->offset = 0;
#ifdef HAVE_MMAP
    ms->map_called = 0;
    ms->unmap_called = 0;
#endif
    return ms;
}

int main(void)
{
    const char *srcdir = getenv("srcdir");
    if (!srcdir)
        srcdir = ".";
    char path[1024];

    /* Test opening JPEG sample: should fail */
    snprintf(path, sizeof(path), "%s/images/TEST_JPEG.jpg", srcdir);
    MemoryStream *jpeg_stream = memory_stream_from_file(path);
    if (!jpeg_stream)
    {
        fprintf(stderr, "Cannot read %s\n", path);
        return 1;
    }
    TIFF *tif = TIFFClientOpen("memjpeg", "r", (thandle_t)jpeg_stream, mem_read,
                               mem_write, mem_seek, mem_close, mem_size,
#ifdef HAVE_MMAP
                               mem_map, mem_unmap
#else
                               NULL, NULL
#endif
    );
    if (tif)
    {
        fprintf(stderr, "Unexpectedly opened JPEG as TIFF\n");
        TIFFClose(tif);
        return 1;
    }
    /* jpeg_stream is closed by libtiff when opening failed */

#ifdef HAVE_MMAP
    /* Test opening a valid TIFF and check mmap usage */
    snprintf(path, sizeof(path), "%s/images/miniswhite-1c-1b.tiff", srcdir);
    MemoryStream *tiff_stream = memory_stream_from_file(path);
    if (!tiff_stream)
    {
        fprintf(stderr, "Cannot read %s\n", path);
        return 1;
    }
    tif = TIFFClientOpen("memtiff", "r", (thandle_t)tiff_stream, mem_read,
                         mem_write, mem_seek, mem_close, mem_size, mem_map,
                         mem_unmap);
    if (!tif)
    {
        fprintf(stderr, "Cannot open TIFF via stream\n");
        return 1;
    }
    uint32_t w = 0, h = 0;
    if (!TIFFGetField(tif, TIFFTAG_IMAGEWIDTH, &w) ||
        !TIFFGetField(tif, TIFFTAG_IMAGELENGTH, &h))
    {
        fprintf(stderr, "Cannot read image fields\n");
        TIFFClose(tif);
        return 1;
    }
    TIFFClose(tif);
    if (tiff_stream->map_called == 0 || tiff_stream->unmap_called == 0)
    {
        fprintf(stderr, "Memory map callbacks not called\n");
        return 1;
    }
#endif

    return 0;
}

