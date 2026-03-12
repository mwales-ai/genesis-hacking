#ifndef SPRITESHEETWIDGET_H
#define SPRITESHEETWIDGET_H

#include <QWidget>
#include <QVector>
#include <QImage>
#include <QString>
#include <QPoint>
#include <QKeyEvent>

struct SpriteThumb
{
    QImage  image;
    QString name;
    int     groupIndex;
    int     spriteIndex;
    int     frameIndex;   // frame within a multi-frame sprite entry (0-based)
};

class SpriteSheetWidget : public QWidget
{
    Q_OBJECT
public:
    explicit SpriteSheetWidget(QWidget *parent = nullptr);

    void setSprites(const QVector<SpriteThumb> & sprites);
    void clearSprites();
    void setThumbZoom(int factor);

    QSize sizeHint() const override;

signals:
    void spriteSelected(int groupIndex, int spriteIndex, int frameIndex);
    void spriteDoubleClicked(int groupIndex, int spriteIndex, int frameIndex);

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseDoubleClickEvent(QMouseEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

private:
    void recalcLayout();
    int itemIndexAt(const QPoint & pos) const;
    void selectItem(int idx);

    QVector<SpriteThumb> theSprites;
    int                  theThumbZoom;
    int                  theSelectedIndex;
    QVector<QPoint>      theItemPositions;
    int                  theCellWidth;
    int                  theCellHeight;

    static const int CELL_PADDING = 6;
};

#endif // SPRITESHEETWIDGET_H
