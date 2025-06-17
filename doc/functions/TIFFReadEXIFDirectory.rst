TIFFReadEXIFDirectory
====================

Synopsis
--------

.. highlight:: c

::

    #include <tiffio.h>

.. c:function:: int TIFFReadEXIFDirectory(TIFF* tif, toff_t diroff)

Description
-----------

:c:func:`TIFFReadEXIFDirectory` reads an EXIF directory from the given
offset.

See also
--------

:doc:`TIFFCustomDirectory` (3tiff),
:doc:`libtiff` (3tiff)
