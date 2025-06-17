TIFFClientdata
==============

Synopsis
--------

.. highlight:: c

::

    #include <tiffio.h>

.. c:function:: thandle_t TIFFClientdata(TIFF* tif)

Description
-----------

:c:func:`TIFFClientdata` returns the client data handle associated with an
open TIFF file. This value represents the underlying file descriptor or
handle used by ``libtiff``.

See also
--------

:doc:`TIFFOpen` (3tiff),
:doc:`libtiff` (3tiff)
