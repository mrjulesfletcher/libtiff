ZIP AES Whitening
=================

When compressing with the Deflate codec, libtiff normally forwards image
bytes directly to zlib or libdeflate. On modern AArch64 processors,
hardware AES instructions can be used for a fast statistical whitening
step prior to compression. Applying one or two rounds of the `AESE`
and `AESMC` instructions decorrelates neighbouring bytes, increasing
entropy and improving the effectiveness of subsequent Huffman or LZ77
coding.

libtiff enables this transform automatically when hardware AES support is
detected. Applications can query or override the behaviour with
``TIFFUseAES()`` and ``TIFFSetUseAES()`` from :file:`tiffio.h`.

This whitening is not encryption. The transform is reversible but uses
no secret key, so the resulting Deflate stream remains fully compatible
with existing decoders. The technique simply breaks up spatial patterns
in the input data so that the compressor can encode it more efficiently.

In addition to the ZIP codec paths, the Bayer raw helpers
``TIFFPackRaw10``/``12``/``14``/``16`` and their matching unpack
functions also whiten the packed buffers when AES support is available.
Raw pipelines therefore benefit from the transform before the data is
compressed with Deflate.
