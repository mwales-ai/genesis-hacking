#include "NemesisDecompressor.h"
#include <QMessageBox>

QByteArray NemesisDecompressor::decompress(const QByteArray & /*romData*/,
                                            uint32_t /*offset*/,
                                            uint32_t *outCompressedSize)
{
    if (outCompressedSize) *outCompressedSize = 0;
    // TODO: implement Nemesis decompression
    QMessageBox::warning(nullptr, "Not Implemented",
        "Nemesis decompression is not yet implemented.\n"
        "Change the sprite's 'compression' field to 'none' if the data is uncompressed.\n"
        "Use the Raw Tile Browser to find uncompressed tile regions first.");
    return QByteArray();
}

QByteArray NemesisDecompressor::compress(const QByteArray & /*srcData*/)
{
    QMessageBox::warning(nullptr, "Not Implemented",
        "Nemesis re-compression is not yet implemented.\n"
        "Only sprites with compression 'none' can be replaced.");
    return QByteArray();
}
