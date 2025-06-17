TIFFRewriteDirectory
====================

Synopsis
--------

.. highlight:: c

::

    #include <tiffio.h>

.. c:function:: int TIFFRewriteDirectory(TIFF* tif)

Description
-----------

:c:func:`TIFFRewriteDirectory` rewrites the current directory in place
without creating a new one.

See also
--------

:doc:`TIFFWriteDirectory` (3tiff),
:doc:`libtiff` (3tiff)
