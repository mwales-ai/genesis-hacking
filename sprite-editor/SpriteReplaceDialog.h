#ifndef SPRITEREPLACEDIALOG_H
#define SPRITEREPLACEDIALOG_H

#include <QDialog>
#include <QImage>
#include "TileDecoder.h"

namespace Ui { class SpriteReplaceDialog; }

class TileCanvasWidget;

class SpriteReplaceDialog : public QDialog
{
    Q_OBJECT
public:
    explicit SpriteReplaceDialog(int widthTiles, int heightTiles,
                                  const GenesisPalette & palette,
                                  QWidget *parent = nullptr);
    ~SpriteReplaceDialog();

    // Valid only after exec() == QDialog::Accepted
    QByteArray encodedTileData() const;

private slots:
    void browsePng();
    void updatePreview();

private:
    Ui::SpriteReplaceDialog *ui;
    TileCanvasWidget        *thePreviewCanvas;

    int             theWidthTiles;
    int             theHeightTiles;
    GenesisPalette  thePalette;
    QImage          theImportedImage;
    QByteArray      theEncodedData;
};

#endif // SPRITEREPLACEDIALOG_H
