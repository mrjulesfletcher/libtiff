TIFFCurrentDirectory
====================

Synopsis
--------

.. highlight:: c

::

    #include <tiffio.h>

.. c:function:: tdir_t TIFFCurrentDirectory(TIFF* tif)

Description
-----------

:c:func:`TIFFCurrentDirectory` returns the index of the current
:ref:`Image File Directory (IFD) <ImageFileDirectory>` for the provided
TIFF handle.

See also
--------

:doc:`TIFFSetDirectory` (3tiff),
:doc:`TIFFquery` (3tiff),
:doc:`libtiff` (3tiff)
