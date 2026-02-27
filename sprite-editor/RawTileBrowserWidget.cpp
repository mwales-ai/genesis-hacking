#include "RawTileBrowserWidget.h"
#include <QPainter>
#include <QMouseEvent>
#include <QToolTip>
#include <QEvent>
#include <QHelpEvent>
#include <QScrollArea>

RawTileBrowserWidget::RawTileBrowserWidget(QWidget *parent)
    : QWidget(parent)
    , theRomBaseOffset(0)
    , theZoom(4)
    , theSelectedItem(-1)
    , theSpriteW(1)
    , theSpriteH(1)
    , theAssemblySkip(0)
    , theCellW(34)
    , theCellH(34)
    , theTilesPerRow(10)
    , theTotalRows(0)
{
    thePalette = TileDecoder::greyPalette();
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    setMouseTracking(true);
}

void RawTileBrowserWidget::setTileData(const QByteArray & rawData,
                                        uint32_t romBaseOffset,
                                        const GenesisPalette & palette)
{
    theTileData      = rawData;
    theRomBaseOffset = romBaseOffset;
    thePalette       = palette;
    theSelectedItem  = -1;
    theAssemblySkip  = 0;
    rebuildImages();
    recalcLayout();
    updateGeometry();
    update();
}

void RawTileBrowserWidget::clearTiles()
{
    theTileData.clear();
    theDecodedTiles.clear();
    theSelectedItem = -1;
    theTotalRows = 0;
    updateGeometry();
    update();
}

void RawTileBrowserWidget::setPalette(const GenesisPalette & palette)
{
    thePalette = palette;
    rebuildImages();
    update();
}

void RawTileBrowserWidget::setZoom(int factor)
{
    if (factor < 1) factor = 1;
    if (factor > 8) factor = 8;
    theZoom = factor;
    recalcLayout();
    updateGeometry();
    update();
}

int RawTileBrowserWidget::zoom() const
{
    return theZoom;
}

void RawTileBrowserWidget::setSpriteSize(int w, int h)
{
    theSpriteW = qMax(1, w);
    theSpriteH = qMax(1, h);
    rebuildImages();
    recalcLayout();
    updateGeometry();
    update();
}

void RawTileBrowserWidget::setAssemblyStart(int tileSkip)
{
    theAssemblySkip = qMax(0, tileSkip);
    rebuildImages();
    recalcLayout();
    updateGeometry();
    update();
}

void RawTileBrowserWidget::rebuildImages()
{
    int numRawTiles    = theTileData.size() / 32;
    int tilesPerSprite = theSpriteW * theSpriteH;
    int skip           = qMin(theAssemblySkip, numRawTiles);
    int usableTiles    = numRawTiles - skip;
    int numItems       = usableTiles / tilesPerSprite;

    theDecodedTiles.resize(numItems);

    if (tilesPerSprite == 1)
    {
        for (int i = 0; i < numItems; ++i)
            theDecodedTiles[i] = TileDecoder::decodeTile(theTileData, (skip + i) * 32, thePalette);
    }
    else
    {
        for (int s = 0; s < numItems; s++)
        {
            int firstTile = skip + s * tilesPerSprite;
            QImage sprite(theSpriteW * 8, theSpriteH * 8, QImage::Format_ARGB32);
            sprite.fill(0);
            QPainter painter(&sprite);
            // Genesis column-major tile order: tile at (col, row) = col * spriteH + row
            for (int col = 0; col < theSpriteW; col++)
            {
                for (int row = 0; row < theSpriteH; row++)
                {
                    int tileIdx = firstTile + col * theSpriteH + row;
                    QImage tile = TileDecoder::decodeTile(theTileData, tileIdx * 32, thePalette);
                    painter.drawImage(col * 8, row * 8, tile);
                }
            }
            theDecodedTiles[s] = sprite;
        }
    }
}

void RawTileBrowserWidget::recalcLayout()
{
    theCellW = theSpriteW * 8 * theZoom + 2; // +2 for 1px border each side
    theCellH = theSpriteH * 8 * theZoom + 2;
    int availWidth = qMax(width(), 200);
    theTilesPerRow = qMax(1, availWidth / theCellW);
    int numItems   = theDecodedTiles.size();
    theTotalRows   = numItems > 0 ? (numItems + theTilesPerRow - 1) / theTilesPerRow : 0;
    setMinimumHeight(theTotalRows * theCellH);
}

QSize RawTileBrowserWidget::sizeHint() const
{
    return QSize(width(), theTotalRows * theCellH);
}

QSize RawTileBrowserWidget::minimumSizeHint() const
{
    return QSize(theCellW * 4, theCellH * 2);
}

void RawTileBrowserWidget::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.fillRect(rect(), QColor(20, 20, 20));

    if (theDecodedTiles.isEmpty())
    {
        p.setPen(Qt::gray);
        p.drawText(rect(), Qt::AlignCenter, "Load a ROM and select a tile range");
        return;
    }

    int imgW = theSpriteW * 8 * theZoom;
    int imgH = theSpriteH * 8 * theZoom;

    for (int i = 0; i < theDecodedTiles.size(); ++i)
    {
        int col = i % theTilesPerRow;
        int row = i / theTilesPerRow;
        int x   = col * theCellW;
        int y   = row * theCellH;

        // Selection highlight
        if (i == theSelectedItem)
            p.fillRect(x, y, theCellW, theCellH, QColor(0, 80, 160));

        QImage scaled = theDecodedTiles[i].scaled(
            imgW, imgH,
            Qt::IgnoreAspectRatio,
            Qt::FastTransformation);

        p.drawImage(x + 1, y + 1, scaled);

        // Faint border
        p.setPen(QColor(60, 60, 60));
        p.drawRect(x, y, theCellW - 1, theCellH - 1);
    }
}

void RawTileBrowserWidget::mousePressEvent(QMouseEvent *event)
{
    int idx = itemIndexAt(event->pos());
    if (idx >= 0 && idx < theDecodedTiles.size())
    {
        theSelectedItem = idx;
        update();
        int tilesPerSprite = theSpriteW * theSpriteH;
        int firstTile      = theAssemblySkip + idx * tilesPerSprite;
        uint32_t romOff    = theRomBaseOffset + (uint32_t)(firstTile * 32);
        emit tileClicked(firstTile, romOff);
    }
}

bool RawTileBrowserWidget::event(QEvent *e)
{
    if (e->type() == QEvent::ToolTip)
    {
        QHelpEvent *he = static_cast<QHelpEvent*>(e);
        int idx = itemIndexAt(he->pos());
        if (idx >= 0 && idx < theDecodedTiles.size())
        {
            int tilesPerSprite = theSpriteW * theSpriteH;
            int firstTile      = theAssemblySkip + idx * tilesPerSprite;
            uint32_t romOff    = theRomBaseOffset + (uint32_t)(firstTile * 32);
            QString tip;
            if (tilesPerSprite == 1)
            {
                tip = QString("Tile %1  |  ROM 0x%2")
                    .arg(firstTile)
                    .arg(romOff, 6, 16, QChar('0')).toUpper();
            }
            else
            {
                tip = QString("Sprite %1  (tiles %2–%3)  |  ROM 0x%4")
                    .arg(idx)
                    .arg(firstTile)
                    .arg(firstTile + tilesPerSprite - 1)
                    .arg(romOff, 6, 16, QChar('0')).toUpper();
            }
            QToolTip::showText(he->globalPos(), tip, this);
        }
        else
        {
            QToolTip::hideText();
        }
        return true;
    }
    return QWidget::event(e);
}

void RawTileBrowserWidget::resizeEvent(QResizeEvent *)
{
    recalcLayout();
    updateGeometry();
}

int RawTileBrowserWidget::itemIndexAt(const QPoint & pos) const
{
    if (theCellW <= 0 || theCellH <= 0) return -1;
    int col = pos.x() / theCellW;
    int row = pos.y() / theCellH;
    int idx = row * theTilesPerRow + col;
    if (idx >= 0 && idx < theDecodedTiles.size())
        return idx;
    return -1;
}

void RawTileBrowserWidget::scrollToTile(int tileIndex)
{
    if (theCellW <= 0 || theCellH <= 0 || theTilesPerRow <= 0) return;

    // Convert raw tile index → assembled sprite/item index (accounting for assembly skip)
    int tilesPerSprite = theSpriteW * theSpriteH;
    int adjustedTile   = tileIndex - theAssemblySkip;
    if (adjustedTile < 0) adjustedTile = 0;
    int itemIndex      = adjustedTile / tilesPerSprite;

    if (itemIndex < 0 || itemIndex >= theDecodedTiles.size()) return;

    int row = itemIndex / theTilesPerRow;
    int col = itemIndex % theTilesPerRow;
    int x   = col * theCellW;
    int y   = row * theCellH;

    QWidget *w = parentWidget();
    while (w) {
        if (QScrollArea *sa = qobject_cast<QScrollArea*>(w)) {
            sa->ensureVisible(x, y, theCellW, theCellH);
            return;
        }
        w = w->parentWidget();
    }
}
