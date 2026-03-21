#include "PaintToolPanel.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QPainter>
#include <QPixmap>


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
    static const char *resource_paths[] = {":/painticons/icons/pencil.png",
                                           ":/painticons/icons/fill.png",
                                           ":/painticons/icons/dropper.png",
                                           ":/painticons/icons/brush.png" };
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
        theToolButtons[i]->setIcon(QPixmap(resource_paths[i]));
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
