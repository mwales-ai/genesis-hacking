#ifndef PALETTEWIDGET_H
#define PALETTEWIDGET_H

#include <QWidget>
#include "TileDecoder.h"

class PaletteWidget : public QWidget
{
    Q_OBJECT
public:
    explicit PaletteWidget(QWidget *parent = nullptr);

    void setPalette(const GenesisPalette & palette);
    const GenesisPalette & palette() const;

    int selectedIndex() const;
    QColor selectedColor() const;
    uint16_t selectedCramWord() const;

    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;

public slots:
    void setColorAt(int index, const QColor & color);

signals:
    void colorSelected(int index);
    void colorEditRequested(int index);
    void paletteModified();

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseDoubleClickEvent(QMouseEvent *event) override;

private:
    static const int SWATCH_SIZE = 24;
    static const int PEN_ROW_HEIGHT = 18;
    GenesisPalette thePalette;
    int theSelectedIndex;
};

#endif // PALETTEWIDGET_H
