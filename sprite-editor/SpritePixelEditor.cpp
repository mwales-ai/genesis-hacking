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
    , theZoom(8)
    , thePenIndex(1)
    , theShowGrid(true)
    , theModified(false)
    , thePainting(false)
    , theHasSprite(false)
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

void SpritePixelEditor::updatePalette(const GenesisPalette & palette)
{
    thePalette = palette;
    if (theHasSprite)
    {
        rebuildDisplayImage();
        update();
    }
}

void SpritePixelEditor::setPenIndex(int paletteIndex)
{
    thePenIndex = qBound(0, paletteIndex, 15);
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
    theTileData.clear();
    theDisplayImage = QImage();
    theModified = false;
    updateGeometry();
    update();
}

QSize SpritePixelEditor::sizeHint() const
{
    if (!theHasSprite)
        return QSize(256, 256);
    return QSize(theWidthTiles * 8 * theZoom,
                 theHeightTiles * 8 * theZoom);
}

QSize SpritePixelEditor::minimumSizeHint() const
{
    if (!theHasSprite)
        return QSize(128, 128);
    return QSize(theWidthTiles * 8 * 2,
                 theHeightTiles * 8 * 2);
}

// ---------------------------------------------------------------------------
// 4bpp nibble access — the core of the pixel editor
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

    // Tile grid (every 8 pixels)
    if (theShowGrid && theZoom >= 2)
    {
        p.setPen(QPen(QColor(255, 255, 0, 100), 1));
        for (int x = 0; x <= theWidthTiles; ++x)
            p.drawLine(x * 8 * theZoom, 0, x * 8 * theZoom, scaledH);
        for (int y = 0; y <= theHeightTiles; ++y)
            p.drawLine(0, y * 8 * theZoom, scaledW, y * 8 * theZoom);
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
        paintPixelAt(event->pos());
    }
}

void SpritePixelEditor::mouseMoveEvent(QMouseEvent *event)
{
    if (thePainting && (event->buttons() & Qt::LeftButton))
        paintPixelAt(event->pos());
}

void SpritePixelEditor::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton)
        thePainting = false;
}
