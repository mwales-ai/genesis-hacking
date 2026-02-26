#ifndef ROMFILE_H
#define ROMFILE_H

#include <QString>
#include <QByteArray>
#include <stdint.h>

#define RomFileDebug if(0) std::cout

class RomFile
{
public:
    RomFile();

    bool openRom(const QString & path);
    bool saveRom(const QString & path = QString());

    bool isOpen() const;
    bool isModified() const;

    QString romPath() const;
    int romSize() const;

    // All ROM access goes through here
    QByteArray readBytes(uint32_t offset, uint32_t length) const;
    bool writeBytes(uint32_t offset, const QByteArray & data);

    // Checks for "SEGA" magic at offset 0x100
    bool looksLikeGenesisRom() const;

    // Domestic game title from ROM header (offset 0x120, 48 bytes)
    QString gameTitle() const;

private:
    QByteArray  theRomData;
    QString     theRomPath;
    bool        theModified;
};

#endif // ROMFILE_H
