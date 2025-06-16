TIFFProcFunctions
=================

Synopsis
--------

.. highlight:: c

::

    #include <tiffio.h>

.. c:function:: TIFFCloseProc TIFFGetCloseProc(TIFF* tif)

.. c:function:: TIFFMapFileProc TIFFGetMapFileProc(TIFF* tif)

.. c:function:: TIFFReadWriteProc TIFFGetReadProc(TIFF* tif)

.. c:function:: TIFFSeekProc TIFFGetSeekProc(TIFF* tif)

.. c:function:: TIFFSizeProc TIFFGetSizeProc(TIFF* tif)

.. c:function:: TIFFUnmapFileProc TIFFGetUnmapFileProc(TIFF* tif)

.. c:function:: TIFFReadWriteProc TIFFGetWriteProc(TIFF* tif)

Description
-----------

The internal :c:type:`TIFF` structure stores procedure pointers for I/O. These members (``tif_readproc``, ``tif_writeproc``, etc.) are defined in :file:`libtiff/tiffiop.h` and are assigned by :c:func:`TIFFClientOpen` or :c:func:`TIFFOpen` in :file:`libtiff/tif_open.c`. The library invokes them through macros such as :c:macro:`TIFFReadFile` and :c:macro:`TIFFWriteFile`.
The following routines return the current callbacks associated with an open TIFF file.

:c:func:`TIFFGetCloseProc` returns a pointer to file close method.

:c:func:`TIFFGetMapFileProc` returns a pointer to memory mapping method.

:c:func:`TIFFGetReadProc` returns a pointer to file read method.

:c:func:`TIFFGetSeekProc` returns a pointer to file seek method.

:c:func:`TIFFGetSizeProc` returns a pointer to file size requesting method.

:c:func:`TIFFGetUnmapFileProc` returns a pointer to memory unmapping method.

:c:func:`TIFFGetWriteProc` returns a pointer to file write method.

Diagnostics
-----------

None.

See also
--------

:doc:`libtiff` (3tiff),
:doc:`TIFFOpen` (3tiff)
