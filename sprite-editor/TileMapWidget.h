#ifndef TILEMAPWIDGET_H
#define TILEMAPWIDGET_H

#include <QWidget>
#include <QImage>
#include <QSet>
#include "GameDefinition.h"
#include "TileDecoder.h"
#include "RomFile.h"
#include "SpritePixelEditor.h"  // for EditorTool enum

class TileMapWidget : public QWidget
{
    Q_OBJECT
public:
    explicit TileMapWidget(QWidget *parent = nullptr);

    /** Load a screen capture for display. romFile may be null if all tiles are embedded. */
    void setScreenCapture(const ScreenCapture & capture, RomFile *romFile);

    void clearCapture();

    void setZoom(int factor);
    int zoom() const;

    /** Edit mode: enable pixel-level tile painting */
    void setEditMode(bool enabled);
    bool editMode() const { return theEditMode; }

    /** Set the active painting tool */
    void setTool(EditorTool tool);
    EditorTool currentTool() const { return theTool; }

    /** Set pen color index (0-15) and brush size */
    void setPenIndex(int index);
    int penIndex() const { return thePenIndex; }

    void setBrushSize(int size);
    int brushSize() const { return theBrushSize; }

    /** Set the active palette line for painting (0-3) */
    void setActivePaletteLine(int line);

    /** Get the palette for a specific line */
    const GenesisPalette & palette(int line) const { return thePalettes[qBound(0, line, 3)]; }

    /** True if any tile has been modified since load */
    bool isModified() const { return theModified; }

    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;

signals:
    void colorPicked(int paletteIndex);

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;

private:
    void rebuildImage();
    QByteArray tileDataForEntry(const TileMapEntry & entry) const;

    // Edit mode helpers
    void paintPixelAt(int px, int py);
    void brushPaintAt(int px, int py);
    void bucketFillAt(int px, int py);
    void eyeDropAt(int px, int py);
    int getPixelPaletteIndex(int px, int py) const;
    int tileIndexAtPixel(int px, int py) const;
    QSet<int> sharedTileIndices(int tileIdx) const;
    void setTileNibble(int tileIdx, int localX, int localY, int palIndex);

    ScreenCapture        theCapture;
    GenesisPalette       thePalettes[4];  // decoded palette for each line
    RomFile             *theRomFile;
    QImage               theFullImage;    // pre-rendered full image
    int                  theZoom;
    bool                 theHasCapture;

    // Edit mode state
    bool                 theEditMode;
    bool                 thePainting;
    int                  thePenIndex;
    int                  theBrushSize;
    int                  theActivePaletteLine;
    EditorTool           theTool;
    bool                 theModified;
    int                  theHoverTileIdx;  // tile under cursor (-1 if none)
    QSet<int>            theHighlightedTiles;  // tiles sharing same ROM offset as hover
};

#endif // TILEMAPWIDGET_H
