#ifndef TILEDECODER_H
#define TILEDECODER_H

#include <QImage>
#include <QByteArray>
#include <QVector>
#include <QColor>
#include <stdint.h>

// One Genesis palette: 16 colors.  Index 0 is always transparent.
typedef QVector<QColor> GenesisPalette;

class TileDecoder
{
public:
    // Decode a single 8x8 tile from 32 bytes of 4bpp data.
    // Returns QImage(8, 8, ARGB32).  Pixel index 0 = transparent.
    static QImage decodeTile(const QByteArray & tileData, int offset,
                             const GenesisPalette & palette);

    // Decode widthTiles x heightTiles tiles into one sprite image.
    // Genesis tile order is column-major: tile at (col, row) = col * heightTiles + row.
    // tileData must contain widthTiles * heightTiles * 32 bytes starting at offset.
    static QImage decodeSprite(const QByteArray & tileData,
                               int widthTiles, int heightTiles,
                               const GenesisPalette & palette);

    // Encode a QImage (must be widthTiles*8 x heightTiles*8) to Genesis 4bpp tile data.
    // Each pixel is quantized to the nearest palette entry by Euclidean RGB distance.
    // Returns empty QByteArray on dimension mismatch.
    static QByteArray encodeSprite(const QImage & image,
                                   int widthTiles, int heightTiles,
                                   const GenesisPalette & palette);

    // Convert a Genesis CRAM color word (---- bbb- ggg- rrr-) to QColor.
    static QColor cramWordToColor(uint16_t cramWord);

    // Parse 32 bytes of ROM CRAM data (16 x 16-bit big-endian words) into a palette.
    static GenesisPalette decodePalette(const QByteArray & cramData, int offset = 0);

    // Build a greyscale placeholder palette (useful when no palette is known).
    static GenesisPalette greyPalette();

    // Decode a single 8x8 tile with optional horizontal/vertical flip.
    static QImage decodeTileFlipped(const QByteArray & tileData, int offset,
                                    const GenesisPalette & palette,
                                    bool hFlip, bool vFlip);

    // Build a palette directly from 16 CRAM uint16_t values.
    static GenesisPalette decodePaletteFromCram(const QVector<uint16_t> & cramValues);

    // Convert a QColor to Genesis CRAM word (---- bbb- ggg- rrr-).
    // Each 8-bit channel is rounded to the nearest 3-bit value.
    static uint16_t colorToCramWord(const QColor & color);

    // Encode 16-color palette to 32 bytes of big-endian CRAM data.
    static QByteArray encodePalette(const GenesisPalette & palette);

    // Scale with nearest-neighbor (Qt::FastTransformation) to preserve pixel look.
    static QImage scaleForDisplay(const QImage & src, int scaleFactor);

private:
    // Returns the palette index (1-15) nearest to color; 0 for transparent pixels.
    static int nearestPaletteIndex(const QColor & color, const GenesisPalette & palette);
};

#endif // TILEDECODER_H
