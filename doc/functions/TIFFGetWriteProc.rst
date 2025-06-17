TIFFGetWriteProc
================

Synopsis
--------

.. highlight:: c

::

    #include <tiffio.h>

.. c:function:: TIFFReadWriteProc TIFFGetWriteProc(TIFF* tif)

Description
-----------

:c:func:`TIFFGetWriteProc` returns a pointer to the routine used by
``libtiff`` to write to the specified TIFF handle.

See also
--------

:doc:`TIFFProcFunctions` (3tiff),
:doc:`libtiff` (3tiff)
