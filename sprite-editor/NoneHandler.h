#ifndef NONEHANDLER_H
#define NONEHANDLER_H

#include "CompressionHandler.h"

// Trivial passthrough — raw uncompressed tile data.
class NoneHandler : public IDecompressor
{
public:
    // byteCount is passed via the wrapper; we store it on construction per call
    // Actually: decompress just reads byteCount bytes from the data.
    // outCompressedSize = byteCount (consumed == produced for "none")
    QByteArray decompress(const QByteArray & romData,
                          uint32_t offset,
                          uint32_t *outCompressedSize) override;

    QByteArray compress(const QByteArray & srcData) override;

    QString name() const override { return "none"; }

    // Extra parameter for "none" only: how many bytes to read
    void setByteCount(int count) { theByteCount = count; }

private:
    int theByteCount = 0;
};

#endif // NONEHANDLER_H
