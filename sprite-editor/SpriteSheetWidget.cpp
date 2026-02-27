#include "SpriteSheetWidget.h"
#include <QPainter>
#include <QMouseEvent>
#include <QFontMetrics>

SpriteSheetWidget::SpriteSheetWidget(QWidget *parent)
    : QWidget(parent)
    , theThumbZoom(2)
    , theSelectedIndex(-1)
    , theCellWidth(80)
    , theCellHeight(80)
{
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    setFocusPolicy(Qt::ClickFocus);
}

void SpriteSheetWidget::setSprites(const QVector<SpriteThumb> & sprites)
{
    theSprites = sprites;
    theSelectedIndex = -1;
    recalcLayout();
    updateGeometry();
    update();
}

void SpriteSheetWidget::clearSprites()
{
    theSprites.clear();
    theSelectedIndex = -1;
    theItemPositions.clear();
    updateGeometry();
    update();
}

void SpriteSheetWidget::setThumbZoom(int factor)
{
    if (factor < 1) factor = 1;
    theThumbZoom = factor;
    recalcLayout();
    update();
}

void SpriteSheetWidget::recalcLayout()
{
    if (theSprites.isEmpty())
    {
        theItemPositions.clear();
        return;
    }

    // Find the maximum sprite dimensions to set a uniform cell size
    int maxW = 8, maxH = 8;
    for (const auto & s : theSprites)
    {
        if (s.image.width()  > maxW) maxW = s.image.width();
        if (s.image.height() > maxH) maxH = s.image.height();
    }

    int nameHeight = 14;
    theCellWidth  = maxW * theThumbZoom + CELL_PADDING * 2;
    theCellHeight = maxH * theThumbZoom + CELL_PADDING * 2 + nameHeight;

    int availWidth = (width() > 0) ? width() : 400;
    int cols = qMax(1, availWidth / theCellWidth);

    theItemPositions.resize(theSprites.size());
    for (int i = 0; i < theSprites.size(); ++i)
    {
        int col = i % cols;
        int row = i / cols;
        theItemPositions[i] = QPoint(col * theCellWidth, row * theCellHeight);
    }

    // Set widget height to fit all rows
    int rows = (theSprites.size() + cols - 1) / cols;
    setMinimumHeight(rows * theCellHeight);
}

QSize SpriteSheetWidget::sizeHint() const
{
    if (theItemPositions.isEmpty())
        return QSize(200, 100);
    int totalH = 0;
    for (const auto & pos : theItemPositions)
        totalH = qMax(totalH, pos.y() + theCellHeight);
    return QSize(width(), totalH);
}

void SpriteSheetWidget::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.fillRect(rect(), QColor(30, 30, 30));

    if (theSprites.isEmpty())
    {
        p.setPen(Qt::gray);
        p.drawText(rect(), Qt::AlignCenter, "Load a ROM and Game Definition\nto view sprites");
        return;
    }

    QFont nameFont = p.font();
    nameFont.setPointSize(7);
    p.setFont(nameFont);
    QFontMetrics fm(nameFont);

    for (int i = 0; i < theSprites.size() && i < theItemPositions.size(); ++i)
    {
        QPoint topLeft = theItemPositions[i];
        QRect cellRect(topLeft, QSize(theCellWidth, theCellHeight));

        // Selection highlight
        if (i == theSelectedIndex)
        {
            p.fillRect(cellRect, QColor(0, 80, 160));
            p.setPen(QColor(100, 160, 255));
            p.drawRect(cellRect.adjusted(0, 0, -1, -1));
        }

        // Draw sprite thumbnail
        const QImage & img = theSprites[i].image;
        if (!img.isNull())
        {
            QImage scaled = img.scaled(
                img.width()  * theThumbZoom,
                img.height() * theThumbZoom,
                Qt::IgnoreAspectRatio,
                Qt::FastTransformation);

            int imgX = topLeft.x() + (theCellWidth - scaled.width()) / 2;
            int imgY = topLeft.y() + CELL_PADDING;
            p.drawImage(imgX, imgY, scaled);
        }

        // Draw name label below sprite
        QString name = theSprites[i].name;
        name = fm.elidedText(name, Qt::ElideRight, theCellWidth - 4);
        p.setPen(i == theSelectedIndex ? Qt::white : Qt::lightGray);
        p.drawText(topLeft.x() + 2, topLeft.y() + theCellHeight - 4, name);
    }
}

void SpriteSheetWidget::mousePressEvent(QMouseEvent *event)
{
    int idx = itemIndexAt(event->pos());
    if (idx >= 0 && idx < theSprites.size())
        selectItem(idx);
}

void SpriteSheetWidget::keyPressEvent(QKeyEvent *event)
{
    if (theSprites.isEmpty())
    {
        QWidget::keyPressEvent(event);
        return;
    }

    int cols = qMax(1, (theCellWidth > 0) ? (width() / theCellWidth) : 1);
    int cur  = (theSelectedIndex >= 0) ? theSelectedIndex : 0;

    int next = cur;
    switch (event->key())
    {
    case Qt::Key_Right: next = cur + 1; break;
    case Qt::Key_Left:  next = cur - 1; break;
    case Qt::Key_Down:  next = cur + cols; break;
    case Qt::Key_Up:    next = cur - cols; break;
    case Qt::Key_Home:  next = 0; break;
    case Qt::Key_End:   next = theSprites.size() - 1; break;
    default:
        QWidget::keyPressEvent(event);
        return;
    }

    next = qBound(0, next, theSprites.size() - 1);
    if (next != cur || theSelectedIndex < 0)
        selectItem(next);
}

void SpriteSheetWidget::selectItem(int idx)
{
    if (idx < 0 || idx >= theSprites.size()) return;
    theSelectedIndex = idx;
    update();
    emit spriteSelected(theSprites[idx].groupIndex, theSprites[idx].spriteIndex);
}

void SpriteSheetWidget::resizeEvent(QResizeEvent *)
{
    recalcLayout();
}

int SpriteSheetWidget::itemIndexAt(const QPoint & pos) const
{
    for (int i = 0; i < theItemPositions.size(); ++i)
    {
        QRect r(theItemPositions[i], QSize(theCellWidth, theCellHeight));
        if (r.contains(pos))
            return i;
    }
    return -1;
}
