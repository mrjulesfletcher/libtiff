TIFFIsBigEndian
===============

Synopsis
--------

.. highlight:: c

::

    #include <tiffio.h>

.. c:function:: int TIFFIsBigEndian(TIFF* tif)

Description
-----------

:c:func:`TIFFIsBigEndian` returns a non-zero value if the TIFF file is
stored in big-endian byte order and zero if it is little-endian.

See also
--------

:doc:`TIFFquery` (3tiff),
:doc:`libtiff` (3tiff)
