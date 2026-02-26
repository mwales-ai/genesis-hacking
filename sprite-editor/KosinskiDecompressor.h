#ifndef KOSINSKIDECOMPRESSOR_H
#define KOSINSKIDECOMPRESSOR_H

#include "CompressionHandler.h"

// Kosinski LZ compression — used in Sonic and other Sega titles.
// Decompressor is a stub; returns empty with a warning until implemented.
class KosinskiDecompressor : public IDecompressor
{
public:
    QByteArray decompress(const QByteArray & romData,
                          uint32_t offset,
                          uint32_t *outCompressedSize) override;

    QByteArray compress(const QByteArray & srcData) override;

    QString name() const override { return "kosinski"; }
};

#endif // KOSINSKIDECOMPRESSOR_H
