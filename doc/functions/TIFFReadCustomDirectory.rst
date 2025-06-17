TIFFReadCustomDirectory
======================

Synopsis
--------

.. highlight:: c

::

    #include <tiffio.h>

.. c:function:: int TIFFReadCustomDirectory(TIFF* tif, toff_t diroff, const TIFFFieldArray* infoarray)

Description
-----------

:c:func:`TIFFReadCustomDirectory` reads a custom directory from an
arbitrary file offset and sets the context of the TIFF handle
accordingly.

See also
--------

:doc:`TIFFCustomDirectory` (3tiff),
:doc:`libtiff` (3tiff)
