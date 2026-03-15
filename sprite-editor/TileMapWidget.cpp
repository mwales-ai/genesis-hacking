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
    , theEditMode(false)
    , thePainting(false)
    , thePenIndex(1)
    , theBrushSize(1)
    , theActivePaletteLine(0)
    , theTool(TOOL_PENCIL)
    , theModified(false)
    , theHoverTileIdx(-1)
{
    setMouseTracking(true);
}

void TileMapWidget::setScreenCapture(const ScreenCapture & capture, RomFile *romFile)
{
    theCapture = capture;
    theRomFile = romFile;
    theHasCapture = true;
    theModified = false;
    theHoverTileIdx = -1;
    theHighlightedTiles.clear();

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
    theHoverTileIdx = -1;
    theHighlightedTiles.clear();
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

void TileMapWidget::setEditMode(bool enabled)
{
    theEditMode = enabled;
    if (enabled)
        setCursor(Qt::CrossCursor);
    else
        setCursor(Qt::ArrowCursor);
    update();
}

void TileMapWidget::setTool(EditorTool tool)
{
    theTool = tool;
}

void TileMapWidget::setPenIndex(int index)
{
    thePenIndex = qBound(0, index, 15);
}

void TileMapWidget::setBrushSize(int size)
{
    theBrushSize = qBound(1, size, 16);
}

void TileMapWidget::setActivePaletteLine(int line)
{
    theActivePaletteLine = qBound(0, line, 3);
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
        if (theCapture.embeddedTiles.contains(vramKey))
            return theCapture.embeddedTiles[vramKey];

        vramKey = QString("0x%1").arg(entry.pattern * 32, 0, 16, QChar('0')).toUpper();
        if (theCapture.embeddedTiles.contains(vramKey))
            return theCapture.embeddedTiles[vramKey];

        uint32_t targetAddr = entry.pattern * 32;
        for (auto it = theCapture.embeddedTiles.begin(); it != theCapture.embeddedTiles.end(); ++it)
        {
            bool ok = false;
            uint32_t addr = it.key().mid(2).toUInt(&ok, 16);
            if (ok && addr == targetAddr)
                return it.value();
        }
    }

    return QByteArray(32, '\0');
}

// ---------------------------------------------------------------------------
// Edit mode helpers
// ---------------------------------------------------------------------------

int TileMapWidget::tileIndexAtPixel(int px, int py) const
{
    int tileCol = px / 8;
    int tileRow = py / 8;
    if (tileCol < 0 || tileCol >= theCapture.widthTiles ||
        tileRow < 0 || tileRow >= theCapture.heightTiles)
        return -1;
    int idx = tileRow * theCapture.widthTiles + tileCol;
    if (idx >= theCapture.tileMap.size())
        return -1;
    return idx;
}

QSet<int> TileMapWidget::sharedTileIndices(int tileIdx) const
{
    QSet<int> result;
    if (tileIdx < 0 || tileIdx >= theCapture.tileMap.size())
        return result;

    const TileMapEntry & ref = theCapture.tileMap[tileIdx];

    // Find all tiles that share the same ROM offset or embedded pattern
    for (int i = 0; i < theCapture.tileMap.size(); ++i)
    {
        const TileMapEntry & other = theCapture.tileMap[i];

        if (!ref.romOffset.isEmpty() && ref.romOffset == other.romOffset)
        {
            result.insert(i);
        }
        else if (ref.source == "embedded" && other.source == "embedded" &&
                 ref.pattern == other.pattern)
        {
            result.insert(i);
        }
    }

    return result;
}

int TileMapWidget::getPixelPaletteIndex(int px, int py) const
{
    int tileIdx = tileIndexAtPixel(px, py);
    if (tileIdx < 0)
        return -1;

    const TileMapEntry & entry = theCapture.tileMap[tileIdx];
    QByteArray tileBytes = tileDataForEntry(entry);
    if (tileBytes.size() < 32)
        return -1;

    int localX = px % 8;
    int localY = py % 8;

    // Account for flip
    if (entry.hFlip) localX = 7 - localX;
    if (entry.vFlip) localY = 7 - localY;

    int byteOffset = localY * 4 + localX / 2;
    if (byteOffset >= tileBytes.size())
        return -1;

    uint8_t byte = (uint8_t)tileBytes[byteOffset];
    if (localX % 2 == 0)
        return (byte >> 4) & 0x0F;
    else
        return byte & 0x0F;
}

void TileMapWidget::setTileNibble(int tileIdx, int localX, int localY, int palIndex)
{
    if (tileIdx < 0 || tileIdx >= theCapture.tileMap.size())
        return;

    const TileMapEntry & entry = theCapture.tileMap[tileIdx];

    // Account for flip
    int rawX = entry.hFlip ? (7 - localX) : localX;
    int rawY = entry.vFlip ? (7 - localY) : localY;

    int byteOffset = rawY * 4 + rawX / 2;
    int idx = qBound(0, palIndex, 15);

    // Write to ROM if ROM-sourced
    if (!entry.romOffset.isEmpty() && theRomFile && theRomFile->isOpen())
    {
        bool ok = false;
        uint32_t offset = entry.romOffset.mid(2).toUInt(&ok, 16);
        if (ok && byteOffset < 32)
        {
            QByteArray tileBytes = theRomFile->readBytes(offset, 32);
            if (tileBytes.size() >= 32)
            {
                uint8_t byte = (uint8_t)tileBytes[byteOffset];
                if (rawX % 2 == 0)
                    byte = (byte & 0x0F) | (idx << 4);
                else
                    byte = (byte & 0xF0) | idx;

                // Write the single byte back
                QByteArray singleByte(1, (char)byte);
                theRomFile->writeBytes(offset + byteOffset, singleByte);
            }
        }
    }
    else if (entry.source == "embedded")
    {
        // Write to embedded tile data
        uint32_t vramAddr = entry.pattern * 32;
        for (auto it = theCapture.embeddedTiles.begin(); it != theCapture.embeddedTiles.end(); ++it)
        {
            bool ok = false;
            uint32_t addr = it.key().mid(2).toUInt(&ok, 16);
            if (ok && addr == vramAddr && byteOffset < it.value().size())
            {
                uint8_t byte = (uint8_t)it.value()[byteOffset];
                if (rawX % 2 == 0)
                    byte = (byte & 0x0F) | (idx << 4);
                else
                    byte = (byte & 0xF0) | idx;
                it.value()[byteOffset] = (char)byte;
                break;
            }
        }
    }
}

void TileMapWidget::paintPixelAt(int px, int py)
{
    int tileIdx = tileIndexAtPixel(px, py);
    if (tileIdx < 0)
        return;

    int localX = px % 8;
    int localY = py % 8;

    setTileNibble(tileIdx, localX, localY, thePenIndex);
    theModified = true;
}

void TileMapWidget::brushPaintAt(int px, int py)
{
    int pixW = theCapture.widthTiles * 8;
    int pixH = theCapture.heightTiles * 8;
    int radius = theBrushSize / 2;

    for (int dy = -radius; dy <= radius; ++dy)
    {
        for (int dx = -radius; dx <= radius; ++dx)
        {
            int nx = px + dx;
            int ny = py + dy;
            if (nx >= 0 && nx < pixW && ny >= 0 && ny < pixH)
                paintPixelAt(nx, ny);
        }
    }
}

void TileMapWidget::bucketFillAt(int px, int py)
{
    int pixW = theCapture.widthTiles * 8;
    int pixH = theCapture.heightTiles * 8;
    int targetIdx = getPixelPaletteIndex(px, py);
    if (targetIdx < 0 || targetIdx == thePenIndex)
        return;

    QVector<QPoint> stack;
    stack.append(QPoint(px, py));
    QVector<bool> visited(pixW * pixH, false);
    visited[py * pixW + px] = true;

    while (!stack.isEmpty())
    {
        QPoint pt = stack.takeLast();
        int cx = pt.x();
        int cy = pt.y();

        if (getPixelPaletteIndex(cx, cy) != targetIdx)
            continue;

        paintPixelAt(cx, cy);

        static const int dx[] = {-1, 1, 0, 0};
        static const int dy[] = {0, 0, -1, 1};
        for (int d = 0; d < 4; ++d)
        {
            int nx = cx + dx[d];
            int ny = cy + dy[d];
            if (nx < 0 || nx >= pixW || ny < 0 || ny >= pixH)
                continue;
            int vidx = ny * pixW + nx;
            if (visited[vidx])
                continue;
            visited[vidx] = true;
            if (getPixelPaletteIndex(nx, ny) == targetIdx)
                stack.append(QPoint(nx, ny));
        }
    }
}

void TileMapWidget::eyeDropAt(int px, int py)
{
    int idx = getPixelPaletteIndex(px, py);
    if (idx >= 0)
    {
        thePenIndex = idx;
        emit colorPicked(idx);
    }
}

// ---------------------------------------------------------------------------
// Image rendering
// ---------------------------------------------------------------------------

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

    // In edit mode, highlight shared tiles
    if (theEditMode && !theHighlightedTiles.isEmpty())
    {
        QPen highlightPen(QColor(255, 255, 0, 180), 2);
        p.setPen(highlightPen);
        p.setBrush(QColor(255, 255, 0, 40));

        for (int idx : theHighlightedTiles)
        {
            if (idx < 0 || idx >= theCapture.tileMap.size())
                continue;
            const TileMapEntry & e = theCapture.tileMap[idx];
            int rx = e.col * 8 * theZoom;
            int ry = e.row * 8 * theZoom;
            int rw = 8 * theZoom;
            int rh = 8 * theZoom;
            p.drawRect(rx, ry, rw - 1, rh - 1);
        }
    }

    // Highlight the tile under the cursor in edit mode
    if (theEditMode && theHoverTileIdx >= 0 && theHoverTileIdx < theCapture.tileMap.size())
    {
        const TileMapEntry & e = theCapture.tileMap[theHoverTileIdx];
        int rx = e.col * 8 * theZoom;
        int ry = e.row * 8 * theZoom;
        int rw = 8 * theZoom;
        int rh = 8 * theZoom;
        p.setPen(QPen(QColor(0, 255, 0, 200), 2));
        p.setBrush(Qt::NoBrush);
        p.drawRect(rx, ry, rw - 1, rh - 1);
    }
}

void TileMapWidget::mousePressEvent(QMouseEvent *event)
{
    if (!theHasCapture || !theEditMode)
        return;

    int px = event->pos().x() / theZoom;
    int py = event->pos().y() / theZoom;

    if (event->button() == Qt::LeftButton)
    {
        switch (theTool)
        {
        case TOOL_PENCIL:
            thePainting = true;
            paintPixelAt(px, py);
            rebuildImage();
            update();
            break;

        case TOOL_BUCKET:
            bucketFillAt(px, py);
            rebuildImage();
            update();
            break;

        case TOOL_EYEDROPPER:
            eyeDropAt(px, py);
            break;

        case TOOL_BRUSH:
            thePainting = true;
            brushPaintAt(px, py);
            rebuildImage();
            update();
            break;
        }
    }
    else if (event->button() == Qt::RightButton)
    {
        eyeDropAt(px, py);
    }
}

void TileMapWidget::mouseMoveEvent(QMouseEvent *event)
{
    if (!theHasCapture)
        return;

    int px = event->pos().x() / theZoom;
    int py = event->pos().y() / theZoom;

    // Emit tile hover info for the status bar
    int hoverTileCol = px / 8;
    int hoverTileRow = py / 8;
    if (hoverTileCol >= 0 && hoverTileCol < theCapture.widthTiles &&
        hoverTileRow >= 0 && hoverTileRow < theCapture.heightTiles)
    {
        int hIdx = hoverTileRow * theCapture.widthTiles + hoverTileCol;
        if (hIdx < theCapture.tileMap.size())
        {
            const TileMapEntry & he = theCapture.tileMap[hIdx];
            emit tileHovered(he.row, he.col, he.romOffset,
                             he.pattern, he.paletteLine, he.source);
        }
    }

    if (theEditMode)
    {
        // Update hover tile and shared tile highlighting
        int tileIdx = tileIndexAtPixel(px, py);
        if (tileIdx != theHoverTileIdx)
        {
            theHoverTileIdx = tileIdx;
            if (tileIdx >= 0)
                theHighlightedTiles = sharedTileIndices(tileIdx);
            else
                theHighlightedTiles.clear();
            update();
        }

        // Drag painting
        if (thePainting && (event->buttons() & Qt::LeftButton))
        {
            if (theTool == TOOL_PENCIL)
                paintPixelAt(px, py);
            else if (theTool == TOOL_BRUSH)
                brushPaintAt(px, py);

            rebuildImage();
            update();
        }
        return;
    }

    // Non-edit mode: show tooltip
    int tileCol = px / 8;
    int tileRow = py / 8;

    if (tileCol < 0 || tileCol >= theCapture.widthTiles ||
        tileRow < 0 || tileRow >= theCapture.heightTiles)
    {
        QToolTip::hideText();
        return;
    }

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

void TileMapWidget::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton)
        thePainting = false;
}
