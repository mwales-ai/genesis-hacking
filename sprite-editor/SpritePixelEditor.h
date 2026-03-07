#ifndef SPRITEPIXELEDITOR_H
#define SPRITEPIXELEDITOR_H

#include <QWidget>
#include <QImage>
#include <QByteArray>
#include "TileDecoder.h"

/**
 * Pixel-level editor for a single Genesis sprite.
 * Displays sprite tile data at high zoom with pixel grid,
 * supports mouse painting with 4bpp palette indices.
 */
class SpritePixelEditor : public QWidget
{
    Q_OBJECT
public:
    explicit SpritePixelEditor(QWidget *parent = nullptr);

    /** Load a sprite for editing. tileData is raw 4bpp Genesis tile bytes. */
    void loadSprite(const QByteArray & tileData, int widthTiles, int heightTiles,
                    const GenesisPalette & palette, bool hFlip, bool vFlip);

    /** Update palette without changing tile data (for live color editing). */
    void updatePalette(const GenesisPalette & palette);

    /** Set the pen color (palette index 0-15). */
    void setPenIndex(int paletteIndex);
    int penIndex() const { return thePenIndex; }

    /** Get the modified raw tile data (for writing back to ROM). */
    QByteArray modifiedTileData() const { return theTileData; }

    /** True if any pixel has been painted since load. */
    bool isModified() const { return theModified; }

    void setZoom(int factor);
    int zoom() const { return theZoom; }

    void setShowGrid(bool show);
    bool showGrid() const { return theShowGrid; }

    /** Clear the editor (no sprite loaded). */
    void clearSprite();

    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;

signals:
    void pixelPainted(int x, int y, int paletteIndex);

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;

private:
    /** Rebuild the display image from tile data + palette. */
    void rebuildDisplayImage();

    /** Map a display pixel to nibble offset in tile data, accounting for flip. */
    int getNibbleAt(int px, int py) const;

    /** Write a palette index nibble at display pixel (px, py). */
    void setNibbleAt(int px, int py, int paletteIndex);

    /** Paint a pixel at widget position, returns true if anything changed. */
    bool paintPixelAt(const QPoint & widgetPos);

    QByteArray       theTileData;
    GenesisPalette   thePalette;
    QImage           theDisplayImage;
    int              theWidthTiles;
    int              theHeightTiles;
    bool             theHFlip;
    bool             theVFlip;
    int              theZoom;
    int              thePenIndex;
    bool             theShowGrid;
    bool             theModified;
    bool             thePainting;     // mouse button held for drag painting
    bool             theHasSprite;
};

#endif // SPRITEPIXELEDITOR_H
