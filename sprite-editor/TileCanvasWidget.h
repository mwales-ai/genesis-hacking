#ifndef TILECANVASWIDGET_H
#define TILECANVASWIDGET_H

#include <QWidget>
#include <QImage>
#include <QVector>
#include <QRect>
#include <QColor>

struct SpriteOverlayRect
{
    QRect  rect;   // in sprite-pixel coordinates
    QColor color;
};

class TileCanvasWidget : public QWidget
{
    Q_OBJECT
public:
    explicit TileCanvasWidget(QWidget *parent = nullptr);

    void setSprite(const QImage & image);
    void clearSprite();

    const QImage & sprite() const { return theSprite; }

    void setZoom(int factor);
    int zoom() const;

    void setShowGrid(bool show);
    bool showGrid() const;

    /** Set border overlay rectangles (drawn at 1px regardless of zoom). */
    void setBorderOverlays(const QVector<SpriteOverlayRect> & overlays);
    void clearBorderOverlays();

    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;

signals:
    void doubleClicked(int spriteX, int spriteY);

protected:
    void paintEvent(QPaintEvent *event) override;
    void mouseDoubleClickEvent(QMouseEvent *event) override;

private:
    QImage theSprite;
    int    theZoom;
    bool   theShowGrid;
    QVector<SpriteOverlayRect> theOverlays;
};

#endif // TILECANVASWIDGET_H
