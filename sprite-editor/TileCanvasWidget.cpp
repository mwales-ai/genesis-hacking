#include "TileCanvasWidget.h"
#include <QPainter>
#include <QPen>
#include <QMouseEvent>

TileCanvasWidget::TileCanvasWidget(QWidget *parent)
    : QWidget(parent)
    , theZoom(4)
    , theShowGrid(false)
{
    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    setMinimumSize(64, 64);
}

void TileCanvasWidget::setSprite(const QImage & image)
{
    theSprite = image;
    resize(sizeHint());
    updateGeometry();
    update();
}

void TileCanvasWidget::clearSprite()
{
    theSprite = QImage();
    resize(sizeHint());
    updateGeometry();
    update();
}

void TileCanvasWidget::setZoom(int factor)
{
    if (factor < 1) factor = 1;
    if (factor > 8) factor = 8;
    theZoom = factor;
    resize(sizeHint());
    updateGeometry();
    update();
}

int TileCanvasWidget::zoom() const
{
    return theZoom;
}

void TileCanvasWidget::setShowGrid(bool show)
{
    theShowGrid = show;
    update();
}

bool TileCanvasWidget::showGrid() const
{
    return theShowGrid;
}

void TileCanvasWidget::setBorderOverlays(const QVector<SpriteOverlayRect> & overlays)
{
    theOverlays = overlays;
    update();
}

void TileCanvasWidget::clearBorderOverlays()
{
    theOverlays.clear();
    update();
}

QSize TileCanvasWidget::sizeHint() const
{
    if (theSprite.isNull())
        return QSize(64, 64);
    return QSize(theSprite.width() * theZoom, theSprite.height() * theZoom);
}

QSize TileCanvasWidget::minimumSizeHint() const
{
    return sizeHint();
}

void TileCanvasWidget::paintEvent(QPaintEvent *)
{
    QPainter p(this);

    // Dark background
    p.fillRect(rect(), QColor(40, 40, 40));

    if (theSprite.isNull())
    {
        p.setPen(Qt::gray);
        p.drawText(rect(), Qt::AlignCenter, "No sprite");
        return;
    }

    // Scale with nearest-neighbor to keep pixels sharp
    QImage scaled = theSprite.scaled(
        theSprite.width()  * theZoom,
        theSprite.height() * theZoom,
        Qt::IgnoreAspectRatio,
        Qt::FastTransformation);

    p.drawImage(0, 0, scaled);

    // Tile grid overlay (8px boundaries)
    if (theShowGrid)
    {
        QPen gridPen(QColor(200, 200, 200, 80));
        gridPen.setWidth(1);
        p.setPen(gridPen);

        int gridStep = 8 * theZoom;
        for (int x = gridStep; x < scaled.width(); x += gridStep)
            p.drawLine(x, 0, x, scaled.height());
        for (int y = gridStep; y < scaled.height(); y += gridStep)
            p.drawLine(0, y, scaled.width(), y);
    }

    // Border overlays — drawn at 1px width regardless of zoom
    for (const SpriteOverlayRect & overlay : theOverlays)
    {
        QPen borderPen(overlay.color);
        borderPen.setWidth(1);
        p.setPen(borderPen);
        p.setBrush(Qt::NoBrush);

        int rx = overlay.rect.x() * theZoom;
        int ry = overlay.rect.y() * theZoom;
        int rw = overlay.rect.width() * theZoom;
        int rh = overlay.rect.height() * theZoom;
        p.drawRect(rx, ry, rw - 1, rh - 1);
    }
}

void TileCanvasWidget::mouseDoubleClickEvent(QMouseEvent *event)
{
    if (theSprite.isNull() || theZoom <= 0)
    {
        QWidget::mouseDoubleClickEvent(event);
        return;
    }

    // Convert widget coords to sprite coords
    int spriteX = event->pos().x() / theZoom;
    int spriteY = event->pos().y() / theZoom;

    if (spriteX >= 0 && spriteX < theSprite.width() &&
        spriteY >= 0 && spriteY < theSprite.height())
    {
        emit doubleClicked(spriteX, spriteY);
    }
}
