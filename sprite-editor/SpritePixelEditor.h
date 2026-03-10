#ifndef SPRITEPIXELEDITOR_H
#define SPRITEPIXELEDITOR_H

#include <QWidget>
#include <QImage>
#include <QByteArray>
#include <QVector>
#include "TileDecoder.h"

/**
 * Describes a single sprite in a multi-sprite composite group.
 */
struct EditorSprite
{
    QByteArray tileData;
    int widthTiles;
    int heightTiles;
    int x, y;           // position within composite (0-based after normalization)
    bool hFlip, vFlip;
    int paletteLine;     // 0-3
    QString romOffset;   // hex string for saving back to ROM
};

/**
 * Pixel-level editor for Genesis sprites.
 * Supports single-sprite mode (original) and multi-sprite group mode
 * where multiple hardware sprites are composited and painted across.
 */
class SpritePixelEditor : public QWidget
{
    Q_OBJECT
public:
    explicit SpritePixelEditor(QWidget *parent = nullptr);

    /** Load a single sprite for editing. tileData is raw 4bpp Genesis tile bytes. */
    void loadSprite(const QByteArray & tileData, int widthTiles, int heightTiles,
                    const GenesisPalette & palette, bool hFlip, bool vFlip);

    /** Load a multi-sprite group for editing. */
    void loadSpriteGroup(const QVector<EditorSprite> & sprites,
                         const GenesisPalette palettes[4]);

    /** Update palette without changing tile data (for live color editing). */
    void updatePalette(const GenesisPalette & palette);

    /** Set the pen color (palette index 0-15). */
    void setPenIndex(int paletteIndex);
    int penIndex() const { return thePenIndex; }

    /** Get the modified raw tile data (for writing back to ROM). */
    QByteArray modifiedTileData() const { return theTileData; }

    /** True if any pixel has been painted since load. */
    bool isModified() const { return theModified; }

    /** Group mode accessors */
    bool isGroupMode() const { return theGroupMode; }
    int groupSpriteCount() const { return theGroupSprites.size(); }
    QByteArray modifiedGroupTileData(int spriteIndex) const;
    const EditorSprite & groupSprite(int index) const { return theGroupSprites[index]; }

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

    // Group mode helpers
    void rebuildGroupDisplayImage();
    int findSpriteAtPixel(int px, int py) const;
    void setGroupNibbleAt(int px, int py, int paletteIndex);
    bool paintGroupPixelAt(const QPoint & widgetPos);

    // Single-sprite state
    QByteArray       theTileData;
    GenesisPalette   thePalette;
    int              theWidthTiles;
    int              theHeightTiles;
    bool             theHFlip;
    bool             theVFlip;

    // Group mode state
    QVector<EditorSprite> theGroupSprites;
    GenesisPalette   theGroupPalettes[4];
    bool             theGroupMode;

    // Shared state
    QImage           theDisplayImage;
    int              theZoom;
    int              thePenIndex;
    bool             theShowGrid;
    bool             theModified;
    bool             thePainting;     // mouse button held for drag painting
    bool             theHasSprite;
    int              theCompW;        // composite pixel dimensions (group mode)
    int              theCompH;
};

#endif // SPRITEPIXELEDITOR_H
