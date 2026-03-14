#include "PaletteWidget.h"
#include <QPainter>
#include <QMouseEvent>

PaletteWidget::PaletteWidget(QWidget *parent)
    : QWidget(parent)
    , theSelectedIndex(-1)
{
    thePalette = TileDecoder::greyPalette();
    setFixedHeight(SWATCH_SIZE + 2 + PEN_ROW_HEIGHT);
}

void PaletteWidget::setPalette(const GenesisPalette & palette)
{
    thePalette = palette;
    // Ensure exactly 16 entries
    while (thePalette.size() < 16)
        thePalette.append(QColor(0, 0, 0));
    theSelectedIndex = -1;
    update();
}

const GenesisPalette & PaletteWidget::palette() const
{
    return thePalette;
}

int PaletteWidget::selectedIndex() const
{
    return theSelectedIndex;
}

void PaletteWidget::setSelectedIndex(int index)
{
    if (index >= 0 && index < 16)
    {
        theSelectedIndex = index;
        update();
    }
}

QColor PaletteWidget::selectedColor() const
{
    if (theSelectedIndex >= 0 && theSelectedIndex < thePalette.size())
        return thePalette[theSelectedIndex];
    return QColor();
}

uint16_t PaletteWidget::selectedCramWord() const
{
    if (theSelectedIndex >= 0 && theSelectedIndex < thePalette.size())
        return TileDecoder::colorToCramWord(thePalette[theSelectedIndex]);
    return 0;
}

void PaletteWidget::setColorAt(int index, const QColor & color)
{
    if (index < 0 || index >= thePalette.size())
        return;
    thePalette[index] = color;
    update();
    emit paletteModified();
}

QSize PaletteWidget::sizeHint() const
{
    return QSize(16 * SWATCH_SIZE, SWATCH_SIZE + 2 + PEN_ROW_HEIGHT);
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

        if (i == theSelectedIndex)
        {
            // White outer border + black inner border for selection highlight
            p.setPen(QPen(Qt::white, 2));
            p.drawRect(r.adjusted(0, 0, 0, 0));
            p.setPen(QPen(Qt::black, 1));
            p.drawRect(r.adjusted(2, 2, -2, -2));
        }
        else
        {
            p.setPen(Qt::black);
            p.drawRect(r);
        }
    }

    // Draw pen indicator row below swatches
    if (theSelectedIndex >= 0 && theSelectedIndex < thePalette.size())
    {
        int y = SWATCH_SIZE + 3;
        QColor penColor = thePalette[theSelectedIndex];
        uint16_t cramWord = TileDecoder::colorToCramWord(penColor);

        // Small colored square
        QRect penSwatch(2, y, 14, 14);
        if (theSelectedIndex == 0)
        {
            p.fillRect(penSwatch, Qt::white);
            p.fillRect(penSwatch.x(), penSwatch.y(), 7, 7, Qt::lightGray);
            p.fillRect(penSwatch.x() + 7, penSwatch.y() + 7, 7, 7, Qt::lightGray);
        }
        else
        {
            p.fillRect(penSwatch, penColor);
        }
        p.setPen(Qt::black);
        p.drawRect(penSwatch);

        // Label text
        QString label = QString("Pen: index %1  CRAM: 0x%2")
            .arg(theSelectedIndex)
            .arg(cramWord, 4, 16, QChar('0')).toUpper();
        p.drawText(20, y, width() - 22, 14, Qt::AlignLeft | Qt::AlignVCenter, label);
    }
}

void PaletteWidget::mousePressEvent(QMouseEvent *event)
{
    int index = int(event->position().x()) / SWATCH_SIZE;
    if (index >= 0 && index < 16 && event->position().y() < SWATCH_SIZE + 2)
    {
        theSelectedIndex = index;
        update();
        emit colorSelected(index);
    }
}

void PaletteWidget::mouseDoubleClickEvent(QMouseEvent *event)
{
    int index = int(event->position().x()) / SWATCH_SIZE;
    if (index >= 0 && index < 16 && event->position().y() < SWATCH_SIZE + 2)
    {
        theSelectedIndex = index;
        update();
        emit colorEditRequested(index);
    }
}
