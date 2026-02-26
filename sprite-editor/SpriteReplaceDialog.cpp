#include "SpriteReplaceDialog.h"
#include "ui_SpriteReplaceDialog.h"
#include "TileCanvasWidget.h"
#include <QFileDialog>
#include <QMessageBox>
#include <QPushButton>
#include <QVBoxLayout>

SpriteReplaceDialog::SpriteReplaceDialog(int widthTiles, int heightTiles,
                                          const GenesisPalette & palette,
                                          QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::SpriteReplaceDialog)
    , theWidthTiles(widthTiles)
    , theHeightTiles(heightTiles)
    , thePalette(palette)
{
    ui->setupUi(this);

    ui->theDimLabel->setText(
        QString("Target: %1 tiles wide × %2 tiles tall  (%3×%4 pixels)")
        .arg(widthTiles).arg(heightTiles)
        .arg(widthTiles * 8).arg(heightTiles * 8));

    // Add preview canvas into the placeholder
    thePreviewCanvas = new TileCanvasWidget(ui->thePreviewPlaceholder);
    thePreviewCanvas->setZoom(4);
    QVBoxLayout *previewLayout = qobject_cast<QVBoxLayout*>(
        ui->thePreviewPlaceholder->layout());
    if (previewLayout)
        previewLayout->addWidget(thePreviewCanvas);

    // OK is disabled until an image is loaded
    ui->theButtonBox->button(QDialogButtonBox::Ok)->setEnabled(false);

    connect(ui->theBrowseButton, &QPushButton::clicked, this, &SpriteReplaceDialog::browsePng);
    connect(ui->theButtonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(ui->theButtonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
}

SpriteReplaceDialog::~SpriteReplaceDialog()
{
    delete ui;
}

QByteArray SpriteReplaceDialog::encodedTileData() const
{
    return theEncodedData;
}

void SpriteReplaceDialog::browsePng()
{
    QString path = QFileDialog::getOpenFileName(
        this, "Open Image", QString(),
        "Images (*.png *.bmp *.gif *.jpg *.jpeg);;All Files (*)");

    if (path.isEmpty())
        return;

    QImage img;
    if (!img.load(path))
    {
        QMessageBox::critical(this, "Error", "Failed to load image: " + path);
        return;
    }

    int expectedW = theWidthTiles  * 8;
    int expectedH = theHeightTiles * 8;

    QString warning;
    if (img.width() != expectedW || img.height() != expectedH)
    {
        warning = QString("Image is %1×%2 px. Scaling to %3×%4 px to fit sprite.")
            .arg(img.width()).arg(img.height())
            .arg(expectedW).arg(expectedH);
        img = img.scaled(expectedW, expectedH,
                         Qt::IgnoreAspectRatio, Qt::FastTransformation);
    }

    ui->theWarningLabel->setText(warning);
    ui->thePathEdit->setText(path);

    theImportedImage = img.convertToFormat(QImage::Format_ARGB32);
    updatePreview();
}

void SpriteReplaceDialog::updatePreview()
{
    if (theImportedImage.isNull())
        return;

    // Round-trip: encode to 4bpp then decode back so the preview
    // shows exactly what will be stored in the ROM.
    theEncodedData = TileDecoder::encodeSprite(
        theImportedImage, theWidthTiles, theHeightTiles, thePalette);

    if (theEncodedData.isEmpty())
    {
        ui->theWarningLabel->setText("Failed to encode sprite (dimension mismatch?)");
        ui->theButtonBox->button(QDialogButtonBox::Ok)->setEnabled(false);
        return;
    }

    QImage preview = TileDecoder::decodeSprite(
        theEncodedData, theWidthTiles, theHeightTiles, thePalette);

    thePreviewCanvas->setSprite(preview);
    ui->theButtonBox->button(QDialogButtonBox::Ok)->setEnabled(true);
}
