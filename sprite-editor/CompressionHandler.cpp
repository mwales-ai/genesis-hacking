#include "CompressionHandler.h"
#include "NoneHandler.h"
#include "KosinskiDecompressor.h"
#include "NemesisDecompressor.h"
#include <QMessageBox>

CompressionHandler::CompressionHandler()
{
    theNoneHandler      = new NoneHandler();
    theKosinskiHandler  = new KosinskiDecompressor();
    theNemesisHandler   = new NemesisDecompressor();
}

CompressionHandler::~CompressionHandler()
{
    delete theNoneHandler;
    delete theKosinskiHandler;
    delete theNemesisHandler;
}

IDecompressor *CompressionHandler::decompressorFor(const QString & type)
{
    if (type == "none")      return theNoneHandler;
    if (type == "kosinski")  return theKosinskiHandler;
    if (type == "nemesis")   return theNemesisHandler;
    return nullptr;
}

QByteArray CompressionHandler::decompress(const QString & type,
                                           const QByteArray & romData,
                                           uint32_t offset,
                                           int byteCount,
                                           uint32_t *outCompressedSize)
{
    IDecompressor *d = decompressorFor(type);
    if (!d)
    {
        QMessageBox::warning(nullptr, "Unknown Compression",
            QString("Unknown compression type: '%1'").arg(type));
        if (outCompressedSize) *outCompressedSize = 0;
        return QByteArray();
    }

    // NoneHandler needs the byte count set before calling
    if (type == "none")
        static_cast<NoneHandler*>(d)->setByteCount(byteCount);

    return d->decompress(romData, offset, outCompressedSize);
}

QByteArray CompressionHandler::compress(const QString & type, const QByteArray & srcData)
{
    IDecompressor *d = decompressorFor(type);
    if (!d)
    {
        QMessageBox::warning(nullptr, "Unknown Compression",
            QString("Unknown compression type: '%1'").arg(type));
        return QByteArray();
    }
    return d->compress(srcData);
}

QStringList CompressionHandler::supportedTypes() const
{
    return QStringList() << "none" << "kosinski" << "nemesis";
}
