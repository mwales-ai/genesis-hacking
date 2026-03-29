#ifndef GENESISTYPES_H
#define GENESISTYPES_H

#include <QString>
#include <QVector>
#include <QByteArray>
#include <QRect>
#include "TileDecoder.h"

/**
 * A resolved tile block: raw bytes + geometry + palette + transform.
 * This is the universal sprite piece that every widget can render.
 * Works for sprite group members AND individual screen capture tiles.
 */
struct TileBlock
{
    QByteArray  tileData;       // raw 4bpp Genesis tile bytes (empty if ROM-only)
    int         widthTiles;     // 1-8
    int         heightTiles;    // 1-8
    int         x, y;           // pixel position within a group (0-based)
    bool        hFlip, vFlip;
    bool        priority;
    int         paletteLine;    // 0-3
    QString     romOffset;      // hex string like "0x1234", empty if embedded-only
    QString     source;         // "dma", "search", "embedded", "ram"
    QString     vramAddr;       // VRAM address where tile data was at capture time
    QString     dmaSource;      // DMA source address (ROM or RAM), empty if unknown
};

/**
 * A resolved group of tile blocks composited together with decoded palettes.
 * This is what widgets receive for rendering and editing.
 * Used for sprite groups, screen captures, and single sprites alike.
 */
struct TileBlockGroup
{
    QString                name;
    QRect                  boundingBox;
    GenesisPalette         palettes[4];
    QVector<TileBlock>     blocks;
};

/**
 * Palette metadata for display and identification.
 */
struct PaletteInfo
{
    QString         id;         // pool ID or empty
    QString         name;
    int             line;       // 0-3
    uint32_t        romOffset;  // 0 if embedded only
    GenesisPalette  colors;
};

#endif // GENESISTYPES_H
