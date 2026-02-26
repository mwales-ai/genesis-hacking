#include "RawTileBrowserWidget.h"
#include <QPainter>
#include <QMouseEvent>
#include <QToolTip>
#include <QEvent>
#include <QHelpEvent>

RawTileBrowserWidget::RawTileBrowserWidget(QWidget *parent)
    : QWidget(parent)
    , theRomBaseOffset(0)
    , theZoom(4)
    , theSelectedTile(-1)
    , theTileDisplaySize(32)
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
    theSelectedTile  = -1;
    rebuildImages();
    recalcLayout();
    updateGeometry();
    update();
}

void RawTileBrowserWidget::clearTiles()
{
    theTileData.clear();
    theDecodedTiles.clear();
    theSelectedTile = -1;
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
    theTileDisplaySize = 8 * theZoom;
    recalcLayout();
    updateGeometry();
    update();
}

int RawTileBrowserWidget::zoom() const
{
    return theZoom;
}

void RawTileBrowserWidget::rebuildImages()
{
    int numTiles = theTileData.size() / 32;
    theDecodedTiles.resize(numTiles);
    for (int i = 0; i < numTiles; ++i)
        theDecodedTiles[i] = TileDecoder::decodeTile(theTileData, i * 32, thePalette);
}

void RawTileBrowserWidget::recalcLayout()
{
    theTileDisplaySize = 8 * theZoom + 2; // +2 for 1px border each side
    int availWidth = qMax(width(), 200);
    theTilesPerRow = qMax(1, availWidth / theTileDisplaySize);
    int numTiles   = theDecodedTiles.size();
    theTotalRows   = numTiles > 0 ? (numTiles + theTilesPerRow - 1) / theTilesPerRow : 0;
    setMinimumHeight(theTotalRows * theTileDisplaySize);
}

QSize RawTileBrowserWidget::sizeHint() const
{
    return QSize(width(), theTotalRows * theTileDisplaySize);
}

QSize RawTileBrowserWidget::minimumSizeHint() const
{
    return QSize(theTileDisplaySize * 4, theTileDisplaySize * 2);
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

    int cellSize = theTileDisplaySize;
    int imgSize  = 8 * theZoom;

    for (int i = 0; i < theDecodedTiles.size(); ++i)
    {
        int col = i % theTilesPerRow;
        int row = i / theTilesPerRow;
        int x   = col * cellSize;
        int y   = row * cellSize;

        // Selection highlight
        if (i == theSelectedTile)
            p.fillRect(x, y, cellSize, cellSize, QColor(0, 80, 160));

        QImage scaled = theDecodedTiles[i].scaled(
            imgSize, imgSize,
            Qt::IgnoreAspectRatio,
            Qt::FastTransformation);

        p.drawImage(x + 1, y + 1, scaled);

        // Faint border
        p.setPen(QColor(60, 60, 60));
        p.drawRect(x, y, cellSize - 1, cellSize - 1);
    }
}

void RawTileBrowserWidget::mousePressEvent(QMouseEvent *event)
{
    int idx = tileIndexAt(event->pos());
    if (idx >= 0 && idx < theDecodedTiles.size())
    {
        theSelectedTile = idx;
        update();
        uint32_t romOff = theRomBaseOffset + (uint32_t)(idx * 32);
        emit tileClicked(idx, romOff);
    }
}

bool RawTileBrowserWidget::event(QEvent *e)
{
    if (e->type() == QEvent::ToolTip)
    {
        QHelpEvent *he = static_cast<QHelpEvent*>(e);
        int idx = tileIndexAt(he->pos());
        if (idx >= 0 && idx < theDecodedTiles.size())
        {
            uint32_t romOff = theRomBaseOffset + (uint32_t)(idx * 32);
            QString tip = QString("Tile %1  |  ROM 0x%2")
                .arg(idx)
                .arg(romOff, 6, 16, QChar('0')).toUpper();
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

int RawTileBrowserWidget::tileIndexAt(const QPoint & pos) const
{
    if (theTileDisplaySize <= 0) return -1;
    int col = pos.x() / theTileDisplaySize;
    int row = pos.y() / theTileDisplaySize;
    int idx = row * theTilesPerRow + col;
    if (idx >= 0 && idx < theDecodedTiles.size())
        return idx;
    return -1;
}
