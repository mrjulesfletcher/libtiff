#include "tif_config.h"
#ifdef USE_IO_URING
#include <liburing.h>
#include <errno.h>
#include "tiffiop.h"

static tmsize_t io_uring_rw(int readflag, thandle_t fd, void *buf, tmsize_t size)
{
    struct io_uring ring;
    if (io_uring_queue_init(1, &ring, 0) < 0)
    {
        return (tmsize_t)-1;
    }

    struct io_uring_sqe *sqe = io_uring_get_sqe(&ring);
    if (!sqe)
    {
        io_uring_queue_exit(&ring);
        errno = EIO;
        return (tmsize_t)-1;
    }
    if (readflag)
        io_uring_prep_read(sqe, (int)(intptr_t)fd, buf, (unsigned int)size, -1);
    else
        io_uring_prep_write(sqe, (int)(intptr_t)fd, buf, (unsigned int)size, -1);

    if (io_uring_submit_and_wait(&ring, 1) < 0)
    {
        io_uring_queue_exit(&ring);
        return (tmsize_t)-1;
    }

    struct io_uring_cqe *cqe;
    if (io_uring_peek_cqe(&ring, &cqe) < 0)
    {
        io_uring_queue_exit(&ring);
        return (tmsize_t)-1;
    }
    int res = cqe->res;
    if (res < 0)
    {
        errno = -res;
        res = -1;
    }
    io_uring_cqe_seen(&ring, cqe);
    io_uring_queue_exit(&ring);
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
