#include "PaletteWidget.h"
#include <QPainter>
#include <QMouseEvent>

PaletteWidget::PaletteWidget(QWidget *parent)
    : QWidget(parent)
{
    thePalette = TileDecoder::greyPalette();
    setFixedHeight(SWATCH_SIZE + 2);
}

void PaletteWidget::setPalette(const GenesisPalette & palette)
{
    thePalette = palette;
    // Ensure exactly 16 entries
    while (thePalette.size() < 16)
        thePalette.append(QColor(0, 0, 0));
    update();
}

const GenesisPalette & PaletteWidget::palette() const
{
    return thePalette;
}

QSize PaletteWidget::sizeHint() const
{
    return QSize(16 * SWATCH_SIZE, SWATCH_SIZE + 2);
}

QSize PaletteWidget::minimumSizeHint() const
{
    return sizeHint();
}

void PaletteWidget::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.setPen(Qt::black);

    for (int i = 0; i < 16 && i < thePalette.size(); ++i)
    {
        QRect r(i * SWATCH_SIZE, 1, SWATCH_SIZE - 1, SWATCH_SIZE - 1);

        if (i == 0)
        {
            // Draw checkerboard to indicate transparency
            p.fillRect(r, Qt::white);
            p.fillRect(r.x(),                   r.y(),                   SWATCH_SIZE/2, SWATCH_SIZE/2, Qt::lightGray);
            p.fillRect(r.x() + SWATCH_SIZE/2,   r.y() + SWATCH_SIZE/2,  SWATCH_SIZE/2, SWATCH_SIZE/2, Qt::lightGray);
        }
        else
        {
            p.fillRect(r, thePalette[i]);
        }
        p.drawRect(r);
    }
}

void PaletteWidget::mousePressEvent(QMouseEvent *event)
{
    int index = int(event->position().x()) / SWATCH_SIZE;
    if (index >= 0 && index < 16)
        emit colorClicked(index);
}
