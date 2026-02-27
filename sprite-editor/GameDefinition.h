#ifndef GAMEDEFINITION_H
#define GAMEDEFINITION_H

#include <QString>
#include <QVector>
#include <stdint.h>

struct PaletteDefinition
{
    QString  name;
    uint32_t romOffset;  // byte offset to 16 x 2-byte CRAM entries (32 bytes)
};

struct SpriteEntry
{
    QString  name;
    uint32_t romOffset;
    int      widthTiles;
    int      heightTiles;
    int      frameCount;    // number of consecutive frames (default 1)
    int      paletteIndex;  // index into parent SpriteGroup::palettes
    QString  compression;   // "none", "kosinski", "nemesis"
    QString  notes;
};

struct SpriteGroup
{
    QString                    name;
    QVector<PaletteDefinition> palettes;
    QVector<SpriteEntry>       sprites;
};

struct TileRange
{
    QString  label;
    uint32_t startOffset;
    uint32_t endOffset;
    int      defaultPaletteGroup;  // -1 = none
};

class GameDefinition
{
public:
    GameDefinition();

    bool loadFromFile(const QString & path);
    bool isLoaded() const;
    QString lastError() const;

    QString gameName() const;
    QString gameId() const;

    const QVector<SpriteGroup> & spriteGroups() const;
    const QVector<TileRange>   & tileRanges() const;

private:
    bool parseJson(const QByteArray & jsonData);
    static uint32_t parseOffset(const QString & hexStr, bool *ok);

    QString               theGameName;
    QString               theGameId;
    QVector<SpriteGroup>  theSpriteGroups;
    QVector<TileRange>    theTileRanges;
    QString               theLastError;
    bool                  theLoaded;
};

#endif // GAMEDEFINITION_H
