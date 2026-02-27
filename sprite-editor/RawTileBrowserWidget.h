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

    // Scroll the parent QScrollArea so tileIndex is visible.
    void scrollToTile(int tileIndex);
    int tileCount() const { return theDecodedTiles.size(); }

    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;

signals:
    void tileClicked(int tileIndex, uint32_t romOffset);

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    bool event(QEvent *e) override;
    void resizeEvent(QResizeEvent *event) override;

private:
    void rebuildImages();
    void recalcLayout();
    int tileIndexAt(const QPoint & pos) const;

    QByteArray        theTileData;
    uint32_t          theRomBaseOffset;
    GenesisPalette    thePalette;
    QVector<QImage>   theDecodedTiles;
    int               theZoom;
    int               theSelectedTile;

    // Layout cache
    int               theTileDisplaySize; // pixels per tile (8 * zoom)
    int               theTilesPerRow;
    int               theTotalRows;
};

#endif // RAWTILEBROWSERWIDGET_H
