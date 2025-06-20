// tiff stream interface class definition

#ifndef _TIFF_STREAM_H_
#define _TIFF_STREAM_H_

#include <cstddef>
#include <cstdint>
#include <iostream>

#include "tiffio.h"

class TiffStream
{

  public:
    // ctor/dtor
    TiffStream();
    ~TiffStream();

  public:
    enum SeekDir
    {
        beg,
        cur,
        end,
    };

  public:
    // factory methods
    TIFF *makeFileStream(std::iostream *str);
    TIFF *makeFileStream(std::istream *str);
    TIFF *makeFileStream(std::ostream *str);

  public:
    // tiff client methods
    static tsize_t read(thandle_t fd, tdata_t buf, tsize_t size);
    static tsize_t write(thandle_t fd, tdata_t buf, tsize_t size);
    static toff_t seek(thandle_t fd, toff_t offset, int origin);
    static toff_t size(thandle_t fd);
    static int close(thandle_t fd);
    static int map(thandle_t fd, tdata_t *phase, toff_t *psize);
    static void unmap(thandle_t fd, tdata_t base, toff_t size);

  public:
    // query method
    TIFF *getTiffHandle() const { return m_tif; }
    std::size_t getStreamLength() const { return m_streamLength; }

  private:
    // internal methods
    std::uint64_t getSize(thandle_t fd);
    std::uint64_t tell(thandle_t fd);
    bool seekInt(thandle_t fd, std::uint64_t offset, int origin);
    bool isOpen(thandle_t fd) const;

  private:
    thandle_t m_this;
    TIFF *m_tif;
    static const char *m_name;
    std::istream *m_inStream;
    std::ostream *m_outStream;
    std::iostream *m_ioStream;
    std::size_t m_streamLength;
};

#endif // _TIFF_STREAM_H_
