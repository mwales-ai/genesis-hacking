#ifndef TILECANVASWIDGET_H
#define TILECANVASWIDGET_H

#include <QWidget>
#include <QImage>

class TileCanvasWidget : public QWidget
{
    Q_OBJECT
public:
    explicit TileCanvasWidget(QWidget *parent = nullptr);

    void setSprite(const QImage & image);
    void clearSprite();

    void setZoom(int factor);
    int zoom() const;

    void setShowGrid(bool show);
    bool showGrid() const;

    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    QImage theSprite;
    int    theZoom;
    bool   theShowGrid;
};

#endif // TILECANVASWIDGET_H
