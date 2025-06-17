TIFFGetReadProc
===============

Synopsis
--------

.. highlight:: c

::

    #include <tiffio.h>

.. c:function:: TIFFReadWriteProc TIFFGetReadProc(TIFF* tif)

Description
-----------

:c:func:`TIFFGetReadProc` returns a pointer to the routine used by
``libtiff`` to read from the specified TIFF handle.

See also
--------

:doc:`TIFFProcFunctions` (3tiff),
:doc:`libtiff` (3tiff)
