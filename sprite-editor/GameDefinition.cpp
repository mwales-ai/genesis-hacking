#include "GameDefinition.h"
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <iostream>

#define GameDefDebug if(0) std::cout

GameDefinition::GameDefinition()
    : theLoaded(false)
{
}

bool GameDefinition::loadFromFile(const QString & path)
{
    theLoaded = false;
    theLastError.clear();
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

uint32_t GameDefinition::parseOffset(const QString & hexStr, bool *ok)
{
    QString s = hexStr.trimmed();
    if (s.startsWith("0x") || s.startsWith("0X"))
        s = s.mid(2);
    return s.toUInt(ok, 16);
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

    // Parse tile_ranges (optional)
    QJsonArray ranges = root["tile_ranges"].toArray();
    for (const QJsonValue & rv : ranges)
    {
        QJsonObject ro = rv.toObject();
        TileRange range;
        range.label               = ro["label"].toString("Range");
        range.defaultPaletteGroup = ro["default_palette_group"].toInt(-1);
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

    // Parse screen_captures (optional)
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

    // Parse sprite_collections (optional — from BlastEm spritecap command)
    QJsonArray collections = root["sprite_collections"].toArray();
    for (const QJsonValue & colv : collections)
    {
        QJsonObject cobj = colv.toObject();
        SpriteCollection col;
        col.name = cobj["name"].toString("Unnamed Collection");

        // Bounding box
        QJsonObject bbox = cobj["bounding_box"].toObject();
        col.boundingBox = QRect(bbox["x"].toInt(0), bbox["y"].toInt(0),
                                bbox["width"].toInt(0), bbox["height"].toInt(0));

        // Palettes (same format as screen captures)
        QJsonArray pals = cobj["palettes"].toArray();
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
            col.palettes.append(pal);
        }

        // Sprites
        QJsonArray spArr = cobj["sprites"].toArray();
        for (const QJsonValue & sv : spArr)
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
            col.sprites.append(cs);
        }

        theSpriteCollections.append(col);
    }

    GameDefDebug << "GameDefinition: " << theSpriteGroups.size() << " groups, "
                 << theTileRanges.size() << " tile ranges, "
                 << theScreenCaptures.size() << " screen captures, "
                 << theSpriteCollections.size() << " sprite collections" << std::endl;

    theLoaded = true;
    return true;
}
