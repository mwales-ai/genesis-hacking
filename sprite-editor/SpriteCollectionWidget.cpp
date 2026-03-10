#include "SpriteCollectionWidget.h"
#include <QPainter>
#include <QMouseEvent>
#include <QToolTip>
#include <iostream>

#define CollDebug if(0) std::cout

SpriteCollectionWidget::SpriteCollectionWidget(QWidget *parent)
    : QWidget(parent)
    , theRomFile(nullptr)
    , theZoom(2)
    , theHasCollection(false)
    , theHoveredSpriteIndex(-1)
    , theRubberBanding(false)
{
    setMouseTracking(true);
}

void SpriteCollectionWidget::setCollection(const SpriteCollection & collection, RomFile *romFile)
{
    theCollection = collection;
    theRomFile = romFile;
    theHasCollection = true;

    // Decode the 4 palettes from CRAM values
    for (int line = 0; line < 4; ++line)
    {
        if (line < collection.palettes.size())
            thePalettes[line] = TileDecoder::decodePaletteFromCram(collection.palettes[line].cramValues);
        else
            thePalettes[line] = TileDecoder::greyPalette();
    }

    rebuildImage();
    resize(sizeHint());
    updateGeometry();
    update();
}

void SpriteCollectionWidget::clearCollection()
{
    theHasCollection = false;
    theCompositeImage = QImage();
    theHoveredSpriteIndex = -1;
    theSelectedSpriteIndices.clear();
    theHiddenSpriteIndices.clear();
    theRubberBanding = false;
    updateGeometry();
    update();
}

void SpriteCollectionWidget::clearSelection()
{
    theHoveredSpriteIndex = -1;
    theSelectedSpriteIndices.clear();
    update();
}

void SpriteCollectionWidget::setHiddenSprites(const QSet<int> & indices)
{
    theHiddenSpriteIndices = indices;
    rebuildImage();
    update();
}

void SpriteCollectionWidget::clearHiddenSprites()
{
    theHiddenSpriteIndices.clear();
    rebuildImage();
    update();
}

void SpriteCollectionWidget::setZoom(int factor)
{
    if (factor < 1) factor = 1;
    if (factor > 8) factor = 8;
    theZoom = factor;
    resize(sizeHint());
    updateGeometry();
    update();
}

int SpriteCollectionWidget::zoom() const
{
    return theZoom;
}

QSize SpriteCollectionWidget::sizeHint() const
{
    if (!theHasCollection || theCompositeImage.isNull())
        return QSize(320, 224);
    return QSize(theCompositeImage.width() * theZoom,
                 theCompositeImage.height() * theZoom);
}

QSize SpriteCollectionWidget::minimumSizeHint() const
{
    if (!theHasCollection || theCompositeImage.isNull())
        return QSize(160, 112);
    return QSize(theCompositeImage.width(),
                 theCompositeImage.height());
}

QByteArray SpriteCollectionWidget::tileDataForSprite(const CollectionSprite & sprite) const
{
    int totalBytes = sprite.widthTiles * sprite.heightTiles * 32;

    // If embedded tile data is provided, use it directly
    if (sprite.source == "embedded" && !sprite.tileData.isEmpty())
        return sprite.tileData;

    // Try reading from ROM
    if (!sprite.romOffset.isEmpty() && theRomFile && theRomFile->isOpen())
    {
        bool ok = false;
        QString offsetStr = sprite.romOffset;
        if (offsetStr.startsWith("0x") || offsetStr.startsWith("0X"))
            offsetStr = offsetStr.mid(2);
        uint32_t offset = offsetStr.toUInt(&ok, 16);
        if (ok)
            return theRomFile->readBytes(offset, totalBytes);
    }

    // Blank fallback
    return QByteArray(totalBytes, '\0');
}

void SpriteCollectionWidget::rebuildImage()
{
    if (!theHasCollection || theCollection.sprites.isEmpty())
        return;

    // Use bounding box from the collection
    QRect bbox = theCollection.boundingBox;
    if (bbox.width() <= 0 || bbox.height() <= 0)
    {
        // Recalculate from sprites if missing
        int minX = 32767, minY = 32767, maxX = -32768, maxY = -32768;
        for (const CollectionSprite & s : theCollection.sprites)
        {
            if (s.x < minX) minX = s.x;
            if (s.y < minY) minY = s.y;
            int right  = s.x + s.widthTiles * 8;
            int bottom = s.y + s.heightTiles * 8;
            if (right > maxX)  maxX = right;
            if (bottom > maxY) maxY = bottom;
        }
        bbox = QRect(minX, minY, maxX - minX, maxY - minY);
    }

    int pixW = bbox.width();
    int pixH = bbox.height();
    if (pixW <= 0 || pixH <= 0) return;

    theCompositeImage = QImage(pixW, pixH, QImage::Format_ARGB32);
    theCompositeImage.fill(Qt::transparent);

    // Render sprites in order (first sprite is highest priority in link list)
    // Reverse order so earlier sprites draw on top
    for (int i = theCollection.sprites.size() - 1; i >= 0; --i)
    {
        // Skip hidden sprites
        if (theHiddenSpriteIndices.contains(i))
            continue;

        const CollectionSprite & sprite = theCollection.sprites[i];
        int palLine = qBound(0, sprite.paletteLine, 3);

        QByteArray tileBytes = tileDataForSprite(sprite);
        if (tileBytes.isEmpty())
            continue;

        // Decode the multi-tile sprite
        // Genesis sprites are column-major: tile index = col * heightTiles + row
        int spritePixX = sprite.x - bbox.x();
        int spritePixY = sprite.y - bbox.y();

        for (int col = 0; col < sprite.widthTiles; ++col)
        {
            for (int row = 0; row < sprite.heightTiles; ++row)
            {
                int tileIdx = col * sprite.heightTiles + row;
                int tileOffset = tileIdx * 32;
                if (tileOffset + 32 > tileBytes.size())
                    continue;

                QImage tile = TileDecoder::decodeTileFlipped(
                    tileBytes, tileOffset, thePalettes[palLine],
                    sprite.hFlip, sprite.vFlip);

                // Calculate tile position within the sprite, respecting flip
                int tileX, tileY;
                if (sprite.hFlip)
                    tileX = (sprite.widthTiles - 1 - col) * 8;
                else
                    tileX = col * 8;
                if (sprite.vFlip)
                    tileY = (sprite.heightTiles - 1 - row) * 8;
                else
                    tileY = row * 8;

                int dx = spritePixX + tileX;
                int dy = spritePixY + tileY;

                for (int ty = 0; ty < 8; ++ty)
                {
                    for (int tx = 0; tx < 8; ++tx)
                    {
                        int px = dx + tx;
                        int py = dy + ty;
                        if (px < 0 || px >= pixW || py < 0 || py >= pixH)
                            continue;
                        QRgb pixel = tile.pixel(tx, ty);
                        if (qAlpha(pixel) > 0)
                            theCompositeImage.setPixel(px, py, pixel);
                    }
                }
            }
        }
    }

    CollDebug << "SpriteCollectionWidget: rebuilt " << pixW << "x" << pixH
              << " from " << theCollection.sprites.size() << " sprites" << std::endl;
}

int SpriteCollectionWidget::hitTestSprite(const QPoint & widgetPos) const
{
    if (!theHasCollection)
        return -1;

    QRect bbox = theCollection.boundingBox;
    int worldX = widgetPos.x() / theZoom + bbox.x();
    int worldY = widgetPos.y() / theZoom + bbox.y();

    // Check sprites in order (first = highest priority)
    for (int i = 0; i < theCollection.sprites.size(); ++i)
    {
        const CollectionSprite & s = theCollection.sprites[i];
        int sw = s.widthTiles * 8;
        int sh = s.heightTiles * 8;

        if (worldX >= s.x && worldX < s.x + sw &&
            worldY >= s.y && worldY < s.y + sh)
        {
            return i;
        }
    }
    return -1;
}

void SpriteCollectionWidget::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    QPainter p(this);

    if (!theHasCollection || theCompositeImage.isNull())
    {
        p.fillRect(rect(), Qt::darkGray);
        p.setPen(Qt::white);
        p.drawText(rect(), Qt::AlignCenter, "No sprite collection loaded");
        return;
    }

    // Draw a checkerboard background to show transparency
    int scaledW = theCompositeImage.width() * theZoom;
    int scaledH = theCompositeImage.height() * theZoom;
    int checkSize = 8 * theZoom;
    for (int cy = 0; cy < scaledH; cy += checkSize)
    {
        for (int cx = 0; cx < scaledW; cx += checkSize)
        {
            bool dark = ((cx / checkSize) + (cy / checkSize)) % 2;
            p.fillRect(cx, cy, checkSize, checkSize,
                       dark ? QColor(180, 180, 180) : QColor(220, 220, 220));
        }
    }

    QImage scaled = theCompositeImage.scaled(scaledW, scaledH,
                                              Qt::IgnoreAspectRatio,
                                              Qt::FastTransformation);
    p.drawImage(0, 0, scaled);

    // Draw outlines around hovered, selected, and hidden sprites
    QRect bbox = theCollection.boundingBox;
    for (int i = 0; i < theCollection.sprites.size(); ++i)
    {
        bool isSelected = theSelectedSpriteIndices.contains(i);
        bool isHovered  = (i == theHoveredSpriteIndex && !isSelected);
        bool isHidden   = theHiddenSpriteIndices.contains(i);

        if (!isSelected && !isHovered && !isHidden)
            continue;

        const CollectionSprite & s = theCollection.sprites[i];
        int rx = (s.x - bbox.x()) * theZoom;
        int ry = (s.y - bbox.y()) * theZoom;
        int rw = s.widthTiles * 8 * theZoom;
        int rh = s.heightTiles * 8 * theZoom;

        if (isHidden)
        {
            // Hidden sprites: show dashed outline only on hover or select
            if (isSelected)
            {
                p.setPen(QPen(QColor(0, 220, 0), 2, Qt::DashLine));
                p.drawRect(rx, ry, rw - 1, rh - 1);
            }
            else if (isHovered)
            {
                p.setPen(QPen(QColor(160, 160, 160), 1, Qt::DashLine));
                p.drawRect(rx, ry, rw - 1, rh - 1);
            }
        }
        else if (isSelected)
        {
            p.setPen(QPen(QColor(0, 220, 0), 2));
            p.drawRect(rx, ry, rw - 1, rh - 1);
        }
        else  // hovered
        {
            p.setPen(QPen(QColor(100, 255, 100), 1, Qt::DashLine));
            p.drawRect(rx, ry, rw - 1, rh - 1);
        }
    }

    // Draw rubber-band rectangle
    if (theRubberBanding && !theRubberBandRect.isNull())
    {
        p.setPen(QPen(QColor(60, 120, 255), 1, Qt::DashLine));
        p.setBrush(QColor(60, 120, 255, 40));
        p.drawRect(theRubberBandRect);
    }
}

void SpriteCollectionWidget::mouseMoveEvent(QMouseEvent *event)
{
    if (!theHasCollection)
        return;

    // Rubber-band dragging
    if (theRubberBanding)
    {
        theRubberBandRect = QRect(theRubberBandOrigin, event->pos()).normalized();
        update();
        return;
    }

    int oldHover = theHoveredSpriteIndex;
    theHoveredSpriteIndex = hitTestSprite(event->pos());

    if (theHoveredSpriteIndex != oldHover)
        update();

    QRect bbox = theCollection.boundingBox;
    int worldX = event->pos().x() / theZoom + bbox.x();
    int worldY = event->pos().y() / theZoom + bbox.y();

    if (theHoveredSpriteIndex >= 0)
    {
        const CollectionSprite & s = theCollection.sprites[theHoveredSpriteIndex];
        int sw = s.widthTiles * 8;
        int sh = s.heightTiles * 8;

        QString hiddenTag = theHiddenSpriteIndices.contains(theHoveredSpriteIndex)
            ? " [HIDDEN]" : "";
        QString tip = QString("Sprite #%1%12\nPos: (%2, %3)  Size: %4x%5\n"
                              "Pattern: %6  Palette: %7\n"
                              "Flip H:%8 V:%9  Priority: %10\n"
                              "ROM: %11  Source: %13\nClick to edit, Ctrl+click to multi-select")
            .arg(s.index)
            .arg(s.x).arg(s.y)
            .arg(sw).arg(sh)
            .arg(s.pattern).arg(s.paletteLine)
            .arg(s.hFlip ? "yes" : "no")
            .arg(s.vFlip ? "yes" : "no")
            .arg(s.priority ? "yes" : "no")
            .arg(s.romOffset.isEmpty() ? "N/A" : s.romOffset)
            .arg(hiddenTag)
            .arg(s.source);

        QToolTip::showText(event->globalPosition().toPoint(), tip, this);
        emit spriteHovered(theHoveredSpriteIndex, worldX, worldY);
    }
    else
    {
        QToolTip::hideText();
    }
}

void SpriteCollectionWidget::mousePressEvent(QMouseEvent *event)
{
    if (!theHasCollection || event->button() != Qt::LeftButton)
        return;

    int idx = hitTestSprite(event->pos());

    if (idx >= 0)
    {
        bool ctrl = (event->modifiers() & Qt::ControlModifier);
        if (ctrl)
        {
            // Ctrl+click: toggle in selection set
            if (theSelectedSpriteIndices.contains(idx))
                theSelectedSpriteIndices.remove(idx);
            else
                theSelectedSpriteIndices.insert(idx);
        }
        else
        {
            // Plain click: replace selection with single sprite
            theSelectedSpriteIndices.clear();
            theSelectedSpriteIndices.insert(idx);
            emit spriteClicked(idx);  // backward compat: single-sprite editor
        }
        emit selectionChanged(theSelectedSpriteIndices);
        update();
    }
    else
    {
        // Click on empty area: start rubber-band
        theRubberBanding = true;
        theRubberBandOrigin = event->pos();
        theRubberBandRect = QRect();
    }
}

void SpriteCollectionWidget::mouseReleaseEvent(QMouseEvent *event)
{
    if (!theHasCollection || event->button() != Qt::LeftButton)
        return;

    if (theRubberBanding)
    {
        theRubberBanding = false;

        // Convert rubber-band rect to world coordinates
        QRect bbox = theCollection.boundingBox;
        QRect rb = QRect(theRubberBandOrigin, event->pos()).normalized();
        int worldLeft   = rb.left()   / theZoom + bbox.x();
        int worldTop    = rb.top()    / theZoom + bbox.y();
        int worldRight  = rb.right()  / theZoom + bbox.x();
        int worldBottom = rb.bottom() / theZoom + bbox.y();

        // Find all sprites that intersect the rubber-band
        QSet<int> hitSprites;
        for (int i = 0; i < theCollection.sprites.size(); ++i)
        {
            const CollectionSprite & s = theCollection.sprites[i];
            int sw = s.widthTiles * 8;
            int sh = s.heightTiles * 8;

            // Check intersection
            if (s.x + sw > worldLeft && s.x < worldRight + 1 &&
                s.y + sh > worldTop  && s.y < worldBottom + 1)
            {
                hitSprites.insert(i);
            }
        }

        bool ctrl = (event->modifiers() & Qt::ControlModifier);
        if (ctrl)
        {
            // Ctrl+rubber-band: add to existing selection
            theSelectedSpriteIndices.unite(hitSprites);
        }
        else
        {
            // Plain rubber-band: replace selection
            theSelectedSpriteIndices = hitSprites;
        }

        theRubberBandRect = QRect();
        emit selectionChanged(theSelectedSpriteIndices);
        update();
    }
}
