#include "PaletteGridWidget.h"
#include <QPainter>
#include <QMouseEvent>

PaletteGridWidget::PaletteGridWidget(QWidget *parent)
    : QWidget(parent)
    , theSelectedIndex(-1)
{
    thePalette = TileDecoder::greyPalette();
    setFixedSize(sizeHint());
}

void PaletteGridWidget::setPalette(const GenesisPalette & palette)
{
    thePalette = palette;
    while (thePalette.size() < 16)
        thePalette.append(QColor(0, 0, 0));
    update();
}

void PaletteGridWidget::setSelectedIndex(int index)
{
    if (index >= 0 && index < 16)
    {
        theSelectedIndex = index;
        update();
    }
}

QSize PaletteGridWidget::sizeHint() const
{
    return QSize(4 * SWATCH + 2, 4 * SWATCH + 2);
}

QSize PaletteGridWidget::minimumSizeHint() const
{
    return sizeHint();
}

void PaletteGridWidget::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.setPen(Qt::black);

    for (int i = 0; i < 16 && i < thePalette.size(); ++i)
    {
        int col = i % 4;
        int row = i / 4;
        QRect r(col * SWATCH + 1, row * SWATCH + 1, SWATCH - 1, SWATCH - 1);

        if (i == 0)
        {
            // Checkerboard for transparency
            p.fillRect(r, Qt::white);
            int half = SWATCH / 2;
            p.fillRect(r.x(), r.y(), half, half, Qt::lightGray);
            p.fillRect(r.x() + half, r.y() + half, half, half, Qt::lightGray);
        }
        else
        {
            p.fillRect(r, thePalette[i]);
        }

        if (i == theSelectedIndex)
        {
            p.setPen(QPen(Qt::white, 2));
            p.drawRect(r);
            p.setPen(QPen(Qt::black, 1));
            p.drawRect(r.adjusted(2, 2, -2, -2));
        }
        else
        {
            p.setPen(Qt::black);
            p.drawRect(r);
        }
    }
}

void PaletteGridWidget::mousePressEvent(QMouseEvent *event)
{
    int col = (int(event->position().x()) - 1) / SWATCH;
    int row = (int(event->position().y()) - 1) / SWATCH;
    if (col >= 0 && col < 4 && row >= 0 && row < 4)
    {
        int index = row * 4 + col;
        theSelectedIndex = index;
        update();
        emit colorSelected(index);
    }
}
