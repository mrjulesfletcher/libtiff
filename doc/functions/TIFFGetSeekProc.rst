TIFFGetSeekProc
===============

Synopsis
--------

.. highlight:: c

::

    #include <tiffio.h>

.. c:function:: TIFFSeekProc TIFFGetSeekProc(TIFF* tif)

Description
-----------

:c:func:`TIFFGetSeekProc` returns a pointer to the routine used by
``libtiff`` to seek within the specified TIFF handle.

See also
--------

:doc:`TIFFProcFunctions` (3tiff),
:doc:`libtiff` (3tiff)
