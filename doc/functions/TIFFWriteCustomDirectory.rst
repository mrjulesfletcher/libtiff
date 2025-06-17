TIFFWriteCustomDirectory
=======================

Synopsis
--------

.. highlight:: c

::

    #include <tiffio.h>

.. c:function:: int TIFFWriteCustomDirectory(TIFF* tif, uint64 *pdiroff)

Description
-----------

:c:func:`TIFFWriteCustomDirectory` writes the current custom directory to
the file and returns its offset in *pdiroff*.

See also
--------

:doc:`TIFFCustomDirectory` (3tiff),
:doc:`libtiff` (3tiff)
