#ifndef PALETTEGRIDWIDGET_H
#define PALETTEGRIDWIDGET_H

#include <QWidget>
#include "TileDecoder.h"

/**
 * 4x4 grid of 16 palette colors.  Click to select a pen color.
 */
class PaletteGridWidget : public QWidget
{
    Q_OBJECT
public:
    explicit PaletteGridWidget(QWidget *parent = nullptr);

    void setPalette(const GenesisPalette & palette);
    int selectedIndex() const { return theSelectedIndex; }
    void setSelectedIndex(int index);

    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;

signals:
    void colorSelected(int index);

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;

private:
    static const int SWATCH = 24;   // pixel size of each color square
    GenesisPalette thePalette;
    int theSelectedIndex;
};

#endif // PALETTEGRIDWIDGET_H
