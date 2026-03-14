#ifndef GAMEDEFINITION_H
#define GAMEDEFINITION_H

#include <QString>
#include <QVector>
#include <QMap>
#include <QByteArray>
#include <QRect>
#include <QJsonArray>
#include <stdint.h>

// ---------------------------------------------------------------------------
// New normalized pool types
// ---------------------------------------------------------------------------

struct PoolPalette
{
    QString           id;          // string key (e.g., "mj_gray_suit")
    QString           name;        // display name
    uint32_t          romOffset;   // 0 if using inline cram_values
    QVector<uint16_t> cramValues;  // 16 CRAM words (empty if using romOffset)
};

struct PoolPattern
{
    QString    id;
    QString    name;
    uint32_t   romOffset;    // 0 if embedded only
    int        widthTiles;   // clamped [1, 8]
    int        heightTiles;  // clamped [1, 8]
    int        frameCount;   // >= 1
    QString    compression;  // "none", "kosinski", "nemesis"
    QByteArray tileData;     // optional embedded (empty if romOffset set)
};

struct NormalizedSprite
{
    QString patternId;    // -> PoolPattern
    int     frame;        // which frame within pattern (0-based)
    QString paletteId;    // -> PoolPalette
    int     x, y;         // position relative to collection origin
    bool    hFlip, vFlip;
    bool    priority;
};

struct NormalizedCollection
{
    QString                    id;
    QString                    name;
    QVector<NormalizedSprite>  sprites;
};

// ---------------------------------------------------------------------------
// Legacy types (used by .sprec files and backward compat)
// ---------------------------------------------------------------------------

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
    int      defaultPaletteGroup;  // -1 = none (legacy: index into sprite_groups)
    QString  defaultPalette;       // new: palette pool ID (empty = none)
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

// ---------------------------------------------------------------------------
// SpriteRecording — loaded from .sprec files
// ---------------------------------------------------------------------------

class SpriteRecording
{
public:
    SpriteRecording();

    bool loadFromFile(const QString & path);
    bool isLoaded() const;
    QString lastError() const;

    QString gameName() const;
    const QVector<ScreenCapturePalette> & palettes() const;
    const QVector<AnimationFrame> & frames() const;

private:
    bool parseJson(const QByteArray & jsonData);

    QString                       theGameName;
    QVector<ScreenCapturePalette> thePalettes;
    QVector<AnimationFrame>       theFrames;
    QString                       theLastError;
    bool                          theLoaded;
};

// ---------------------------------------------------------------------------
// GameDefinition
// ---------------------------------------------------------------------------

class GameDefinition
{
public:
    GameDefinition();

    bool loadFromFile(const QString & path);
    bool isLoaded() const;
    QString lastError() const;

    QString gameName() const;
    QString gameId() const;

    // New normalized pools
    const QMap<QString, PoolPalette> & palettePool() const;
    const QMap<QString, PoolPattern> & patternPool() const;
    const QVector<NormalizedCollection> & normalizedCollections() const;

    // Legacy accessors (still used during transition)
    const QVector<SpriteGroup>   & spriteGroups() const;
    const QVector<TileRange>     & tileRanges() const;
    const QVector<ScreenCapture>     & screenCaptures() const;
    void addScreenCapture(const ScreenCapture & cap);
    void removeScreenCapture(int index);
    const QVector<SpriteCollection> & spriteCollections() const;

    // Whether the definition uses the new normalized format
    bool isNormalized() const;

    // Promote to normalized format (enables capture workflow)
    void ensureNormalized();

    // Initialize an empty definition (for capture without a pre-existing file)
    void initEmpty(const QString & gameName, const QString & gameId);

    // Mutable accessors for capture workflow
    void addPoolPalette(const PoolPalette & pal);
    void addPoolPattern(const PoolPattern & pat);
    void addNormalizedCollection(const NormalizedCollection & col);
    bool hasPaletteId(const QString & id) const;
    bool hasPatternId(const QString & id) const;
    bool hasCollectionId(const QString & id) const;
    void renameCollection(int index, const QString & newName);
    void moveNormalizedCollection(int fromIndex, int toIndex);
    void removeNormalizedCollection(int index);

    /** Count how many sprites across all collections reference this palette ID. */
    int countPaletteReferences(const QString & paletteId) const;

    // Serialization
    bool saveToFile(const QString & path);
    QString definitionPath() const;

    static bool loadScreenCaptureFromFile(const QString & path, QVector<ScreenCapture> & out);
    static uint32_t parseOffset(const QString & hexStr, bool *ok);
    static QVector<ScreenCapturePalette> parsePalettes(const QJsonArray & arr);
    static QVector<CollectionSprite> parseSprites(const QJsonArray & arr);

private:
    QJsonObject toJson() const;

    bool parseJson(const QByteArray & jsonData);
    void parseLegacyFormat(const QJsonObject & root);
    void parseNormalizedFormat(const QJsonObject & root);

    QString               theGameName;
    QString               theGameId;
    QString               theDefinitionPath;
    bool                  theNormalized;

    // New pool storage
    QMap<QString, PoolPalette>   thePalettePool;
    QMap<QString, PoolPattern>   thePatternPool;
    QVector<NormalizedCollection> theNormalizedCollections;

    // Legacy storage
    QVector<SpriteGroup>       theSpriteGroups;
    QVector<TileRange>         theTileRanges;
    QVector<ScreenCapture>     theScreenCaptures;
    QVector<SpriteCollection>  theSpriteCollections;

    QString                theLastError;
    bool                   theLoaded;
};

#endif // GAMEDEFINITION_H
