#ifndef NEMESISDECOMPRESSOR_H
#define NEMESISDECOMPRESSOR_H

#include "CompressionHandler.h"

// Nemesis compression — Huffman-based scheme specialized for 4bpp tile data.
// Used by early Sega titles including Michael Jackson's Moonwalker.
// Decompressor is a stub; returns empty with a warning until implemented.
class NemesisDecompressor : public IDecompressor
{
public:
    QByteArray decompress(const QByteArray & romData,
                          uint32_t offset,
                          uint32_t *outCompressedSize) override;

    QByteArray compress(const QByteArray & srcData) override;

    QString name() const override { return "nemesis"; }
};

#endif // NEMESISDECOMPRESSOR_H
