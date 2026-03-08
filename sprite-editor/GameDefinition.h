#ifndef GAMEDEFINITION_H
#define GAMEDEFINITION_H

#include <QString>
#include <QVector>
#include <QMap>
#include <QByteArray>
#include <QRect>
#include <QJsonArray>
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

struct ScreenCapturePalette
{
    int                line;        // 0-3
    QVector<uint16_t>  cramValues;  // 16 raw Genesis CRAM words
    QString            dmaSource;   // hex offset or empty
};

struct TileMapEntry
{
    int     row, col;
    int     pattern;
    int     paletteLine;
    bool    hFlip, vFlip, priority;
    QString romOffset;   // hex string or empty
    QString source;      // "dma", "search", "embedded", "blank"
};

struct ScreenCapture
{
    QString                       name;
    int                           widthTiles;
    int                           heightTiles;
    QVector<ScreenCapturePalette> palettes;
    QVector<TileMapEntry>         tileMap;
    QMap<QString, QByteArray>     embeddedTiles;  // VRAM addr hex -> 32 bytes
};

struct CollectionSprite
{
    int      index;         // hardware sprite index
    int      x, y;          // screen position (already offset by -128)
    int      widthTiles;
    int      heightTiles;
    int      paletteLine;   // 0-3
    bool     priority;
    bool     hFlip, vFlip;
    int      pattern;       // tile pattern index
    QString  vramAddr;      // hex string
    QString  romOffset;     // hex string or empty
    QString  source;        // "dma", "search", "embedded"
    QByteArray tileData;    // embedded tile data (if source == "embedded")
};

struct SpriteCollection
{
    QString                       name;
    QRect                         boundingBox;  // overall bounding box
    QVector<ScreenCapturePalette> palettes;     // reuse same palette struct (4 lines)
    QVector<CollectionSprite>     sprites;
};

struct AnimationFrame
{
    uint32_t                  frameNumber;
    QRect                     boundingBox;
    QVector<CollectionSprite> sprites;
};

struct SpriteAnimation
{
    QString                       gameName;
    QVector<ScreenCapturePalette> palettes;   // shared across all frames
    QVector<AnimationFrame>       frames;
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

    const QVector<SpriteGroup>   & spriteGroups() const;
    const QVector<TileRange>     & tileRanges() const;
    const QVector<ScreenCapture>     & screenCaptures() const;
    const QVector<SpriteCollection> & spriteCollections() const;
    const QVector<SpriteAnimation>  & spriteAnimations() const;

private:
    bool parseJson(const QByteArray & jsonData);
    static uint32_t parseOffset(const QString & hexStr, bool *ok);
    static QVector<ScreenCapturePalette> parsePalettes(const QJsonArray & arr);
    static QVector<CollectionSprite> parseSprites(const QJsonArray & arr);

    QString               theGameName;
    QString               theGameId;
    QVector<SpriteGroup>   theSpriteGroups;
    QVector<TileRange>     theTileRanges;
    QVector<ScreenCapture>     theScreenCaptures;
    QVector<SpriteCollection> theSpriteCollections;
    QVector<SpriteAnimation>  theSpriteAnimations;
    QString                theLastError;
    bool                   theLoaded;
};

#endif // GAMEDEFINITION_H
