# TODO Summary

This document lists the TODO comments found throughout the repository as of this commit. Items affecting correctness or error handling are marked with **[!]**.

## Build system
- **cmake/TiffConfig.cmake.in**: import dependencies (line 2).

## contrib/addtiffo
- **addtiffo.c**: un-hardwire array length for `anOverviews` (line 77) **[!]**
- **addtiffo.c**: encode resampling mode parameter as an integer (line 118).
- **addtiffo.c**: default overview levels based on source image size (line 140).
- **tif_overview.c**: update header notes (line 48).
- **tif_overview.c**: write `YCbCrPositioning` and `YCbCrCoefficients` tags (line 132).
- **tif_overview.c**: add parameter for JPEG compression quality (line 135).
- **tif_overview.c**: handle error when `padfSamples_size` is zero (line 304) **[!]**
- **tif_overview.c**: test with various subsampling and tile/strip sizes (line 403).
- **tif_overview.c**: rename `nBitsPerPixel` to `nBitsPerSample` (line 752).
- **tif_overview.c**: use consistent error reporting (`TIFFError` vs `fprintf`) (lines 767, 789) **[!]**
- **tif_overview.c**: consider `TIFFGetFieldDefaulted` for YCbCrSubsampling (line 782).
- **tif_ovrcache.c**: use consistent error reporting (`TIFFError` vs `fprintf`) (line 111) **[!]**
- **tif_ovrcache.c**: check return status of `TIFFFlush` (line 331) **[!]**
- **tif_ovrcache.c**: check return status of `TIFFSetSubDirectory` (line 333) **[!]**
- **tif_ovrcache.c**: extend `TIFF_Downsample` for iSample offsets (line 340).

## libtiff core
- **tif_getimage.c**: improve YCbCr support checks (lines 161, 469, 483).
- **tif_getimage.c**: rename obfuscated variables (line 2779).
- **tif_getimage.c**: add additional cases for YCbCr conversion (line 3391).
- **tif_jpeg.c**: review bytes-per-clumpline calculation (line 2439) **[!]**
- **tif_ojpeg.c**: assume one table per marker (line 1710) **[!]**
- **tif_ojpeg.c**: validate array bounds in SOF marker parsing (line 1885) **[!]**
- **tif_ojpeg.c**: clarify error handling when `subsamplingcorrect` is set (line 2207) **[!]**
- **tif_dir.c**: special handling for `DotRange` tag is an "evil" exception (lines 877, 1500).
- **tif_read.c**: confirm seeking works with partial buffers (line 445) **[!]**
- **tif_print.c**: same `DotRange` exception as in `tif_dir.c` (line 662).

## Documentation
- **TIFFClientInfo.rst**: verify explanation of clientinfo usage (line 20).
- **TIFFFieldQuery.rst**: check explanation and ability to handle duplicate tag definitions (lines 28, 42).
- **TIFFProcFunctions.rst**: explain procedure handling (line 30).
- **TIFFReadRGBAImage.rst**: specify return value when error occurs and stopOnError is non-zero (line 95) **[!]**
- **libtiff.rst**: clarify why `toff_t` is unsigned (line 127).

## Tests
- **test/custom_dir_EXIF_231.c**: complete GPS EXIF writing switch logic (line 638) **[!]**

Items marked **[!]** may impact correctness or error handling and should be prioritized.

