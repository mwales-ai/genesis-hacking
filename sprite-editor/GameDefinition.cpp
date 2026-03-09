#include "GameDefinition.h"
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <iostream>

#define GameDefDebug if(0) std::cout

// ===========================================================================
// SpriteRecording
// ===========================================================================

SpriteRecording::SpriteRecording()
    : theLoaded(false)
{
}

bool SpriteRecording::loadFromFile(const QString & path)
{
    theLoaded = false;
    theLastError.clear();
    thePalettes.clear();
    theFrames.clear();

    QFile f(path);
    if (!f.open(QIODevice::ReadOnly))
    {
        theLastError = "Cannot open file: " + path;
        return false;
    }
    QByteArray data = f.readAll();
    return parseJson(data);
}

bool SpriteRecording::isLoaded() const
{
    return theLoaded;
}

QString SpriteRecording::lastError() const
{
    return theLastError;
}

QString SpriteRecording::gameName() const
{
    return theGameName;
}

const QVector<ScreenCapturePalette> & SpriteRecording::palettes() const
{
    return thePalettes;
}

const QVector<AnimationFrame> & SpriteRecording::frames() const
{
    return theFrames;
}

bool SpriteRecording::parseJson(const QByteArray & jsonData)
{
    QJsonParseError err;
    QJsonDocument doc = QJsonDocument::fromJson(jsonData, &err);
    if (doc.isNull())
    {
        theLastError = "JSON parse error: " + err.errorString();
        return false;
    }
    if (!doc.isObject())
    {
        theLastError = "JSON root must be an object";
        return false;
    }

    QJsonObject root = doc.object();

    // Support old format: if root has "sprite_animation", unwrap it
    if (root.contains("sprite_animation"))
    {
        root = root["sprite_animation"].toObject();
    }
    // Support old spritecap format: sprite_collections array with palettes + sprites
    else if (root.contains("sprite_collections") && !root.contains("frames"))
    {
        theGameName = root["game_name"].toString("Unknown");

        QJsonArray collections = root["sprite_collections"].toArray();
        for (const QJsonValue & colv : collections)
        {
            QJsonObject cobj = colv.toObject();
            AnimationFrame frame;
            frame.frameNumber = 0;

            QJsonObject bbox = cobj["bounding_box"].toObject();
            frame.boundingBox = QRect(bbox["x"].toInt(0), bbox["y"].toInt(0),
                                      bbox["width"].toInt(0), bbox["height"].toInt(0));

            // Use palettes from the collection
            thePalettes = GameDefinition::parsePalettes(cobj["palettes"].toArray());
            frame.sprites = GameDefinition::parseSprites(cobj["sprites"].toArray());
            theFrames.append(frame);
        }

        if (!theFrames.isEmpty())
        {
            theLoaded = true;
            GameDefDebug << "SpriteRecording: loaded old spritecap format '"
                         << theGameName.toStdString() << "' with "
                         << theFrames.size() << " frames" << std::endl;
        }
        return theLoaded;
    }

    theGameName = root["game_name"].toString("Unknown");
    thePalettes = GameDefinition::parsePalettes(root["palettes"].toArray());

    QJsonArray framesArr = root["frames"].toArray();
    for (const QJsonValue & fv : framesArr)
    {
        QJsonObject fo = fv.toObject();
        AnimationFrame frame;
        frame.frameNumber = fo["frame"].toInt(0);

        QJsonObject bbox = fo["bounding_box"].toObject();
        frame.boundingBox = QRect(bbox["x"].toInt(0), bbox["y"].toInt(0),
                                  bbox["width"].toInt(0), bbox["height"].toInt(0));
        frame.sprites = GameDefinition::parseSprites(fo["sprites"].toArray());

        theFrames.append(frame);
    }

    if (!theFrames.isEmpty())
    {
        theLoaded = true;
        GameDefDebug << "SpriteRecording: loaded '"
                     << theGameName.toStdString() << "' with "
                     << theFrames.size() << " frames" << std::endl;
    }
    return theLoaded;
}

// ===========================================================================
// GameDefinition
// ===========================================================================

GameDefinition::GameDefinition()
    : theNormalized(false)
    , theLoaded(false)
{
}

bool GameDefinition::loadFromFile(const QString & path)
{
    theLoaded = false;
    theNormalized = false;
    theLastError.clear();
    thePalettePool.clear();
    thePatternPool.clear();
    theNormalizedCollections.clear();
    theSpriteGroups.clear();
    theTileRanges.clear();
    theScreenCaptures.clear();
    theSpriteCollections.clear();

    QFile f(path);
    if (!f.open(QIODevice::ReadOnly))
    {
        theLastError = "Cannot open file: " + path;
        return false;
    }
    QByteArray data = f.readAll();
    return parseJson(data);
}

bool GameDefinition::isLoaded() const
{
    return theLoaded;
}

QString GameDefinition::lastError() const
{
    return theLastError;
}

QString GameDefinition::gameName() const
{
    return theGameName;
}

QString GameDefinition::gameId() const
{
    return theGameId;
}

const QMap<QString, PoolPalette> & GameDefinition::palettePool() const
{
    return thePalettePool;
}

const QMap<QString, PoolPattern> & GameDefinition::patternPool() const
{
    return thePatternPool;
}

const QVector<NormalizedCollection> & GameDefinition::normalizedCollections() const
{
    return theNormalizedCollections;
}

const QVector<SpriteGroup> & GameDefinition::spriteGroups() const
{
    return theSpriteGroups;
}

const QVector<TileRange> & GameDefinition::tileRanges() const
{
    return theTileRanges;
}

const QVector<ScreenCapture> & GameDefinition::screenCaptures() const
{
    return theScreenCaptures;
}

const QVector<SpriteCollection> & GameDefinition::spriteCollections() const
{
    return theSpriteCollections;
}

bool GameDefinition::isNormalized() const
{
    return theNormalized;
}

uint32_t GameDefinition::parseOffset(const QString & hexStr, bool *ok)
{
    QString s = hexStr.trimmed();
    if (s.startsWith("0x") || s.startsWith("0X"))
        s = s.mid(2);
    return s.toUInt(ok, 16);
}

QVector<ScreenCapturePalette> GameDefinition::parsePalettes(const QJsonArray & arr)
{
    QVector<ScreenCapturePalette> result;
    for (const QJsonValue & pv : arr)
    {
        QJsonObject po = pv.toObject();
        ScreenCapturePalette pal;
        pal.line = po["line"].toInt(0);
        QJsonArray cramArr = po["cram_values"].toArray();
        for (const QJsonValue & cval : cramArr)
        {
            bool ok = false;
            uint16_t word = cval.toString("0").toUInt(&ok, 16);
            pal.cramValues.append(word);
        }
        pal.dmaSource = po["dma_source"].isNull() ? QString() : po["dma_source"].toString();
        result.append(pal);
    }
    return result;
}

QVector<CollectionSprite> GameDefinition::parseSprites(const QJsonArray & arr)
{
    QVector<CollectionSprite> result;
    for (const QJsonValue & sv : arr)
    {
        QJsonObject so = sv.toObject();
        CollectionSprite cs;
        cs.index       = so["index"].toInt(0);
        cs.x           = so["x"].toInt(0);
        cs.y           = so["y"].toInt(0);
        cs.widthTiles  = qBound(1, so["width_tiles"].toInt(1), 4);
        cs.heightTiles = qBound(1, so["height_tiles"].toInt(1), 4);
        cs.paletteLine = qBound(0, so["palette_line"].toInt(0), 3);
        cs.priority    = so["priority"].toBool(false);
        cs.hFlip       = so["h_flip"].toBool(false);
        cs.vFlip       = so["v_flip"].toBool(false);
        cs.pattern     = so["pattern"].toInt(0);
        cs.vramAddr    = so["vram_addr"].toString();
        cs.romOffset   = so["rom_offset"].isNull() ? QString() : so["rom_offset"].toString();
        cs.source      = so["source"].toString("dma");

        // Parse embedded tile data if present
        QString tileHex = so["tile_data"].toString();
        if (!tileHex.isEmpty())
        {
            for (int i = 0; i + 1 < tileHex.length(); i += 2)
            {
                bool ok = false;
                uint8_t byte = tileHex.mid(i, 2).toUInt(&ok, 16);
                cs.tileData.append(static_cast<char>(byte));
            }
        }
        result.append(cs);
    }
    return result;
}

bool GameDefinition::parseJson(const QByteArray & jsonData)
{
    QJsonParseError err;
    QJsonDocument doc = QJsonDocument::fromJson(jsonData, &err);
    if (doc.isNull())
    {
        theLastError = "JSON parse error: " + err.errorString();
        return false;
    }
    if (!doc.isObject())
    {
        theLastError = "JSON root must be an object";
        return false;
    }

    QJsonObject root = doc.object();

    theGameName = root["game_name"].toString("Unknown Game");
    theGameId   = root["game_id"].toString("unknown");

    GameDefDebug << "GameDefinition: loading '" << theGameName.toStdString() << "'" << std::endl;

    // Detect format: "palettes" as object = new normalized format
    // "sprite_groups" or "palettes" as array = legacy format
    if (root.contains("palettes") && root["palettes"].isObject())
    {
        theNormalized = true;
        parseNormalizedFormat(root);
    }
    else
    {
        theNormalized = false;
        parseLegacyFormat(root);
    }

    // Parse tile_ranges (common to both formats)
    QJsonArray ranges = root["tile_ranges"].toArray();
    for (const QJsonValue & rv : ranges)
    {
        QJsonObject ro = rv.toObject();
        TileRange range;
        range.label               = ro["label"].toString("Range");
        range.defaultPaletteGroup = ro["default_palette_group"].toInt(-1);
        range.defaultPalette      = ro["default_palette"].toString();
        bool ok1 = false, ok2 = false;
        range.startOffset = parseOffset(ro["start_offset"].toString("0x200"), &ok1);
        range.endOffset   = parseOffset(ro["end_offset"].toString("0x80000"), &ok2);
        if (!ok1) range.startOffset = 0x200;
        if (!ok2) range.endOffset   = 0x80000;
        theTileRanges.append(range);
    }

    // Add a default "Full ROM" range if none were specified
    if (theTileRanges.isEmpty())
    {
        TileRange defaultRange;
        defaultRange.label               = "Full ROM (0x200 - end)";
        defaultRange.startOffset         = 0x200;
        defaultRange.endOffset           = 0x80000;
        defaultRange.defaultPaletteGroup = -1;
        theTileRanges.append(defaultRange);
    }

    // Parse screen_captures (common to both formats)
    QJsonArray captures = root["screen_captures"].toArray();
    for (const QJsonValue & cv : captures)
    {
        QJsonObject co = cv.toObject();
        ScreenCapture cap;
        cap.name        = co["name"].toString("Unnamed Capture");
        cap.widthTiles  = co["width_tiles"].toInt(40);
        cap.heightTiles = co["height_tiles"].toInt(28);

        // Parse palettes
        QJsonArray pals = co["palettes"].toArray();
        for (const QJsonValue & pv : pals)
        {
            QJsonObject po = pv.toObject();
            ScreenCapturePalette pal;
            pal.line = po["line"].toInt(0);
            QJsonArray cramArr = po["cram_values"].toArray();
            for (const QJsonValue & cval : cramArr)
            {
                bool ok = false;
                uint16_t word = cval.toString("0").toUInt(&ok, 16);
                pal.cramValues.append(word);
            }
            pal.dmaSource = po["dma_source"].isNull() ? QString() : po["dma_source"].toString();
            cap.palettes.append(pal);
        }

        // Parse tile_map
        QJsonArray tileArr = co["tile_map"].toArray();
        for (const QJsonValue & tv : tileArr)
        {
            QJsonObject to_ = tv.toObject();
            TileMapEntry entry;
            entry.row         = to_["row"].toInt(0);
            entry.col         = to_["col"].toInt(0);
            entry.pattern     = to_["pattern"].toInt(0);
            entry.paletteLine = to_["palette_line"].toInt(0);
            entry.hFlip       = to_["h_flip"].toBool(false);
            entry.vFlip       = to_["v_flip"].toBool(false);
            entry.priority    = to_["priority"].toBool(false);
            entry.romOffset   = to_["rom_offset"].isNull() ? QString() : to_["rom_offset"].toString();
            entry.source      = to_["source"].toString("blank");
            cap.tileMap.append(entry);
        }

        // Parse embedded_tiles
        QJsonObject embedObj = co["embedded_tiles"].toObject();
        for (auto it = embedObj.begin(); it != embedObj.end(); ++it)
        {
            QString hexData = it.value().toString();
            QByteArray tileBytes;
            for (int i = 0; i + 1 < hexData.length(); i += 2)
            {
                bool ok = false;
                uint8_t byte = hexData.mid(i, 2).toUInt(&ok, 16);
                tileBytes.append(static_cast<char>(byte));
            }
            cap.embeddedTiles.insert(it.key(), tileBytes);
        }

        theScreenCaptures.append(cap);
    }

    GameDefDebug << "GameDefinition: " << (theNormalized ? "normalized" : "legacy")
                 << " format, "
                 << thePalettePool.size() << " pool palettes, "
                 << thePatternPool.size() << " pool patterns, "
                 << theSpriteGroups.size() << " sprite groups, "
                 << theTileRanges.size() << " tile ranges, "
                 << theScreenCaptures.size() << " screen captures, "
                 << theSpriteCollections.size() << " sprite collections"
                 << std::endl;

    theLoaded = true;
    return true;
}

void GameDefinition::parseLegacyFormat(const QJsonObject & root)
{
    // Parse sprite_groups
    QJsonArray groups = root["sprite_groups"].toArray();
    for (const QJsonValue & gv : groups)
    {
        QJsonObject go = gv.toObject();
        SpriteGroup group;
        group.name = go["name"].toString("Unnamed Group");

        // Parse palettes
        QJsonArray palettes = go["palettes"].toArray();
        for (const QJsonValue & pv : palettes)
        {
            QJsonObject po = pv.toObject();
            PaletteDefinition pal;
            pal.name = po["name"].toString("Palette");
            bool ok = false;
            pal.romOffset = parseOffset(po["rom_offset"].toString("0x0"), &ok);
            if (!ok) pal.romOffset = 0;
            group.palettes.append(pal);
        }

        // Parse sprites
        QJsonArray sprites = go["sprites"].toArray();
        for (const QJsonValue & sv : sprites)
        {
            QJsonObject so = sv.toObject();
            SpriteEntry sprite;
            sprite.name         = so["name"].toString("Unnamed Sprite");
            sprite.widthTiles   = so["width_tiles"].toInt(1);
            sprite.heightTiles  = so["height_tiles"].toInt(1);
            sprite.frameCount   = so["frame_count"].toInt(1);
            sprite.paletteIndex = so["palette_index"].toInt(0);
            sprite.compression  = so["compression"].toString("none");
            sprite.notes        = so["notes"].toString();
            bool ok = false;
            sprite.romOffset = parseOffset(so["rom_offset"].toString("0x0"), &ok);
            if (!ok) sprite.romOffset = 0;

            // Clamp tile dimensions to valid Genesis range (up to 8x8 tiles = 64x64px)
            sprite.widthTiles  = qBound(1, sprite.widthTiles,  8);
            sprite.heightTiles = qBound(1, sprite.heightTiles, 8);
            sprite.frameCount  = qMax(1, sprite.frameCount);
            sprite.paletteIndex = qBound(0, sprite.paletteIndex, 3);

            group.sprites.append(sprite);
        }

        theSpriteGroups.append(group);
    }

    // Parse legacy sprite_collections (from old spritecap format in game defs)
    QJsonArray collections = root["sprite_collections"].toArray();
    for (const QJsonValue & colv : collections)
    {
        QJsonObject cobj = colv.toObject();
        SpriteCollection col;
        col.name = cobj["name"].toString("Unnamed Collection");

        QJsonObject bbox = cobj["bounding_box"].toObject();
        col.boundingBox = QRect(bbox["x"].toInt(0), bbox["y"].toInt(0),
                                bbox["width"].toInt(0), bbox["height"].toInt(0));
        col.palettes = parsePalettes(cobj["palettes"].toArray());
        col.sprites  = parseSprites(cobj["sprites"].toArray());

        theSpriteCollections.append(col);
    }
}

void GameDefinition::parseNormalizedFormat(const QJsonObject & root)
{
    // Parse "palettes" object (keyed by ID)
    QJsonObject palettesObj = root["palettes"].toObject();
    for (auto it = palettesObj.begin(); it != palettesObj.end(); ++it)
    {
        PoolPalette pal;
        pal.id = it.key();
        QJsonObject po = it.value().toObject();
        pal.name = po["name"].toString(pal.id);

        bool ok = false;
        pal.romOffset = parseOffset(po["rom_offset"].toString("0x0"), &ok);
        if (!ok) pal.romOffset = 0;

        QJsonArray cramArr = po["cram_values"].toArray();
        for (const QJsonValue & cval : cramArr)
        {
            bool cvOk = false;
            uint16_t word = cval.toString("0").toUInt(&cvOk, 16);
            pal.cramValues.append(word);
        }

        thePalettePool.insert(pal.id, pal);
    }

    // Parse "patterns" object (keyed by ID)
    QJsonObject patternsObj = root["patterns"].toObject();
    for (auto it = patternsObj.begin(); it != patternsObj.end(); ++it)
    {
        PoolPattern pat;
        pat.id = it.key();
        QJsonObject po = it.value().toObject();
        pat.name        = po["name"].toString(pat.id);
        pat.widthTiles  = qBound(1, po["width_tiles"].toInt(1), 8);
        pat.heightTiles = qBound(1, po["height_tiles"].toInt(1), 8);
        pat.frameCount  = qMax(1, po["frame_count"].toInt(1));
        pat.compression = po["compression"].toString("none");

        bool ok = false;
        pat.romOffset = parseOffset(po["rom_offset"].toString("0x0"), &ok);
        if (!ok) pat.romOffset = 0;

        // Parse optional embedded tile data
        QString tileHex = po["tile_data"].toString();
        if (!tileHex.isEmpty())
        {
            for (int i = 0; i + 1 < tileHex.length(); i += 2)
            {
                bool hok = false;
                uint8_t byte = tileHex.mid(i, 2).toUInt(&hok, 16);
                pat.tileData.append(static_cast<char>(byte));
            }
        }

        thePatternPool.insert(pat.id, pat);
    }

    // Parse "sprite_collections" object (keyed by ID)
    QJsonObject colsObj = root["sprite_collections"].toObject();
    for (auto it = colsObj.begin(); it != colsObj.end(); ++it)
    {
        NormalizedCollection col;
        col.id = it.key();
        QJsonObject co = it.value().toObject();
        col.name = co["name"].toString(col.id);

        QJsonArray spritesArr = co["sprites"].toArray();
        for (const QJsonValue & sv : spritesArr)
        {
            QJsonObject so = sv.toObject();
            NormalizedSprite ns;
            ns.patternId = so["pattern"].toString();
            ns.frame     = so["frame"].toInt(0);
            ns.paletteId = so["palette"].toString();
            ns.x         = so["x"].toInt(0);
            ns.y         = so["y"].toInt(0);
            ns.hFlip     = so["h_flip"].toBool(false);
            ns.vFlip     = so["v_flip"].toBool(false);
            ns.priority  = so["priority"].toBool(false);
            col.sprites.append(ns);
        }

        theNormalizedCollections.append(col);
    }
}
