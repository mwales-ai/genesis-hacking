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

    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;

signals:
    void colorClicked(int index);

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;

private:
    static const int SWATCH_SIZE = 24;
    GenesisPalette thePalette;
};

#endif // PALETTEWIDGET_H
