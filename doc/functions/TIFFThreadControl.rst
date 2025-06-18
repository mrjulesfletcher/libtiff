TIFFThreadControl
=================

Synopsis
--------

.. highlight:: c

::

    #include <tiffio.h>

.. c:function:: int TIFFSetThreadPoolSize(int size)

.. c:function:: int TIFFSetUseNEON(int flag)

.. c:function:: int TIFFSetUseSSE41(int flag)

.. c:function:: int TIFFUseNEON(void)

.. c:function:: int TIFFUseSSE41(void)

Description
-----------

These functions control optional CPU optimizations in ``libtiff``.

:c:func:`TIFFSetThreadPoolSize` configures the number of worker threads used by
the internal thread pool. A value of zero disables multithreading.

:c:func:`TIFFSetUseNEON` and :c:func:`TIFFSetUseSSE41` enable or disable usage of
SIMD routines optimized for ARM NEON and x86 SSE4.1 instructions,
respectively, when compiled with such support.

:c:func:`TIFFUseNEON` and :c:func:`TIFFUseSSE41` report whether NEON or SSE4.1
optimizations are currently enabled.
