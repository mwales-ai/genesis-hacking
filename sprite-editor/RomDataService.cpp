#include "RomDataService.h"
#include <QPainter>
#include <iostream>

RomDataService::RomDataService()
    : theRom(nullptr)
    , theDef(nullptr)
    , theCompressor(nullptr)
{
}

void RomDataService::setRom(RomFile *rom)
{
    theRom = rom;
}

void RomDataService::setDefinition(GameDefinition *def)
{
    theDef = def;
}

void RomDataService::setCompressor(CompressionHandler *comp)
{
    theCompressor = comp;
}

GenesisPalette RomDataService::resolvePalette(const QString & paletteId)
{
    if (!theDef || paletteId.isEmpty())
        return TileDecoder::greyPalette();

    const auto & pool = theDef->palettePool();
    if (!pool.contains(paletteId))
        return TileDecoder::greyPalette();

    const PoolPalette & pp = pool[paletteId];
    if (!pp.cramValues.isEmpty())
        return TileDecoder::decodePaletteFromCram(pp.cramValues);

    if (pp.romOffset != 0 && theRom && theRom->isOpen())
    {
        QByteArray palData = theRom->readBytes(pp.romOffset, 32);
        QVector<uint16_t> cramValues;
        for (int i = 0; i + 1 < palData.size() && cramValues.size() < 16; i += 2)
        {
            uint16_t word = (uint8_t(palData[i]) << 8) | uint8_t(palData[i + 1]);
            cramValues.append(word);
        }
        if (!cramValues.isEmpty())
            return TileDecoder::decodePaletteFromCram(cramValues);
    }

    return TileDecoder::greyPalette();
}

QByteArray RomDataService::fetchTileData(const SpriteEntry & entry)
{
    if (!theRom || !theRom->isOpen())
        return QByteArray();

    int totalBytes = entry.widthTiles * entry.heightTiles * 32;

    if (entry.compression == "none" || entry.compression.isEmpty())
    {
        return theRom->readBytes(entry.romOffset, totalBytes);
    }
    else if (theCompressor)
    {
        // Read a generous buffer for compressed data
        QByteArray romData = theRom->readBytes(entry.romOffset, totalBytes * 4);
        uint32_t compSize = 0;
        return theCompressor->decompress(entry.compression,
                                          romData, 0, totalBytes, &compSize);
    }

    return QByteArray();
}

QByteArray RomDataService::fetchPatternTileData(const PoolPattern & pat)
{
    if (pat.romOffset == 0)
        return pat.tileData;

    if (!theRom || !theRom->isOpen())
        return pat.tileData;

    int totalBytes = pat.widthTiles * pat.heightTiles * 32 * pat.frameCount;

    if (pat.compression == "none" || pat.compression.isEmpty())
    {
        return theRom->readBytes(pat.romOffset, totalBytes);
    }
    else if (theCompressor)
    {
        QByteArray romData = theRom->readBytes(pat.romOffset, totalBytes * 4);
        uint32_t compSize = 0;
        return theCompressor->decompress(pat.compression,
                                          romData, 0, totalBytes, &compSize);
    }

    return pat.tileData;
}

QByteArray RomDataService::readTileRange(uint32_t startOffset, uint32_t length)
{
    if (!theRom || !theRom->isOpen())
        return QByteArray();
    return theRom->readBytes(startOffset, length);
}

TileBlockGroup RomDataService::resolveNormalized(const NormalizedCollection & norm)
{
    TileBlockGroup group;
    group.name = norm.name;

    if (!theDef)
        return group;

    const auto & palPool = theDef->palettePool();
    const auto & patPool = theDef->patternPool();

    // Assign palette lines (max 4 unique palettes)
    QMap<QString, int> palLineMap;
    for (const auto & ns : norm.sprites)
    {
        if (!palLineMap.contains(ns.paletteId) && palLineMap.size() < 4)
            palLineMap.insert(ns.paletteId, palLineMap.size());
    }

    // Resolve palettes
    for (auto it = palLineMap.begin(); it != palLineMap.end(); ++it)
    {
        int line = it.value();
        group.palettes[line] = resolvePalette(it.key());
    }

    // Build tile blocks
    int16_t minX = 32767, minY = 32767, maxX = -32768, maxY = -32768;

    for (int i = 0; i < norm.sprites.size(); ++i)
    {
        const NormalizedSprite & ns = norm.sprites[i];
        TileBlock block;
        block.x = ns.x;
        block.y = ns.y;
        block.hFlip = ns.hFlip;
        block.vFlip = ns.vFlip;
        block.priority = ns.priority;
        block.paletteLine = palLineMap.value(ns.paletteId, 0);

        if (patPool.contains(ns.patternId))
        {
            const PoolPattern & pat = patPool[ns.patternId];
            block.widthTiles = pat.widthTiles;
            block.heightTiles = pat.heightTiles;

            if (!pat.tileData.isEmpty())
            {
                block.tileData = pat.tileData;
                block.source = "embedded";
            }
            else if (pat.romOffset != 0)
            {
                int bytesPerFrame = pat.widthTiles * pat.heightTiles * 32;
                uint32_t frameRomOffset = pat.romOffset + uint32_t(ns.frame * bytesPerFrame);
                block.romOffset = QString("0x%1").arg(frameRomOffset, 0, 16).toUpper();
                block.source = "dma";

                // Read tile data from ROM
                if (theRom && theRom->isOpen())
                    block.tileData = theRom->readBytes(frameRomOffset, bytesPerFrame);
            }
            else
            {
                block.source = "embedded";
            }
        }
        else
        {
            block.widthTiles = 1;
            block.heightTiles = 1;
            block.source = "embedded";
        }

        if (block.x < minX) minX = block.x;
        if (block.y < minY) minY = block.y;
        int right = block.x + block.widthTiles * 8;
        int bottom = block.y + block.heightTiles * 8;
        if (right > maxX) maxX = right;
        if (bottom > maxY) maxY = bottom;

        group.blocks.append(block);
    }

    if (!norm.sprites.isEmpty())
        group.boundingBox = QRect(minX, minY, maxX - minX, maxY - minY);

    return group;
}

TileBlockGroup RomDataService::resolveLegacy(const SpriteCollection & col)
{
    TileBlockGroup group;
    group.name = col.name;
    group.boundingBox = col.boundingBox;

    // Decode palettes from CRAM values
    for (int line = 0; line < 4 && line < col.palettes.size(); ++line)
    {
        if (!col.palettes[line].cramValues.isEmpty())
            group.palettes[line] = TileDecoder::decodePaletteFromCram(col.palettes[line].cramValues);
        else
            group.palettes[line] = TileDecoder::greyPalette();
    }

    // Convert CollectionSprites to TileBlocks
    for (const CollectionSprite & cs : col.sprites)
    {
        TileBlock block;
        block.widthTiles = cs.widthTiles;
        block.heightTiles = cs.heightTiles;
        block.x = cs.x;
        block.y = cs.y;
        block.hFlip = cs.hFlip;
        block.vFlip = cs.vFlip;
        block.priority = cs.priority;
        block.paletteLine = cs.paletteLine;
        block.romOffset = cs.romOffset;
        block.source = cs.source;
        block.tileData = cs.tileData;
        block.vramAddr = cs.vramAddr;

        // If ROM-sourced and no embedded data, read from ROM
        if (block.tileData.isEmpty() && !block.romOffset.isEmpty() &&
            theRom && theRom->isOpen())
        {
            bool ok = false;
            QString offStr = block.romOffset;
            if (offStr.startsWith("0x") || offStr.startsWith("0X"))
                offStr = offStr.mid(2);
            uint32_t offset = offStr.toUInt(&ok, 16);
            if (ok)
            {
                int totalBytes = block.widthTiles * block.heightTiles * 32;
                block.tileData = theRom->readBytes(offset, totalBytes);
            }
        }

        group.blocks.append(block);
    }

    return group;
}

TileBlockGroup RomDataService::resolveRecordingFrame(const SpriteRecording & rec, int frameIndex)
{
    if (frameIndex < 0 || frameIndex >= rec.frames().size())
        return TileBlockGroup();

    const AnimationFrame & frame = rec.frames()[frameIndex];

    // Build a SpriteCollection and resolve it via resolveLegacy
    SpriteCollection col;
    col.name = QString("%1 - frame %2").arg(rec.gameName()).arg(frame.frameNumber);
    col.boundingBox = frame.boundingBox;
    col.sprites = frame.sprites;
    col.palettes = rec.palettes();

    return resolveLegacy(col);
}

QImage RomDataService::renderComposite(const TileBlockGroup & group)
{
    int imgW = group.boundingBox.width();
    int imgH = group.boundingBox.height();
    if (imgW <= 0 || imgH <= 0)
        return QImage();

    int originX = group.boundingBox.x();
    int originY = group.boundingBox.y();

    QImage composite(imgW, imgH, QImage::Format_ARGB32);
    composite.fill(Qt::transparent);

    // Render blocks in reverse order (last = back, first = front)
    for (int i = group.blocks.size() - 1; i >= 0; --i)
    {
        const TileBlock & block = group.blocks[i];
        int palLine = qBound(0, block.paletteLine, 3);
        const GenesisPalette & pal = group.palettes[palLine];

        if (block.tileData.isEmpty())
            continue;

        QImage sprImg = TileDecoder::decodeSprite(
            block.tileData, block.widthTiles, block.heightTiles, pal);
        if (block.hFlip || block.vFlip)
            sprImg = sprImg.mirrored(block.hFlip, block.vFlip);

        int destX = block.x - originX;
        int destY = block.y - originY;
        QPainter painter(&composite);
        painter.drawImage(destX, destY, sprImg);
    }

    return composite;
}

QVector<PaletteInfo> RomDataService::availablePalettes()
{
    QVector<PaletteInfo> result;

    if (!theDef || !theDef->isLoaded())
        return result;

    if (theDef->isNormalized())
    {
        const auto & pool = theDef->palettePool();
        int line = 0;
        for (auto it = pool.begin(); it != pool.end(); ++it, ++line)
        {
            PaletteInfo info;
            info.id = it.key();
            info.name = it.value().name;
            info.line = line;
            info.romOffset = it.value().romOffset;
            info.colors = resolvePalette(it.key());
            result.append(info);
        }
    }
    else
    {
        int line = 0;
        for (const auto & g : theDef->spriteGroups())
        {
            for (const auto & pal : g.palettes)
            {
                PaletteInfo info;
                info.name = QString("%1 / %2").arg(g.name, pal.name);
                info.line = line++;
                info.romOffset = pal.romOffset;
                result.append(info);
            }
        }
    }

    return result;
}
