#include "TileDecoder.h"
#include <climits>
#include <iostream>

#define TileDebug if(0) std::cout

QImage TileDecoder::decodeTile(const QByteArray & tileData, int offset,
                               const GenesisPalette & palette)
{
    QImage img(8, 8, QImage::Format_ARGB32);

    for (int row = 0; row < 8; ++row)
    {
        for (int byteIdx = 0; byteIdx < 4; ++byteIdx)
        {
            uint8_t b = (uint8_t)tileData[offset + row * 4 + byteIdx];
            int leftPixel  = (b >> 4) & 0x0f;
            int rightPixel = b & 0x0f;

            int col = byteIdx * 2;
            img.setPixel(col,     row, leftPixel  == 0 ? 0x00000000u : palette[leftPixel].rgba());
            img.setPixel(col + 1, row, rightPixel == 0 ? 0x00000000u : palette[rightPixel].rgba());
        }
    }
    return img;
}

QImage TileDecoder::decodeSprite(const QByteArray & tileData,
                                 int widthTiles, int heightTiles,
                                 const GenesisPalette & palette)
{
    QImage img(widthTiles * 8, heightTiles * 8, QImage::Format_ARGB32);
    img.fill(Qt::transparent);

    int totalTiles = widthTiles * heightTiles;
    for (int col = 0; col < widthTiles; ++col)
    {
        for (int row = 0; row < heightTiles; ++row)
        {
            // Column-major order: tile index = col * heightTiles + row
            int tileIdx = col * heightTiles + row;
            if (tileIdx >= totalTiles)
                break;

            int tileOffset = tileIdx * 32;
            if (tileOffset + 32 > tileData.size())
                break;

            QImage tile = decodeTile(tileData, tileOffset, palette);

            // Copy tile into the sprite image
            for (int ty = 0; ty < 8; ++ty)
                for (int tx = 0; tx < 8; ++tx)
                    img.setPixel(col * 8 + tx, row * 8 + ty, tile.pixel(tx, ty));
        }
    }
    return img;
}

QByteArray TileDecoder::encodeSprite(const QImage & image,
                                     int widthTiles, int heightTiles,
                                     const GenesisPalette & palette)
{
    int expectedW = widthTiles * 8;
    int expectedH = heightTiles * 8;
    if (image.width() != expectedW || image.height() != expectedH)
        return QByteArray();

    int totalBytes = widthTiles * heightTiles * 32;
    QByteArray result(totalBytes, (char)0);

    for (int col = 0; col < widthTiles; ++col)
    {
        for (int row = 0; row < heightTiles; ++row)
        {
            int tileIdx = col * heightTiles + row;
            int tileByteOffset = tileIdx * 32;

            for (int ty = 0; ty < 8; ++ty)
            {
                for (int byteIdx = 0; byteIdx < 4; ++byteIdx)
                {
                    int px = col * 8 + byteIdx * 2;
                    int py = row * 8 + ty;

                    QColor leftColor(image.pixel(px, py));
                    QColor rightColor(image.pixel(px + 1, py));

                    int leftIdx  = nearestPaletteIndex(leftColor,  palette);
                    int rightIdx = nearestPaletteIndex(rightColor, palette);

                    uint8_t b = (uint8_t)((leftIdx << 4) | (rightIdx & 0x0f));
                    result[tileByteOffset + ty * 4 + byteIdx] = (char)b;
                }
            }
        }
    }
    return result;
}

QImage TileDecoder::decodeTileFlipped(const QByteArray & tileData, int offset,
                                      const GenesisPalette & palette,
                                      bool hFlip, bool vFlip)
{
    QImage img = decodeTile(tileData, offset, palette);
    if (hFlip || vFlip)
        img = img.mirrored(hFlip, vFlip);
    return img;
}

GenesisPalette TileDecoder::decodePaletteFromCram(const QVector<uint16_t> & cramValues)
{
    GenesisPalette pal;
    pal.reserve(16);
    for (int i = 0; i < 16 && i < cramValues.size(); ++i)
        pal.append(cramWordToColor(cramValues[i]));
    while (pal.size() < 16)
        pal.append(QColor(0, 0, 0, 0));
    return pal;
}

QColor TileDecoder::cramWordToColor(uint16_t cramWord)
{
    // Genesis CRAM format: ---- bbb- ggg- rrr-
    int r = (cramWord >> 1) & 0x07;
    int g = (cramWord >> 5) & 0x07;
    int b = (cramWord >> 9) & 0x07;

    // Scale 3-bit channel (0-7) to 8-bit (0-255): multiply by 36
    return QColor(r * 36, g * 36, b * 36);
}

GenesisPalette TileDecoder::decodePalette(const QByteArray & cramData, int offset)
{
    GenesisPalette pal;
    pal.reserve(16);
    for (int i = 0; i < 16; ++i)
    {
        int bytePos = offset + i * 2;
        if (bytePos + 1 >= cramData.size())
        {
            pal.append(QColor(0, 0, 0, 0));
            continue;
        }
        uint16_t word = ((uint8_t)cramData[bytePos] << 8) | (uint8_t)cramData[bytePos + 1];
        pal.append(cramWordToColor(word));
    }
    return pal;
}

GenesisPalette TileDecoder::greyPalette()
{
    GenesisPalette pal;
    for (int i = 0; i < 16; ++i)
    {
        int v = i * 17; // 0, 17, 34, ... 255
        pal.append(QColor(v, v, v));
    }
    return pal;
}

QImage TileDecoder::scaleForDisplay(const QImage & src, int scaleFactor)
{
    if (scaleFactor <= 1)
        return src;
    return src.scaled(src.width() * scaleFactor, src.height() * scaleFactor,
                      Qt::IgnoreAspectRatio, Qt::FastTransformation);
}

int TileDecoder::nearestPaletteIndex(const QColor & color, const GenesisPalette & palette)
{
    // Transparent pixels map to index 0
    if (color.alpha() < 128)
        return 0;

    int bestIdx = 1;
    int bestDist = INT_MAX;
    for (int i = 1; i < palette.size(); ++i)
    {
        int dr = color.red()   - palette[i].red();
        int dg = color.green() - palette[i].green();
        int db = color.blue()  - palette[i].blue();
        int dist = dr*dr + dg*dg + db*db;
        if (dist < bestDist)
        {
            bestDist = dist;
            bestIdx  = i;
        }
    }
    return bestIdx;
}
