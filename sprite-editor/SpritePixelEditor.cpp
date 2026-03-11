#include "SpritePixelEditor.h"
#include <QPainter>
#include <QMouseEvent>
#include <iostream>

#define PxlDebug if(0) std::cout

SpritePixelEditor::SpritePixelEditor(QWidget *parent)
    : QWidget(parent)
    , theWidthTiles(0)
    , theHeightTiles(0)
    , theHFlip(false)
    , theVFlip(false)
    , theGroupMode(false)
    , theActiveGroupPaletteLine(0)
    , theZoom(8)
    , thePenIndex(1)
    , theShowGrid(true)
    , theModified(false)
    , thePainting(false)
    , theHasSprite(false)
    , theCompW(0)
    , theCompH(0)
{
    setMouseTracking(true);
    setCursor(Qt::CrossCursor);
}

void SpritePixelEditor::loadSprite(const QByteArray & tileData, int widthTiles,
                                   int heightTiles, const GenesisPalette & palette,
                                   bool hFlip, bool vFlip)
{
    theTileData = tileData;
    theWidthTiles = widthTiles;
    theHeightTiles = heightTiles;
    thePalette = palette;
    theHFlip = hFlip;
    theVFlip = vFlip;
    theModified = false;
    theHasSprite = true;
    theGroupMode = false;
    theGroupSprites.clear();

    int expectedBytes = widthTiles * heightTiles * 32;
    if (theTileData.size() < expectedBytes)
        theTileData.append(QByteArray(expectedBytes - theTileData.size(), '\0'));

    rebuildDisplayImage();
    resize(sizeHint());
    updateGeometry();
    update();

    PxlDebug << "SpritePixelEditor: loaded " << widthTiles << "x" << heightTiles
             << " tiles, " << tileData.size() << " bytes, flip H:" << hFlip
             << " V:" << vFlip << std::endl;
}

void SpritePixelEditor::loadSpriteGroup(const QVector<EditorSprite> & sprites,
                                        const GenesisPalette palettes[4])
{
    theGroupSprites = sprites;
    for (int i = 0; i < 4; ++i)
        theGroupPalettes[i] = palettes[i];
    theGroupMode = true;
    theActiveGroupPaletteLine = 0;
    theModified = false;
    theHasSprite = true;

    // Ensure tile data is correctly sized
    for (int i = 0; i < theGroupSprites.size(); ++i)
    {
        EditorSprite & es = theGroupSprites[i];
        int expectedBytes = es.widthTiles * es.heightTiles * 32;
        if (es.tileData.size() < expectedBytes)
            es.tileData.append(QByteArray(expectedBytes - es.tileData.size(), '\0'));
    }

    rebuildGroupDisplayImage();
    resize(sizeHint());
    updateGeometry();
    update();

    PxlDebug << "SpritePixelEditor: loaded group of " << sprites.size()
             << " sprites" << std::endl;
}

void SpritePixelEditor::updatePalette(const GenesisPalette & palette)
{
    thePalette = palette;
    if (theHasSprite)
    {
        if (theGroupMode)
            rebuildGroupDisplayImage();
        else
            rebuildDisplayImage();
        update();
    }
}

void SpritePixelEditor::updateGroupPalette(int paletteLine, const GenesisPalette & palette)
{
    int line = qBound(0, paletteLine, 3);
    theGroupPalettes[line] = palette;
    if (theHasSprite && theGroupMode)
    {
        rebuildGroupDisplayImage();
        update();
    }
}

void SpritePixelEditor::setPenIndex(int paletteIndex)
{
    thePenIndex = qBound(0, paletteIndex, 15);
}

QByteArray SpritePixelEditor::modifiedGroupTileData(int spriteIndex) const
{
    if (spriteIndex >= 0 && spriteIndex < theGroupSprites.size())
        return theGroupSprites[spriteIndex].tileData;
    return QByteArray();
}

void SpritePixelEditor::setZoom(int factor)
{
    if (factor < 1) factor = 1;
    if (factor > 32) factor = 32;
    theZoom = factor;
    resize(sizeHint());
    updateGeometry();
    update();
}

void SpritePixelEditor::setShowGrid(bool show)
{
    theShowGrid = show;
    update();
}

void SpritePixelEditor::clearSprite()
{
    theHasSprite = false;
    theGroupMode = false;
    theTileData.clear();
    theGroupSprites.clear();
    theDisplayImage = QImage();
    theModified = false;
    theCompW = 0;
    theCompH = 0;
    updateGeometry();
    update();
}

QSize SpritePixelEditor::sizeHint() const
{
    if (!theHasSprite)
        return QSize(256, 256);
    if (theGroupMode)
        return QSize(theCompW * theZoom, theCompH * theZoom);
    return QSize(theWidthTiles * 8 * theZoom,
                 theHeightTiles * 8 * theZoom);
}

QSize SpritePixelEditor::minimumSizeHint() const
{
    if (!theHasSprite)
        return QSize(128, 128);
    if (theGroupMode)
        return QSize(theCompW * 2, theCompH * 2);
    return QSize(theWidthTiles * 8 * 2,
                 theHeightTiles * 8 * 2);
}

// ---------------------------------------------------------------------------
// 4bpp nibble access — the core of the pixel editor (single sprite mode)
// ---------------------------------------------------------------------------

int SpritePixelEditor::getNibbleAt(int px, int py) const
{
    if (!theHasSprite)
        return 0;

    int totalW = theWidthTiles * 8;
    int totalH = theHeightTiles * 8;

    // Reverse the flip so we go from display coords back to raw tile coords
    int rawX = theHFlip ? (totalW - 1 - px) : px;
    int rawY = theVFlip ? (totalH - 1 - py) : py;

    if (rawX < 0 || rawX >= totalW || rawY < 0 || rawY >= totalH)
        return 0;

    // Column-major tile order: tileIdx = col * heightTiles + row
    int tileCol = rawX / 8;
    int tileRow = rawY / 8;
    int tileIdx = tileCol * theHeightTiles + tileRow;

    int localX = rawX % 8;
    int localY = rawY % 8;

    // Each tile is 32 bytes.  Each row is 4 bytes (8 pixels x 4 bits).
    // High nibble = even pixel, low nibble = odd pixel.
    int byteOffset = tileIdx * 32 + localY * 4 + localX / 2;

    if (byteOffset < 0 || byteOffset >= theTileData.size())
        return 0;

    uint8_t byte = (uint8_t)theTileData[byteOffset];
    if (localX % 2 == 0)
        return (byte >> 4) & 0x0F;  // high nibble
    else
        return byte & 0x0F;         // low nibble
}

void SpritePixelEditor::setNibbleAt(int px, int py, int paletteIndex)
{
    if (!theHasSprite)
        return;

    int totalW = theWidthTiles * 8;
    int totalH = theHeightTiles * 8;

    int rawX = theHFlip ? (totalW - 1 - px) : px;
    int rawY = theVFlip ? (totalH - 1 - py) : py;

    if (rawX < 0 || rawX >= totalW || rawY < 0 || rawY >= totalH)
        return;

    int tileCol = rawX / 8;
    int tileRow = rawY / 8;
    int tileIdx = tileCol * theHeightTiles + tileRow;

    int localX = rawX % 8;
    int localY = rawY % 8;

    int byteOffset = tileIdx * 32 + localY * 4 + localX / 2;
    if (byteOffset < 0 || byteOffset >= theTileData.size())
        return;

    uint8_t byte = (uint8_t)theTileData[byteOffset];
    int idx = qBound(0, paletteIndex, 15);

    if (localX % 2 == 0)
        byte = (byte & 0x0F) | (idx << 4);  // set high nibble
    else
        byte = (byte & 0xF0) | idx;         // set low nibble

    theTileData[byteOffset] = (char)byte;
}

// ---------------------------------------------------------------------------
// Group mode helpers
// ---------------------------------------------------------------------------

void SpritePixelEditor::rebuildGroupDisplayImage()
{
    if (!theGroupMode || theGroupSprites.isEmpty())
        return;

    // Calculate composite dimensions from all sprites
    int maxX = 0, maxY = 0;
    for (const EditorSprite & es : theGroupSprites)
    {
        int right  = es.x + es.widthTiles * 8;
        int bottom = es.y + es.heightTiles * 8;
        if (right > maxX)  maxX = right;
        if (bottom > maxY) maxY = bottom;
    }

    theCompW = maxX;
    theCompH = maxY;
    if (theCompW <= 0 || theCompH <= 0) return;

    theDisplayImage = QImage(theCompW, theCompH, QImage::Format_ARGB32);
    theDisplayImage.fill(Qt::transparent);

    // Render sprites in reverse order (last = background, first = foreground)
    for (int si = theGroupSprites.size() - 1; si >= 0; --si)
    {
        const EditorSprite & es = theGroupSprites[si];
        int palLine = qBound(0, es.paletteLine, 3);
        const GenesisPalette & pal = theGroupPalettes[palLine];

        for (int col = 0; col < es.widthTiles; ++col)
        {
            for (int row = 0; row < es.heightTiles; ++row)
            {
                int tileIdx = col * es.heightTiles + row;
                int tileOffset = tileIdx * 32;
                if (tileOffset + 32 > es.tileData.size())
                    continue;

                QImage tile = TileDecoder::decodeTileFlipped(
                    es.tileData, tileOffset, pal, es.hFlip, es.vFlip);

                int tileX, tileY;
                if (es.hFlip)
                    tileX = (es.widthTiles - 1 - col) * 8;
                else
                    tileX = col * 8;
                if (es.vFlip)
                    tileY = (es.heightTiles - 1 - row) * 8;
                else
                    tileY = row * 8;

                int dx = es.x + tileX;
                int dy = es.y + tileY;

                for (int ty = 0; ty < 8; ++ty)
                {
                    for (int tx = 0; tx < 8; ++tx)
                    {
                        int px = dx + tx;
                        int py = dy + ty;
                        if (px < 0 || px >= theCompW || py < 0 || py >= theCompH)
                            continue;
                        QRgb pixel = tile.pixel(tx, ty);
                        if (qAlpha(pixel) > 0)
                            theDisplayImage.setPixel(px, py, pixel);
                    }
                }
            }
        }
    }
}

int SpritePixelEditor::findSpriteAtPixel(int px, int py) const
{
    // Check sprites in order (first = foreground priority)
    for (int si = 0; si < theGroupSprites.size(); ++si)
    {
        const EditorSprite & es = theGroupSprites[si];
        int sw = es.widthTiles * 8;
        int sh = es.heightTiles * 8;

        if (px >= es.x && px < es.x + sw &&
            py >= es.y && py < es.y + sh)
        {
            return si;
        }
    }
    return -1;
}

void SpritePixelEditor::setGroupNibbleAt(int px, int py, int paletteIndex)
{
    int si = findSpriteAtPixel(px, py);
    if (si < 0)
        return;

    EditorSprite & es = theGroupSprites[si];

    // Convert composite coords to local sprite coords
    int localX = px - es.x;
    int localY = py - es.y;

    int totalW = es.widthTiles * 8;
    int totalH = es.heightTiles * 8;

    // Account for flip
    int rawX = es.hFlip ? (totalW - 1 - localX) : localX;
    int rawY = es.vFlip ? (totalH - 1 - localY) : localY;

    if (rawX < 0 || rawX >= totalW || rawY < 0 || rawY >= totalH)
        return;

    int tileCol = rawX / 8;
    int tileRow = rawY / 8;
    int tileIdx = tileCol * es.heightTiles + tileRow;

    int lx = rawX % 8;
    int ly = rawY % 8;

    int byteOffset = tileIdx * 32 + ly * 4 + lx / 2;
    if (byteOffset < 0 || byteOffset >= es.tileData.size())
        return;

    uint8_t byte = (uint8_t)es.tileData[byteOffset];
    int idx = qBound(0, paletteIndex, 15);

    if (lx % 2 == 0)
        byte = (byte & 0x0F) | (idx << 4);
    else
        byte = (byte & 0xF0) | idx;

    es.tileData[byteOffset] = (char)byte;
}

bool SpritePixelEditor::paintGroupPixelAt(const QPoint & widgetPos)
{
    if (!theHasSprite || !theGroupMode)
        return false;

    int px = widgetPos.x() / theZoom;
    int py = widgetPos.y() / theZoom;

    if (px < 0 || px >= theCompW || py < 0 || py >= theCompH)
        return false;

    setGroupNibbleAt(px, py, thePenIndex);
    theModified = true;
    rebuildGroupDisplayImage();
    update();

    emit pixelPainted(px, py, thePenIndex);
    return true;
}

// ---------------------------------------------------------------------------
// Rendering
// ---------------------------------------------------------------------------

void SpritePixelEditor::rebuildDisplayImage()
{
    if (!theHasSprite || theTileData.isEmpty())
        return;

    // Decode using TileDecoder::decodeSprite (column-major, no flip)
    QImage raw = TileDecoder::decodeSprite(theTileData,
                                           theWidthTiles, theHeightTiles,
                                           thePalette);

    // Apply flip to match what the hardware would show
    if (theHFlip || theVFlip)
        theDisplayImage = raw.mirrored(theHFlip, theVFlip);
    else
        theDisplayImage = raw;
}

void SpritePixelEditor::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    QPainter p(this);

    if (!theHasSprite || theDisplayImage.isNull())
    {
        p.fillRect(rect(), Qt::darkGray);
        p.setPen(Qt::white);
        p.drawText(rect(), Qt::AlignCenter, "No sprite loaded\nClick a sprite in the collection");
        return;
    }

    int pixW = theDisplayImage.width();
    int pixH = theDisplayImage.height();
    int scaledW = pixW * theZoom;
    int scaledH = pixH * theZoom;

    // Checkerboard background
    int checkSize = theZoom;
    if (checkSize < 4) checkSize = 4;
    for (int cy = 0; cy < scaledH; cy += checkSize)
    {
        for (int cx = 0; cx < scaledW; cx += checkSize)
        {
            bool dark = ((cx / checkSize) + (cy / checkSize)) % 2;
            p.fillRect(cx, cy, checkSize, checkSize,
                       dark ? QColor(180, 180, 180) : QColor(220, 220, 220));
        }
    }

    // Draw scaled image (nearest neighbor)
    QImage scaled = theDisplayImage.scaled(scaledW, scaledH,
                                            Qt::IgnoreAspectRatio,
                                            Qt::FastTransformation);
    p.drawImage(0, 0, scaled);

    // Pixel grid
    if (theShowGrid && theZoom >= 4)
    {
        p.setPen(QPen(QColor(0, 0, 0, 60), 1));
        for (int x = 0; x <= pixW; ++x)
            p.drawLine(x * theZoom, 0, x * theZoom, scaledH);
        for (int y = 0; y <= pixH; ++y)
            p.drawLine(0, y * theZoom, scaledW, y * theZoom);
    }

    // Tile grid (every 8 pixels) — single sprite mode
    if (theShowGrid && theZoom >= 2 && !theGroupMode)
    {
        p.setPen(QPen(QColor(255, 255, 0, 100), 1));
        for (int x = 0; x <= theWidthTiles; ++x)
            p.drawLine(x * 8 * theZoom, 0, x * 8 * theZoom, scaledH);
        for (int y = 0; y <= theHeightTiles; ++y)
            p.drawLine(0, y * 8 * theZoom, scaledW, y * 8 * theZoom);
    }

    // Sprite outlines in group mode — color-coded by palette line
    if (theShowGrid && theZoom >= 2 && theGroupMode)
    {
        // Palette line colors
        static const QColor palLineColors[4] = {
            QColor(255, 200,   0),   // line 0: yellow
            QColor(  0, 200, 255),   // line 1: cyan
            QColor(255, 100, 255),   // line 2: magenta
            QColor(100, 255, 100)    // line 3: green
        };

        for (const EditorSprite & es : theGroupSprites)
        {
            int rx = es.x * theZoom;
            int ry = es.y * theZoom;
            int rw = es.widthTiles * 8 * theZoom;
            int rh = es.heightTiles * 8 * theZoom;

            int line = qBound(0, es.paletteLine, 3);
            QColor lineColor = palLineColors[line];

            if (line == theActiveGroupPaletteLine)
            {
                // Active palette line: solid, 2px
                lineColor.setAlpha(200);
                p.setPen(QPen(lineColor, 2, Qt::SolidLine));
            }
            else
            {
                // Other lines: dashed, 1px
                lineColor.setAlpha(120);
                p.setPen(QPen(lineColor, 1, Qt::DashLine));
            }
            p.drawRect(rx, ry, rw - 1, rh - 1);
        }
    }
}

// ---------------------------------------------------------------------------
// Mouse painting
// ---------------------------------------------------------------------------

bool SpritePixelEditor::paintPixelAt(const QPoint & widgetPos)
{
    if (!theHasSprite)
        return false;

    int px = widgetPos.x() / theZoom;
    int py = widgetPos.y() / theZoom;

    int totalW = theWidthTiles * 8;
    int totalH = theHeightTiles * 8;

    if (px < 0 || px >= totalW || py < 0 || py >= totalH)
        return false;

    int oldVal = getNibbleAt(px, py);
    if (oldVal == thePenIndex)
        return false;

    setNibbleAt(px, py, thePenIndex);
    theModified = true;
    rebuildDisplayImage();
    update();

    emit pixelPainted(px, py, thePenIndex);
    return true;
}

void SpritePixelEditor::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton && theHasSprite)
    {
        thePainting = true;
        if (theGroupMode)
            paintGroupPixelAt(event->pos());
        else
            paintPixelAt(event->pos());
    }
    else if (event->button() == Qt::MiddleButton && theHasSprite && theGroupMode)
    {
        // Middle-click: switch active palette line to the clicked sprite's line
        int px = event->pos().x() / theZoom;
        int py = event->pos().y() / theZoom;
        int sprIdx = findSpriteAtPixel(px, py);
        if (sprIdx >= 0)
        {
            int newLine = theGroupSprites[sprIdx].paletteLine;
            if (newLine != theActiveGroupPaletteLine)
            {
                theActiveGroupPaletteLine = newLine;
                update();   // repaint to update outline styling
                emit groupPaletteLineChanged(newLine);
            }
        }
    }
}

void SpritePixelEditor::mouseMoveEvent(QMouseEvent *event)
{
    if (thePainting && (event->buttons() & Qt::LeftButton))
    {
        if (theGroupMode)
            paintGroupPixelAt(event->pos());
        else
            paintPixelAt(event->pos());
    }
}

void SpritePixelEditor::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton)
        thePainting = false;
}
