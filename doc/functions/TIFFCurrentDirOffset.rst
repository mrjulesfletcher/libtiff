TIFFCurrentDirOffset
====================

Synopsis
--------

.. highlight:: c

::

    #include <tiffio.h>

.. c:function:: uint64_t TIFFCurrentDirOffset(TIFF* tif)

Description
-----------

:c:func:`TIFFCurrentDirOffset` returns the file offset of the current
directory. This value can be passed to :c:func:`TIFFSetSubDirectory` to
access subdirectories linked through a ``SubIFD`` tag.

See also
--------

:doc:`TIFFquery` (3tiff),
:doc:`libtiff` (3tiff)
