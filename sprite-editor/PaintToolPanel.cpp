#include "PaintToolPanel.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QPainter>
#include <QPixmap>

static QIcon makeToolIcon(const QString & type, int size = 28)
{
    QPixmap pix(size, size);
    pix.fill(Qt::transparent);
    QPainter p(&pix);
    p.setRenderHint(QPainter::Antialiasing, true);

    if (type == "pencil")
    {
        p.setPen(QPen(QColor(60, 60, 60), 2));
        p.drawLine(6, size - 6, size - 6, 6);
        p.setPen(Qt::NoPen);
        p.setBrush(QColor(60, 60, 60));
        p.drawEllipse(QPoint(6, size - 6), 3, 3);
        p.setPen(QPen(QColor(220, 180, 50), 3));
        p.drawLine(10, size - 10, size - 6, 6);
    }
    else if (type == "fill")
    {
        p.setPen(QPen(QColor(60, 60, 60), 2));
        QRect bucket(4, 8, 16, 14);
        p.setBrush(QColor(100, 150, 255));
        p.drawRect(bucket);
        p.setBrush(Qt::NoBrush);
        p.drawArc(10, 2, 14, 12, 30 * 16, 120 * 16);
        p.setPen(Qt::NoPen);
        p.setBrush(QColor(100, 150, 255));
        p.drawEllipse(QPoint(size - 6, size - 6), 3, 4);
    }
    else if (type == "eyedropper")
    {
        p.setPen(QPen(QColor(60, 60, 60), 2));
        p.drawLine(8, size - 8, size - 8, 8);
        p.setBrush(QColor(200, 200, 200));
        p.drawEllipse(QPoint(size - 8, 8), 5, 5);
        p.setPen(Qt::NoPen);
        p.setBrush(QColor(60, 60, 60));
        QPointF tip[3] = {QPointF(4, size - 4), QPointF(8, size - 10), QPointF(12, size - 6)};
        p.drawPolygon(tip, 3);
    }
    else if (type == "brush")
    {
        p.setPen(QPen(QColor(120, 80, 40), 3));
        p.drawLine(size - 6, 6, size / 2, size / 2);
        p.setPen(Qt::NoPen);
        p.setBrush(QColor(80, 80, 80));
        p.drawEllipse(QPoint(size / 2 - 2, size / 2 + 2), 7, 7);
    }

    p.end();
    return QIcon(pix);
}

PaintToolPanel::PaintToolPanel(QWidget *parent)
    : QWidget(parent)
{
    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(4, 4, 4, 4);
    layout->setSpacing(4);

    // 2x2 grid of icon tool buttons
    QGridLayout *toolGrid = new QGridLayout();
    toolGrid->setSpacing(2);

    int btnSize = 36;
    int iconSize = 28;
    static const char *names[] = {"pencil", "fill", "eyedropper", "brush"};
    static const char *tips[] = {
        "Pencil - draw one pixel (P)",
        "Fill - flood fill same color (F)",
        "Eyedropper - pick color (E)",
        "Brush - adjustable size (B)"
    };

    theToolButtonGroup = new QButtonGroup(this);
    theToolButtonGroup->setExclusive(true);

    for (int i = 0; i < 4; ++i)
    {
        theToolButtons[i] = new QPushButton();
        theToolButtons[i]->setIcon(makeToolIcon(names[i], iconSize));
        theToolButtons[i]->setIconSize(QSize(iconSize, iconSize));
        theToolButtons[i]->setFixedSize(btnSize, btnSize);
        theToolButtons[i]->setCheckable(true);
        theToolButtons[i]->setToolTip(tips[i]);
        toolGrid->addWidget(theToolButtons[i], i / 2, i % 2);
        theToolButtonGroup->addButton(theToolButtons[i], i);
    }
    theToolButtons[0]->setChecked(true);

    connect(theToolButtonGroup, SIGNAL(idClicked(int)),
            this,               SLOT(onToolClicked(int)));

    layout->addLayout(toolGrid);

    // Brush size (hidden by default)
    QHBoxLayout *brushRow = new QHBoxLayout();
    theBrushSizeLabel = new QLabel("Size:");
    theBrushSizeSpin = new QSpinBox();
    theBrushSizeSpin->setMinimum(1);
    theBrushSizeSpin->setMaximum(16);
    theBrushSizeSpin->setValue(3);
    theBrushSizeSpin->setToolTip("Brush size in pixels");
    brushRow->addWidget(theBrushSizeLabel);
    brushRow->addWidget(theBrushSizeSpin);
    layout->addLayout(brushRow);
    theBrushSizeLabel->setVisible(false);
    theBrushSizeSpin->setVisible(false);

    connect(theBrushSizeSpin, SIGNAL(valueChanged(int)),
            this,             SIGNAL(brushSizeChanged(int)));

    // 4x4 palette grid
    thePaletteGrid = new PaletteGridWidget();
    layout->addWidget(thePaletteGrid);

    connect(thePaletteGrid, &PaletteGridWidget::colorSelected,
            this,           &PaintToolPanel::colorSelected);

    // Delete sprite button (hidden by default)
    theDeleteButton = new QPushButton("Delete Sprite");
    theDeleteButton->setToolTip("Remove this sprite from the group");
    theDeleteButton->setVisible(false);
    theDeleteButton->setEnabled(false);
    layout->addWidget(theDeleteButton);

    connect(theDeleteButton, &QPushButton::clicked,
            this,            &PaintToolPanel::deleteRequested);

    layout->addStretch(1);
}

void PaintToolPanel::setPalette(const GenesisPalette & palette)
{
    thePaletteGrid->setPalette(palette);
}

void PaintToolPanel::setSelectedColor(int index)
{
    thePaletteGrid->setSelectedIndex(index);
}

int PaintToolPanel::selectedColor() const
{
    return thePaletteGrid->selectedIndex();
}

EditorTool PaintToolPanel::currentTool() const
{
    return static_cast<EditorTool>(theToolButtonGroup->checkedId());
}

int PaintToolPanel::brushSize() const
{
    return theBrushSizeSpin->value();
}

void PaintToolPanel::setDeleteButtonVisible(bool visible)
{
    theDeleteButton->setVisible(visible);
}

void PaintToolPanel::setDeleteButtonEnabled(bool enabled)
{
    theDeleteButton->setEnabled(enabled);
}

void PaintToolPanel::onToolClicked(int toolId)
{
    EditorTool tool = static_cast<EditorTool>(toolId);

    bool isBrush = (tool == TOOL_BRUSH);
    theBrushSizeLabel->setVisible(isBrush);
    theBrushSizeSpin->setVisible(isBrush);

    emit toolChanged(tool);
}
