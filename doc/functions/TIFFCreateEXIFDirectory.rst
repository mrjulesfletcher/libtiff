TIFFCreateEXIFDirectory
=======================

Synopsis
--------

.. highlight:: c

::

    #include <tiffio.h>

.. c:function:: int TIFFCreateEXIFDirectory(TIFF* tif)

Description
-----------

:c:func:`TIFFCreateEXIFDirectory` sets up the predefined EXIF custom
directory for the given TIFF handle so that subsequent calls to
:c:func:`TIFFSetField` populate that directory.

See also
--------

:doc:`TIFFCustomDirectory` (3tiff),
:doc:`libtiff` (3tiff)
