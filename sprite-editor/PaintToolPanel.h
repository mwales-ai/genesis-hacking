#ifndef PAINTTOOLPANEL_H
#define PAINTTOOLPANEL_H

#include <QWidget>
#include <QButtonGroup>
#include <QPushButton>
#include <QSpinBox>
#include <QLabel>
#include "TileDecoder.h"
#include "SpritePixelEditor.h"   // for EditorTool enum
#include "PaletteGridWidget.h"

/**
 * Reusable tool panel: 2x2 icon buttons for paint tools,
 * brush size spinner, 4x4 palette color grid, and optional
 * delete-sprite button.
 *
 * Used by both the Sprite Editor tab and Screen Capture edit mode.
 */
class PaintToolPanel : public QWidget
{
    Q_OBJECT
public:
    explicit PaintToolPanel(QWidget *parent = nullptr);

    void setPalette(const GenesisPalette & palette);
    void setSelectedColor(int index);
    int selectedColor() const;

    EditorTool currentTool() const;
    int brushSize() const;

    /** Show/hide the delete sprite button (for editor mode). */
    void setDeleteButtonVisible(bool visible);
    void setDeleteButtonEnabled(bool enabled);

signals:
    void toolChanged(EditorTool tool);
    void brushSizeChanged(int size);
    void colorSelected(int paletteIndex);
    void deleteRequested();

private slots:
    void onToolClicked(int toolId);

private:
    QButtonGroup      *theToolButtonGroup;
    QPushButton       *theToolButtons[4];
    QLabel            *theBrushSizeLabel;
    QSpinBox          *theBrushSizeSpin;
    PaletteGridWidget *thePaletteGrid;
    QPushButton       *theDeleteButton;
};

#endif // PAINTTOOLPANEL_H
