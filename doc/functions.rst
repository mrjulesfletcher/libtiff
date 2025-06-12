TIFF Functions Overview
=======================

.. toctree::
    :maxdepth: 1
    :titlesonly:

    functions/libtiff
    functions/TIFFAccessTagMethods
    functions/TIFFbuffer
    functions/TIFFClientInfo
    functions/TIFFClose
    functions/TIFFcodec
    functions/TIFFcolor
    functions/TIFFCreateDirectory
    functions/TIFFCustomDirectory
    functions/TIFFCustomTagList
    functions/TIFFDataWidth
    functions/TIFFDeferStrileArrayWriting
    functions/TIFFError
    functions/TIFFFieldDataType
    functions/TIFFFieldName
    functions/TIFFFieldPassCount
    functions/TIFFFieldQuery
    functions/TIFFFieldReadCount
    functions/TIFFFieldTag
    functions/TIFFFieldWriteCount
    functions/TIFFFlush
    functions/TIFFGetField
    functions/TIFFmemory
    functions/TIFFMergeFieldInfo
    functions/TIFFOpen
    functions/TIFFOpenOptions
    functions/TIFFPrintDirectory
    functions/TIFFProcFunctions
    functions/TIFFquery
    functions/TIFFReadDirectory
    functions/TIFFReadEncodedStrip
    functions/TIFFReadEncodedTile
    functions/TIFFReadFromUserBuffer
    functions/TIFFReadRawStrip
    functions/TIFFReadRawTile
    functions/TIFFReadRGBAImage
    functions/TIFFReadRGBAStrip
    functions/TIFFReadRGBATile
    functions/TIFFReadScanline
    functions/TIFFReadTile
    functions/TIFFRGBAImage
    functions/TIFFSetDirectory
    functions/TIFFSetField
    functions/TIFFSetTagExtender
    functions/TIFFsize
    functions/TIFFStrileQuery
    functions/TIFFstrip
    functions/TIFFswab
    functions/TIFFtile
    functions/TIFFWarning
    functions/TIFFWriteDirectory
    functions/TIFFWriteEncodedStrip
    functions/TIFFWriteEncodedTile
    functions/TIFFWriteRawStrip
    functions/TIFFWriteRawTile
    functions/TIFFWriteScanline
    functions/TIFFWriteTile
    functions/_TIFFauxiliary
    functions/_TIFFRewriteField

Digital Negative (DNG) tags
--------------------------

``libtiff`` includes built-in definitions for the tags from the Adobe
Digital Negative (DNG) specification.  The definitions can be found in
``tif_dirinfo.c`` and cover versions 1.0 through 1.6 of the
specification, including tags such as ``DNGVersion``, ``DNGBackwardVersion``
and ``DNGPrivateData``.  Applications can therefore read and write DNG
files without having to register these tags.

If an application needs to store additional vendor-specific DNG tags it
may register them using :c:func:`TIFFMergeFieldInfo`.  Projects like
*CinePi* use this mechanism to attach custom colour pipeline metadata at
runtime.
