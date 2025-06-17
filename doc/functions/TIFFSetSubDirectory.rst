TIFFSetSubDirectory
===================

Synopsis
--------

.. highlight:: c

::

    #include <tiffio.h>

.. c:function:: int TIFFSetSubDirectory(TIFF* tif, uint64_t diroff)

Description
-----------

:c:func:`TIFFSetSubDirectory` changes the current directory to the one at
the specified file offset.

See also
--------

:doc:`TIFFSetDirectory` (3tiff),
:doc:`libtiff` (3tiff)
