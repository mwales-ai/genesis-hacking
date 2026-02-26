#ifndef COMPRESSIONHANDLER_H
#define COMPRESSIONHANDLER_H

#include <QByteArray>
#include <QString>
#include <QStringList>
#include <stdint.h>

// Abstract base for all compression strategies
class IDecompressor
{
public:
    virtual ~IDecompressor() {}

    // Decompress from romData starting at offset.
    // Sets outCompressedSize to bytes consumed from romData.
    // Returns empty QByteArray on failure.
    virtual QByteArray decompress(const QByteArray & romData,
                                  uint32_t offset,
                                  uint32_t *outCompressedSize) = 0;

    // Compress srcData.  May return empty QByteArray if not implemented.
    virtual QByteArray compress(const QByteArray & srcData) = 0;

    virtual QString name() const = 0;
};

class CompressionHandler
{
public:
    CompressionHandler();
    ~CompressionHandler();

    // Decompress using the named algorithm.
    // For "none", byteCount bytes are read starting at offset.
    QByteArray decompress(const QString & type,
                          const QByteArray & romData,
                          uint32_t offset,
                          int byteCount,
                          uint32_t *outCompressedSize);

    // Compress using the named algorithm.
    QByteArray compress(const QString & type, const QByteArray & srcData);

    QStringList supportedTypes() const;

private:
    IDecompressor *decompressorFor(const QString & type);

    IDecompressor *theNoneHandler;
    IDecompressor *theKosinskiHandler;
    IDecompressor *theNemesisHandler;
};

#endif // COMPRESSIONHANDLER_H
