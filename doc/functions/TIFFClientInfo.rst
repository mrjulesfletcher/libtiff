TIFFClientInfo
==============

Synopsis
--------

.. highlight:: c

::

    #include <tiffio.h>

.. c:function:: void *TIFFGetClientInfo(TIFF* tif, const char *name)

.. c:function:: void TIFFSetClientInfo(TIFF* tif, void *data, const char *name)

Description
-----------

The *clientinfo* linked list provides a mechanism to associate arbitrary
application data with a :c:type:`TIFF` handle.  Each entry is identified by a
name string and stores a user supplied pointer.  The library maintains the
linked list internally but does not manage the lifetime of the stored data.
Entries persist for the life of the :c:type:`TIFF` object and are removed when
the file is closed; however, the caller is responsible for releasing any memory
referenced by the pointers.

:c:func:`TIFFGetClientInfo` returns the pointer associated with a named entry in
the list or ``NULL`` if *name* is not present.

:c:func:`TIFFSetClientInfo` adds a new entry or replaces an existing one with
the specified *name* and *data* pointer.

Diagnostics
-----------

None.

See also
--------

:doc:`libtiff` (3tiff),
