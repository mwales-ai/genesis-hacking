#ifndef TILEMAPWIDGET_H
#define TILEMAPWIDGET_H

#include <QWidget>
#include <QImage>
#include "GameDefinition.h"
#include "TileDecoder.h"
#include "RomFile.h"

class TileMapWidget : public QWidget
{
    Q_OBJECT
public:
    explicit TileMapWidget(QWidget *parent = nullptr);

    /** Load a screen capture for display. romFile may be null if all tiles are embedded. */
    void setScreenCapture(const ScreenCapture & capture, RomFile *romFile);

    void clearCapture();

    void setZoom(int factor);
    int zoom() const;

    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;

protected:
    void paintEvent(QPaintEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;

private:
    void rebuildImage();
    QByteArray tileDataForEntry(const TileMapEntry & entry) const;

    ScreenCapture        theCapture;
    GenesisPalette       thePalettes[4];  // decoded palette for each line
    RomFile             *theRomFile;
    QImage               theFullImage;    // pre-rendered full image
    int                  theZoom;
    bool                 theHasCapture;
};

#endif // TILEMAPWIDGET_H
