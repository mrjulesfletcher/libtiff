#include "tif_config.h"
#ifdef USE_IO_URING
#include <liburing.h>
#include <errno.h>
#include "tiffiop.h"

/* Mapping between file descriptors and their io_uring */
typedef struct _TIFFURingEntry
{
    int fd;
    struct io_uring *ring;
    int async;
    struct _TIFFURingEntry *next;
} _TIFFURingEntry;

static _TIFFURingEntry *gUringList = NULL;

static _TIFFURingEntry *_tiffUringFind(int fd)
{
    _TIFFURingEntry *cur;
    for (cur = gUringList; cur; cur = cur->next)
    {
        if (cur->fd == fd)
            return cur;
    }
    return NULL;
}

static void _tiffUringAdd(int fd, struct io_uring *ring)
{
    _TIFFURingEntry *e = (_TIFFURingEntry *)_TIFFmallocExt(NULL, sizeof(*e));
    if (!e)
        return;
    e->fd = fd;
    e->ring = ring;
    e->async = 0;
    e->next = gUringList;
    gUringList = e;
}

static void _tiffUringRemove(int fd)
{
    _TIFFURingEntry **pp = &gUringList;
    while (*pp)
    {
        if ((*pp)->fd == fd)
        {
            _TIFFURingEntry *tmp = *pp;
            *pp = tmp->next;
            _TIFFfreeExt(NULL, tmp);
            return;
        }
        pp = &(*pp)->next;
    }
}

int _tiffUringInit(TIFF *tif)
{
    tif->tif_uring = (struct io_uring *)_TIFFmallocExt(tif, sizeof(struct io_uring));
    if (!tif->tif_uring)
        return 0;
    if (io_uring_queue_init(8, tif->tif_uring, 0) < 0)
    {
        _TIFFfreeExt(tif, tif->tif_uring);
        tif->tif_uring = NULL;
        return 0;
    }
    tif->tif_uring_async = 0;
    _tiffUringAdd(tif->tif_fd, tif->tif_uring);
    return 1;
}

void _tiffUringTeardown(TIFF *tif)
{
    if (!tif->tif_uring)
        return;
    _tiffUringRemove(tif->tif_fd);
    _tiffUringWait(tif);
    io_uring_queue_exit(tif->tif_uring);
    _TIFFfreeExt(tif, tif->tif_uring);
    tif->tif_uring = NULL;
}

void _tiffUringSetAsync(TIFF *tif, int enable)
{
    _TIFFURingEntry *e = _tiffUringFind(tif->tif_fd);
    if (e)
        e->async = enable ? 1 : 0;
    tif->tif_uring_async = enable ? 1 : 0;
}

void _tiffUringFlush(TIFF *tif)
{
    if (tif && tif->tif_uring)
        io_uring_submit(tif->tif_uring);
}

void _tiffUringWait(TIFF *tif)
{
    if (!tif || !tif->tif_uring)
        return;
    struct io_uring_cqe *cqe;
    while (io_uring_wait_cqe(tif->tif_uring, &cqe) == 0)
    {
        io_uring_cqe_seen(tif->tif_uring, cqe);
    }
}

static tmsize_t io_uring_rw(int readflag, thandle_t fd, void *buf, tmsize_t size)
{
    _TIFFURingEntry *e = _tiffUringFind((int)(intptr_t)fd);
    if (!e)
        return (tmsize_t)-1;
    struct io_uring *ring = e->ring;

    struct io_uring_sqe *sqe = io_uring_get_sqe(ring);
    if (!sqe)
        return (tmsize_t)-1;

    if (readflag)
        io_uring_prep_read(sqe, (int)(intptr_t)fd, buf, (unsigned int)size, -1);
    else
        io_uring_prep_write(sqe, (int)(intptr_t)fd, buf, (unsigned int)size, -1);

    if (io_uring_submit(ring) < 0)
        return (tmsize_t)-1;

    if (e->async)
        return size;

    struct io_uring_cqe *cqe;
    if (io_uring_wait_cqe(ring, &cqe) < 0)
        return (tmsize_t)-1;
    int res = cqe->res;
    if (res < 0)
    {
        errno = -res;
        res = -1;
    }
    io_uring_cqe_seen(ring, cqe);
    return (tmsize_t)res;
}

tmsize_t _tiffUringReadProc(thandle_t fd, void *buf, tmsize_t size)
{
    return io_uring_rw(1, fd, buf, size);
}

tmsize_t _tiffUringWriteProc(thandle_t fd, void *buf, tmsize_t size)
{
    return io_uring_rw(0, fd, buf, size);
}

#endif /* USE_IO_URING */
