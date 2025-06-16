Asynchronous DNG Writing
========================

The thread pool allows libtiff to assemble and compress image strips in
parallel. Build with ``-Dthreadpool=ON`` (CMake) or
``--enable-multithreading`` (Autotools) to enable it. The pool size
defaults to the number of processors but can be overridden with the
``TIFF_THREAD_COUNT`` environment variable or by calling
``TIFFSetThreadCount()``.

When configured with ``-Dio-uring=ON`` or ``--enable-io-uring`` libtiff
uses Linux ``io_uring`` for asynchronous I/O. The submission queue depth
starts at eight. Tune it through the ``TIFF_URING_DEPTH`` environment
variable, ``TIFFOpenOptionsSetURingQueueDepth()`` before opening a file,
or at runtime with ``TIFFSetURingQueueDepth()``.

An application typically assembles each strip and queues the write while
the next strip is prepared::

    _tiffUringSetAsync(tif, 1);
    for (...) {
        TIFFAssembleStripSIMD(tif, src, w, rows, 1, 1, &sz, tmp, buf);
        _tiffUringWriteProc((thandle_t)TIFFFileno(tif), buf, (tmsize_t)sz);
    }
    _TIFFThreadPoolWait(pool);
    _tiffUringWait(tif);

Adjust ``TIFF_THREAD_COUNT`` and ``TIFFSetURingQueueDepth()`` for the
best throughput on your system.
