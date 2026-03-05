#include "TileMapWidget.h"
#include <QPainter>
#include <QMouseEvent>
#include <QToolTip>
#include <iostream>

#define TileMapDebug if(0) std::cout

TileMapWidget::TileMapWidget(QWidget *parent)
    : QWidget(parent)
    , theRomFile(nullptr)
    , theZoom(2)
    , theHasCapture(false)
{
    setMouseTracking(true);
}

void TileMapWidget::setScreenCapture(const ScreenCapture & capture, RomFile *romFile)
{
    theCapture = capture;
    theRomFile = romFile;
    theHasCapture = true;

    // Decode the 4 palettes from CRAM values
    for (int line = 0; line < 4; ++line)
    {
        if (line < capture.palettes.size())
            thePalettes[line] = TileDecoder::decodePaletteFromCram(capture.palettes[line].cramValues);
        else
            thePalettes[line] = TileDecoder::greyPalette();
    }

    rebuildImage();
    resize(sizeHint());
    updateGeometry();
    update();
}

void TileMapWidget::clearCapture()
{
    theHasCapture = false;
    theFullImage = QImage();
    updateGeometry();
    update();
}

void TileMapWidget::setZoom(int factor)
{
    if (factor < 1) factor = 1;
    if (factor > 8) factor = 8;
    theZoom = factor;
    resize(sizeHint());
    updateGeometry();
    update();
}

int TileMapWidget::zoom() const
{
    return theZoom;
}

QSize TileMapWidget::sizeHint() const
{
    if (!theHasCapture)
        return QSize(320, 224);
    return QSize(theCapture.widthTiles * 8 * theZoom,
                 theCapture.heightTiles * 8 * theZoom);
}

QSize TileMapWidget::minimumSizeHint() const
{
    if (!theHasCapture)
        return QSize(160, 112);
    return QSize(theCapture.widthTiles * 8,
                 theCapture.heightTiles * 8);
}

QByteArray TileMapWidget::tileDataForEntry(const TileMapEntry & entry) const
{
    // For ROM-sourced tiles, read 32 bytes from ROM at the given offset
    if (!entry.romOffset.isEmpty() && theRomFile && theRomFile->isOpen())
    {
        bool ok = false;
        uint32_t offset = entry.romOffset.mid(2).toUInt(&ok, 16);  // strip "0x"
        if (ok)
            return theRomFile->readBytes(offset, 32);
    }

    // For embedded tiles, look up by VRAM address
    if (entry.source == "embedded")
    {
        QString vramKey = QString("0x%1").arg(entry.pattern * 32, 0, 16).toUpper();
        // Try various formatting since the key format may vary
        if (theCapture.embeddedTiles.contains(vramKey))
            return theCapture.embeddedTiles[vramKey];

        // Also try without leading zeros
        vramKey = QString("0x%1").arg(entry.pattern * 32, 0, 16, QChar('0')).toUpper();
        if (theCapture.embeddedTiles.contains(vramKey))
            return theCapture.embeddedTiles[vramKey];

        // Try all keys to find the matching VRAM address
        uint32_t targetAddr = entry.pattern * 32;
        for (auto it = theCapture.embeddedTiles.begin(); it != theCapture.embeddedTiles.end(); ++it)
        {
            bool ok = false;
            uint32_t addr = it.key().mid(2).toUInt(&ok, 16);
            if (ok && addr == targetAddr)
                return it.value();
        }
    }

    // Blank tile: return 32 zero bytes
    return QByteArray(32, '\0');
}

void TileMapWidget::rebuildImage()
{
    if (!theHasCapture)
        return;

    int pixW = theCapture.widthTiles * 8;
    int pixH = theCapture.heightTiles * 8;
    theFullImage = QImage(pixW, pixH, QImage::Format_ARGB32);
    theFullImage.fill(Qt::black);

    for (const TileMapEntry & entry : theCapture.tileMap)
    {
        if (entry.row >= theCapture.heightTiles || entry.col >= theCapture.widthTiles)
            continue;

        int palLine = entry.paletteLine;
        if (palLine < 0 || palLine > 3) palLine = 0;

        QByteArray tileBytes = tileDataForEntry(entry);
        if (tileBytes.size() < 32)
            continue;

        QImage tile = TileDecoder::decodeTileFlipped(tileBytes, 0, thePalettes[palLine],
                                                     entry.hFlip, entry.vFlip);

        // Paint tile into the full image
        int dx = entry.col * 8;
        int dy = entry.row * 8;
        for (int ty = 0; ty < 8; ++ty)
        {
            for (int tx = 0; tx < 8; ++tx)
            {
                QRgb pixel = tile.pixel(tx, ty);
                if (qAlpha(pixel) > 0)
                    theFullImage.setPixel(dx + tx, dy + ty, pixel);
            }
        }
    }

    TileMapDebug << "TileMapWidget: rebuilt image " << pixW << "x" << pixH << std::endl;
}

void TileMapWidget::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    QPainter p(this);

    if (!theHasCapture || theFullImage.isNull())
    {
        p.fillRect(rect(), Qt::darkGray);
        p.setPen(Qt::white);
        p.drawText(rect(), Qt::AlignCenter, "No screen capture loaded");
        return;
    }

    QImage scaled = theFullImage.scaled(theFullImage.width() * theZoom,
                                        theFullImage.height() * theZoom,
                                        Qt::IgnoreAspectRatio,
                                        Qt::FastTransformation);
    p.drawImage(0, 0, scaled);
}

void TileMapWidget::mouseMoveEvent(QMouseEvent *event)
{
    if (!theHasCapture)
        return;

    int tileCol = event->pos().x() / (8 * theZoom);
    int tileRow = event->pos().y() / (8 * theZoom);

    if (tileCol < 0 || tileCol >= theCapture.widthTiles ||
        tileRow < 0 || tileRow >= theCapture.heightTiles)
    {
        QToolTip::hideText();
        return;
    }

    // Find the matching tile map entry
    int idx = tileRow * theCapture.widthTiles + tileCol;
    if (idx >= theCapture.tileMap.size())
    {
        QToolTip::hideText();
        return;
    }

    const TileMapEntry & entry = theCapture.tileMap[idx];
    QString tip = QString("Tile (%1, %2)\nPattern: %3\nPalette: %4\n"
                          "H-Flip: %5  V-Flip: %6\nPriority: %7\n"
                          "ROM Offset: %8\nSource: %9")
        .arg(entry.col).arg(entry.row)
        .arg(entry.pattern)
        .arg(entry.paletteLine)
        .arg(entry.hFlip ? "yes" : "no")
        .arg(entry.vFlip ? "yes" : "no")
        .arg(entry.priority ? "yes" : "no")
        .arg(entry.romOffset.isEmpty() ? "N/A" : entry.romOffset)
        .arg(entry.source);

    QToolTip::showText(event->globalPosition().toPoint(), tip, this);
}
