#ifndef RAWTILEBROWSERWIDGET_H
#define RAWTILEBROWSERWIDGET_H

#include <QWidget>
#include <QVector>
#include <QImage>
#include "TileDecoder.h"
#include <stdint.h>

class RawTileBrowserWidget : public QWidget
{
    Q_OBJECT
public:
    explicit RawTileBrowserWidget(QWidget *parent = nullptr);

    // rawData: bytes to decode (treated as a stream of 32-byte tiles)
    // romBaseOffset: the ROM file offset where rawData starts (for labels/signals)
    void setTileData(const QByteArray & rawData, uint32_t romBaseOffset,
                     const GenesisPalette & palette);
    void clearTiles();

    void setPalette(const GenesisPalette & palette);
    void setZoom(int factor);
    int zoom() const;

    // Assemble tiles into sprites of (w x h) tiles (column-major Genesis order).
    // Default 1x1 shows individual tiles as before.
    void setSpriteSize(int w, int h);
    int spriteW() const { return theSpriteW; }
    int spriteH() const { return theSpriteH; }

    // Skip N raw tiles before beginning sprite assembly.
    // Used to align the assembly grid to a specific ROM offset.
    void setAssemblyStart(int tileSkip);
    int assemblyStart() const { return theAssemblySkip; }

    // Scroll the parent QScrollArea so the item containing tileIndex is visible.
    // tileIndex is always a raw tile index (byte_offset / 32).
    void scrollToTile(int tileIndex);

    // Always returns the number of raw 8x8 tiles (theTileData.size() / 32).
    int tileCount() const { return theTileData.size() / 32; }

    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;

signals:
    // tileIndex = first raw tile index of the clicked sprite; romOffset = its ROM address
    void tileClicked(int tileIndex, uint32_t romOffset);

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    bool event(QEvent *e) override;
    void resizeEvent(QResizeEvent *event) override;

private:
    void rebuildImages();
    void recalcLayout();
    int itemIndexAt(const QPoint & pos) const;

    QByteArray        theTileData;
    uint32_t          theRomBaseOffset;
    GenesisPalette    thePalette;
    QVector<QImage>   theDecodedTiles;  // one entry per assembled sprite (or tile if 1x1)
    int               theZoom;
    int               theSelectedItem;  // index into theDecodedTiles

    // Sprite assembly size
    int               theSpriteW;
    int               theSpriteH;
    int               theAssemblySkip;  // raw tiles to skip before assembly

    // Layout cache (cell = one assembled sprite displayed on screen)
    int               theCellW;       // cell width in pixels  (spriteW * 8 * zoom + 2)
    int               theCellH;       // cell height in pixels (spriteH * 8 * zoom + 2)
    int               theTilesPerRow; // items (assembled sprites) per row
    int               theTotalRows;
};

#endif // RAWTILEBROWSERWIDGET_H
