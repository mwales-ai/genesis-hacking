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

    GameDefDebug << "GameDefinition: " << theSpriteGroups.size() << " groups, "
                 << theTileRanges.size() << " tile ranges" << std::endl;

    theLoaded = true;
    return true;
}
