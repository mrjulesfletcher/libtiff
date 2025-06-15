#include "tif_config.h"
#ifdef USE_IO_URING
#include "tiffiop.h"
#include <errno.h>
#include <liburing.h>
#include <pthread.h>
#include <stdlib.h>
#include <sys/uio.h>

/*
 * io_uring based asynchronous I/O helpers.  The API became stable in
 * Linux 5.1, so callers should expect initialization to fail on older
 * kernels.
 *
 * Mapping between file descriptors and their io_uring.  Access to this list
 * and all io_uring operations is serialized by the global mutex below since
 * liburing itself is not thread-safe.  The default queue depth is 8 events
 * which is typically sufficient for the sequential access patterns used by
 * libtiff, but applications may request any depth supported by the kernel.
 */
typedef struct _TIFFURingEntry
{
    int fd;
    struct io_uring *ring;
    int async;
    struct _TIFFURingEntry *next;
} _TIFFURingEntry;

static _TIFFURingEntry *gUringList = NULL;
static pthread_mutex_t gUringMutex = PTHREAD_MUTEX_INITIALIZER;

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

/* Add a mapping entry. Returns 1 on success. */
static int _tiffUringAdd(TIFF *tif, int fd, struct io_uring *ring)
{
    pthread_mutex_lock(&gUringMutex);
    if (_tiffUringFind(fd))
    {
        pthread_mutex_unlock(&gUringMutex);
        TIFFErrorExtR(tif, "tif_uring", "fd %d already registered", fd);
        return 0;
    }

    _TIFFURingEntry *e = (_TIFFURingEntry *)_TIFFmallocExt(tif, sizeof(*e));
    if (!e)
    {
        pthread_mutex_unlock(&gUringMutex);
        TIFFErrorExtR(tif, "tif_uring", "Out of memory allocating ring entry");
        return 0;
    }

    e->fd = fd;
    e->ring = ring;
    e->async = 0;
    e->next = gUringList;
    gUringList = e;
    pthread_mutex_unlock(&gUringMutex);
    return 1;
}

static void _tiffUringRemove(int fd)
{
    pthread_mutex_lock(&gUringMutex);
    _TIFFURingEntry **pp = &gUringList;
    while (*pp)
    {
        if ((*pp)->fd == fd)
        {
            _TIFFURingEntry *tmp = *pp;
            *pp = tmp->next;
            _TIFFfreeExt(NULL, tmp);
            pthread_mutex_unlock(&gUringMutex);
            return;
        }
        pp = &(*pp)->next;
    }
    pthread_mutex_unlock(&gUringMutex);
}

int _tiffUringInit(TIFF *tif)
{
    tif->tif_uring =
        (struct io_uring *)_TIFFmallocExt(tif, sizeof(struct io_uring));
    if (!tif->tif_uring)
        return 0;
    unsigned int depth = 8;
    const char *env = getenv("TIFF_URING_DEPTH");
    if (tif->tif_uring_depth > 0)
    {
        depth = tif->tif_uring_depth;
    }
    else if (env)
    {
        char *endptr = NULL;
        errno = 0;
        unsigned long val = strtoul(env, &endptr, 10);
        if (errno != 0 || *endptr != '\0' || endptr == env || env[0] == '-' ||
            val == 0)
        {
            TIFFErrorExtR(tif, "tif_uring",
                          "Invalid TIFF_URING_DEPTH value '%s', using default",
                          env);
        }
        else
            depth = (unsigned int)val;
    }
    if (depth == 0)
        depth = 1;
    if (io_uring_queue_init(depth, tif->tif_uring, 0) < 0)
    {
        _TIFFfreeExt(tif, tif->tif_uring);
        tif->tif_uring = NULL;
        /* queue creation failed: nothing added to the mapping list */
        return 0;
    }
    tif->tif_uring_async = 0;
    if (!_tiffUringAdd(tif, tif->tif_fd, tif->tif_uring))
    {
        io_uring_queue_exit(tif->tif_uring);
        _TIFFfreeExt(tif, tif->tif_uring);
        tif->tif_uring = NULL;
        return 0;
    }
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
    _TIFFURingEntry *e;
    pthread_mutex_lock(&gUringMutex);
    e = _tiffUringFind(tif->tif_fd);
    if (e)
        e->async = enable ? 1 : 0;
    pthread_mutex_unlock(&gUringMutex);
    tif->tif_uring_async = enable ? 1 : 0;
}

void _tiffUringFlush(TIFF *tif)
{
    if (tif && tif->tif_uring)
        io_uring_submit(tif->tif_uring);
}

/*
 * Wait for completion events.  The global mutex is released while waiting so
 * that other threads may submit additional requests.
 */
void _tiffUringWait(TIFF *tif)
{
    if (!tif || !tif->tif_uring)
        return;
    struct io_uring *ring = tif->tif_uring;
    struct io_uring_cqe *cqe;
    while (io_uring_wait_cqe(ring, &cqe) == 0)
    {
        pthread_mutex_lock(&gUringMutex);
        io_uring_cqe_seen(ring, cqe);
        pthread_mutex_unlock(&gUringMutex);
    }
}

int TIFFSetURingQueueDepth(TIFF *tif, unsigned int depth)
{
    if (!tif)
        return 0;
    if (tif->tif_uring)
    {
        _tiffUringTeardown(tif);
        tif->tif_uring_depth = depth;
        return _tiffUringInit(tif);
    }
    tif->tif_uring_depth = depth;
    return 1;
}

unsigned int TIFFGetURingQueueDepth(TIFF *tif)
{
    if (!tif)
        return 0;
    return tif->tif_uring_depth;
}

static tmsize_t io_uring_rw(int readflag, thandle_t fd, struct iovec *iov,
                            unsigned int iovcnt, tmsize_t total_size)
{
    pthread_mutex_lock(&gUringMutex);
    _TIFFURingEntry *e = _tiffUringFind((int)(intptr_t)fd);
    if (!e)
    {
        pthread_mutex_unlock(&gUringMutex);
        return (tmsize_t)-1;
    }
    struct io_uring *ring = e->ring;

    struct io_uring_sqe *sqe = io_uring_get_sqe(ring);
    if (!sqe)
    {
        pthread_mutex_unlock(&gUringMutex);
        return (tmsize_t)-1;
    }

    if (readflag)
        io_uring_prep_readv(sqe, (int)(intptr_t)fd, iov, iovcnt, -1);
    else
        io_uring_prep_writev(sqe, (int)(intptr_t)fd, iov, iovcnt, -1);

    if (io_uring_submit(ring) < 0)
    {
        pthread_mutex_unlock(&gUringMutex);
        return (tmsize_t)-1;
    }

    if (e->async)
    {
        pthread_mutex_unlock(&gUringMutex);
        return total_size;
    }

    struct io_uring_cqe *cqe;
    if (io_uring_wait_cqe(ring, &cqe) < 0)
    {
        pthread_mutex_unlock(&gUringMutex);
        return (tmsize_t)-1;
    }
    int res = cqe->res;
    if (res < 0)
    {
        errno = -res;
        res = -1;
    }
    io_uring_cqe_seen(ring, cqe);
    pthread_mutex_unlock(&gUringMutex);
    return (tmsize_t)res;
}

tmsize_t _tiffUringReadProc(thandle_t fd, void *buf, tmsize_t size)
{
    struct iovec iov = {buf, (size_t)size};
    return io_uring_rw(1, fd, &iov, 1, size);
}

tmsize_t _tiffUringWriteProc(thandle_t fd, void *buf, tmsize_t size)
{
    struct iovec iov = {buf, (size_t)size};
    return io_uring_rw(0, fd, &iov, 1, size);
}

tmsize_t _tiffUringReadV(thandle_t fd, struct iovec *iov, unsigned int iovcnt,
                         tmsize_t size)
{
    return io_uring_rw(1, fd, iov, iovcnt, size);
}

tmsize_t _tiffUringWriteV(thandle_t fd, struct iovec *iov, unsigned int iovcnt,
                          tmsize_t size)
{
    return io_uring_rw(0, fd, iov, iovcnt, size);
}

#endif /* USE_IO_URING */
