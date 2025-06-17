TIFFIsByteSwapped
=================

Synopsis
--------

.. highlight:: c

::

    #include <tiffio.h>

.. c:function:: int TIFFIsByteSwapped(TIFF* tif)

Description
-----------

:c:func:`TIFFIsByteSwapped` returns a non-zero value if the image data in
the file uses a different byte order than the host machine.

See also
--------

:doc:`TIFFquery` (3tiff),
:doc:`libtiff` (3tiff)
