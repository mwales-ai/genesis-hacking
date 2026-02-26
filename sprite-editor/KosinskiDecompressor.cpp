#include "KosinskiDecompressor.h"
#include <QMessageBox>

QByteArray KosinskiDecompressor::decompress(const QByteArray & /*romData*/,
                                             uint32_t /*offset*/,
                                             uint32_t *outCompressedSize)
{
    if (outCompressedSize) *outCompressedSize = 0;
    // TODO: implement Kosinski decompression
    QMessageBox::warning(nullptr, "Not Implemented",
        "Kosinski decompression is not yet implemented.\n"
        "Change the sprite's 'compression' field to 'none' if the data is uncompressed.");
    return QByteArray();
}

QByteArray KosinskiDecompressor::compress(const QByteArray & /*srcData*/)
{
    QMessageBox::warning(nullptr, "Not Implemented",
        "Kosinski re-compression is not yet implemented.\n"
        "Only sprites with compression 'none' can be replaced.");
    return QByteArray();
}
