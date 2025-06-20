// tiff stream interface class implementation

#include "tiffstream.h"
#include <algorithm>
#include <cstdint>
#include <limits>

const char *TiffStream::m_name = "TiffStream";

TiffStream::TiffStream()
{
    m_tif = nullptr;

    m_inStream = nullptr;
    m_outStream = nullptr;
    m_ioStream = nullptr;

    m_streamLength = 0;

    m_this = reinterpret_cast<thandle_t>(this);
}

TiffStream::~TiffStream()
{
    if (m_tif != nullptr)
    {
        TIFFClose(m_tif);
        m_tif = nullptr;
    }
}

TIFF *TiffStream::makeFileStream(std::istream *str)
{
    m_inStream = str;
    m_outStream = nullptr;
    m_ioStream = nullptr;
    m_streamLength = getSize(m_this);

    m_tif = TIFFClientOpen(m_name, "r", m_this, read, write, seek, close, size,
                           map, unmap);
    return m_tif;
}

TIFF *TiffStream::makeFileStream(std::ostream *str)
{
    m_inStream = nullptr;
    m_outStream = str;
    m_ioStream = nullptr;
    m_streamLength = getSize(m_this);

    m_tif = TIFFClientOpen(m_name, "w", m_this, read, write, seek, close, size,
                           map, unmap);
    return m_tif;
}

TIFF *TiffStream::makeFileStream(std::iostream *str)
{
    m_inStream = nullptr;
    m_outStream = nullptr;
    m_ioStream = str;
    m_streamLength = getSize(m_this);

    m_tif = TIFFClientOpen(m_name, "r+w", m_this, read, write, seek, close,
                           size, map, unmap);
    return m_tif;
}

tsize_t TiffStream::read(thandle_t fd, tdata_t buf, tsize_t size)
{
    TiffStream *ts = reinterpret_cast<TiffStream *>(fd);
    std::istream *istr = nullptr;
    if (ts->m_inStream != nullptr)
    {
        istr = ts->m_inStream;
    }
    else if (ts->m_ioStream != nullptr)
    {
        istr = ts->m_ioStream;
    }

    if (istr == nullptr)
        return 0;

    std::uint64_t current = ts->tell(fd);
    if (current >= ts->m_streamLength)
        return 0;

    std::uint64_t remain = ts->m_streamLength - current;
    std::size_t to_read = std::min<std::size_t>(remain, size);
    std::streamsize actual = static_cast<std::streamsize>(to_read);
    if (static_cast<std::size_t>(actual) != to_read)
        return 0;

    istr->read(reinterpret_cast<char *>(buf), actual);
    return static_cast<tsize_t>(istr->gcount());
}

tsize_t TiffStream::write(thandle_t fd, tdata_t buf, tsize_t size)
{
    TiffStream *ts = reinterpret_cast<TiffStream *>(fd);
    std::ostream *ostr = nullptr;
    if (ts->m_outStream != nullptr)
    {
        ostr = ts->m_outStream;
    }
    else if (ts->m_ioStream != nullptr)
    {
        ostr = ts->m_ioStream;
    }

    if (ostr == nullptr)
        return 0;

    std::streampos start = ostr->tellp();
    std::streamsize actual = static_cast<std::streamsize>(size);
    if (static_cast<tsize_t>(actual) != size)
        return 0;
    ostr->write(reinterpret_cast<const char *>(buf), actual);
    std::streampos end = ostr->tellp();
    if (end == std::streampos(-1))
        return 0;
    return static_cast<tsize_t>(end - start);
}

toff_t TiffStream::seek(thandle_t fd, toff_t offset, int origin)
{
    TiffStream *ts = reinterpret_cast<TiffStream *>(fd);
    if (ts->seekInt(fd, static_cast<std::uint64_t>(offset), origin))
        return offset;
    else
        return -1;
}

int TiffStream::close(thandle_t fd)
{
    TiffStream *ts = reinterpret_cast<TiffStream *>(fd);
    if (ts->m_inStream != nullptr)
    {
        ts->m_inStream = nullptr;
        ts->m_tif = nullptr;
        return 0;
    }
    else if (ts->m_outStream != nullptr)
    {
        ts->m_outStream = nullptr;
        ts->m_tif = nullptr;
        return 0;
    }
    else if (ts->m_ioStream != nullptr)
    {
        ts->m_ioStream = nullptr;
        ts->m_tif = nullptr;
        return 0;
    }
    return -1;
}

toff_t TiffStream::size(thandle_t fd)
{
    TiffStream *ts = reinterpret_cast<TiffStream *>(fd);
    return ts->getSize(fd);
}

int TiffStream::map(thandle_t /*fd*/, tdata_t * /*phase*/, toff_t * /*psize*/)
{
    return 0;
}

void TiffStream::unmap(thandle_t /*fd*/, tdata_t /*base*/, toff_t /*size*/) {}

std::uint64_t TiffStream::getSize(thandle_t fd)
{
    if (!isOpen(fd))
        return 0;

    std::uint64_t pos = tell(fd);
    seekInt(fd, 0, end);
    std::uint64_t streamSize = tell(fd);
    seekInt(fd, pos, beg);

    return streamSize;
}

std::uint64_t TiffStream::tell(thandle_t fd)
{
    TiffStream *ts = reinterpret_cast<TiffStream *>(fd);
    std::streampos pos = std::streampos(-1);
    if (ts->m_inStream != nullptr)
    {
        pos = ts->m_inStream->tellg();
    }
    else if (ts->m_outStream != nullptr)
    {
        pos = ts->m_outStream->tellp();
    }
    else if (ts->m_ioStream != nullptr)
    {
        pos = ts->m_ioStream->tellg();
    }
    if (pos == std::streampos(-1))
        return 0;
    return static_cast<std::uint64_t>(pos);
}

bool TiffStream::seekInt(thandle_t fd, std::uint64_t offset, int origin)
{
    if (!isOpen(fd))
        return false;

    if (offset >
        static_cast<std::uint64_t>(std::numeric_limits<std::streamoff>::max()))
        return false;

    std::ios::seekdir org;
    switch (origin)
    {
        case beg:
            org = std::ios::beg;
            break;
        case cur:
            org = std::ios::cur;
            break;
        case end:
            org = std::ios::end;
            break;
        default:
            return false;
    }

    TiffStream *ts = reinterpret_cast<TiffStream *>(fd);
    if (ts->m_inStream != nullptr)
    {
        ts->m_inStream->seekg(offset, org);
        return !ts->m_inStream->fail();
    }
    else if (ts->m_outStream != nullptr)
    {
        ts->m_outStream->seekp(offset, org);
        return !ts->m_outStream->fail();
    }
    else if (ts->m_ioStream != nullptr)
    {
        ts->m_ioStream->seekg(offset, org);
        ts->m_ioStream->seekp(offset, org);
        return !ts->m_ioStream->fail();
    }
    return false;
}

bool TiffStream::isOpen(thandle_t fd) const
{
    const TiffStream *ts = reinterpret_cast<const TiffStream *>(fd);
    return (ts->m_inStream != nullptr || ts->m_outStream != nullptr ||
            ts->m_ioStream != nullptr);
}
