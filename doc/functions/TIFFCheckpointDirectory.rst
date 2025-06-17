TIFFCheckpointDirectory
=======================

Synopsis
--------

.. highlight:: c

::

    #include <tiffio.h>

.. c:function:: int TIFFCheckpointDirectory(TIFF* tif)

Description
-----------

The :c:func:`TIFFCheckpointDirectory` writes the current state of the
TIFF directory to the file so that what is already written becomes
readable. Unlike :c:func:`TIFFWriteDirectory`, it does not free the
directory information in memory so that it may be updated and written
again later.

See also
--------

:doc:`TIFFWriteDirectory` (3tiff),
:doc:`libtiff` (3tiff)
