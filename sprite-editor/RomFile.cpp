#include "RomFile.h"
#include <QFile>
#include <iostream>

RomFile::RomFile()
    : theModified(false)
{
}

bool RomFile::openRom(const QString & path)
{
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly))
    {
        RomFileDebug << "RomFile: failed to open " << path.toStdString() << std::endl;
        return false;
    }
    theRomData = f.readAll();
    theRomPath = path;
    theModified = false;
    RomFileDebug << "RomFile: loaded " << theRomData.size() << " bytes from "
                 << path.toStdString() << std::endl;
    return true;
}

bool RomFile::saveRom(const QString & path)
{
    QString savePath = path.isEmpty() ? theRomPath : path;
    QFile f(savePath);
    if (!f.open(QIODevice::WriteOnly))
        return false;
    f.write(theRomData);
    theRomPath = savePath;
    theModified = false;
    return true;
}

bool RomFile::isOpen() const
{
    return !theRomData.isEmpty();
}

bool RomFile::isModified() const
{
    return theModified;
}

QString RomFile::romPath() const
{
    return theRomPath;
}

int RomFile::romSize() const
{
    return theRomData.size();
}

QByteArray RomFile::readBytes(uint32_t offset, uint32_t length) const
{
    if (offset >= (uint32_t)theRomData.size())
        return QByteArray();
    uint32_t available = theRomData.size() - offset;
    uint32_t toRead = (length < available) ? length : available;
    return theRomData.mid(offset, toRead);
}

bool RomFile::writeBytes(uint32_t offset, const QByteArray & data)
{
    if (data.isEmpty())
        return false;
    if (offset + data.size() > (uint32_t)theRomData.size())
        return false;
    theRomData.replace(offset, data.size(), data);
    theModified = true;
    return true;
}

bool RomFile::looksLikeGenesisRom() const
{
    if (theRomData.size() < 0x110)
        return false;
    QByteArray magic = theRomData.mid(0x100, 16);
    QString magicStr = QString::fromLatin1(magic).toUpper();
    return magicStr.contains("SEGA");
}

QString RomFile::gameTitle() const
{
    if (theRomData.size() < 0x150)
        return QString();
    QByteArray title = theRomData.mid(0x120, 48);
    return QString::fromLatin1(title).trimmed();
}
