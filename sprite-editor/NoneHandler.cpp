#include "NoneHandler.h"

QByteArray NoneHandler::decompress(const QByteArray & romData,
                                   uint32_t offset,
                                   uint32_t *outCompressedSize)
{
    uint32_t available = (uint32_t)romData.size();
    if (offset >= available)
    {
        if (outCompressedSize) *outCompressedSize = 0;
        return QByteArray();
    }
    uint32_t toRead = (uint32_t)theByteCount;
    if (offset + toRead > available)
        toRead = available - offset;

    if (outCompressedSize) *outCompressedSize = toRead;
    return romData.mid(offset, toRead);
}

QByteArray NoneHandler::compress(const QByteArray & srcData)
{
    // "none" compression — data is already raw, return as-is
    return srcData;
}
