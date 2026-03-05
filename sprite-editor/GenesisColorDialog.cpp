#include "GenesisColorDialog.h"
#include "TileDecoder.h"

#include <QDialogButtonBox>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QSlider>
#include <QSpinBox>
#include <QVBoxLayout>

GenesisColorDialog::GenesisColorDialog(const QColor & initial, int paletteIndex,
                                       QWidget *parent)
    : QDialog(parent)
    , thePaletteIndex(paletteIndex)
{
    setWindowTitle(QString("Edit Palette Color %1").arg(paletteIndex));
    setModal(true);

    // Convert incoming color to 3-bit Genesis values
    int r = qBound(0, (initial.red()   + 18) / 36, 7);
    int g = qBound(0, (initial.green() + 18) / 36, 7);
    int b = qBound(0, (initial.blue()  + 18) / 36, 7);

    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    // Transparency note for index 0
    if (paletteIndex == 0)
    {
        QLabel *note = new QLabel("Note: Index 0 is transparent in sprites");
        note->setStyleSheet("color: gray; font-style: italic;");
        mainLayout->addWidget(note);
    }

    // Preview swatch
    thePreviewLabel = new QLabel();
    thePreviewLabel->setFixedSize(64, 64);
    thePreviewLabel->setFrameShape(QFrame::Box);
    thePreviewLabel->setAutoFillBackground(true);

    // CRAM and RGB labels
    theCramLabel = new QLabel();
    theRgbLabel  = new QLabel();

    QHBoxLayout *previewRow = new QHBoxLayout();
    previewRow->addWidget(thePreviewLabel);
    QVBoxLayout *infoCol = new QVBoxLayout();
    infoCol->addWidget(theCramLabel);
    infoCol->addWidget(theRgbLabel);
    infoCol->addStretch();
    previewRow->addLayout(infoCol);
    previewRow->addStretch();
    mainLayout->addLayout(previewRow);

    // Channel sliders + spinboxes in a grid
    QGridLayout *grid = new QGridLayout();

    auto makeChannel = [&](const QString & name, int value, QSlider *&slider, QSpinBox *&spin, int row)
    {
        grid->addWidget(new QLabel(name), row, 0);
        slider = new QSlider(Qt::Horizontal);
        slider->setRange(0, 7);
        slider->setValue(value);
        slider->setTickPosition(QSlider::TicksBelow);
        slider->setTickInterval(1);
        grid->addWidget(slider, row, 1);
        spin = new QSpinBox();
        spin->setRange(0, 7);
        spin->setValue(value);
        grid->addWidget(spin, row, 2);
    };

    makeChannel("R:", r, theRedSlider,   theRedSpin,   0);
    makeChannel("G:", g, theGreenSlider, theGreenSpin, 1);
    makeChannel("B:", b, theBlueSlider,  theBlueSpin,  2);

    mainLayout->addLayout(grid);

    // Connect sliders <-> spinboxes
    connect(theRedSlider,   &QSlider::valueChanged, theRedSpin,   &QSpinBox::setValue);
    connect(theRedSpin,     SIGNAL(valueChanged(int)), theRedSlider, SLOT(setValue(int)));
    connect(theGreenSlider, &QSlider::valueChanged, theGreenSpin, &QSpinBox::setValue);
    connect(theGreenSpin,   SIGNAL(valueChanged(int)), theGreenSlider, SLOT(setValue(int)));
    connect(theBlueSlider,  &QSlider::valueChanged, theBlueSpin,  &QSpinBox::setValue);
    connect(theBlueSpin,    SIGNAL(valueChanged(int)), theBlueSlider, SLOT(setValue(int)));

    // Update preview on any change
    connect(theRedSlider,   &QSlider::valueChanged, this, &GenesisColorDialog::onChannelChanged);
    connect(theGreenSlider, &QSlider::valueChanged, this, &GenesisColorDialog::onChannelChanged);
    connect(theBlueSlider,  &QSlider::valueChanged, this, &GenesisColorDialog::onChannelChanged);

    // OK / Cancel
    QDialogButtonBox *buttons = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    mainLayout->addWidget(buttons);

    updatePreview();
}

void GenesisColorDialog::onChannelChanged()
{
    updatePreview();
}

void GenesisColorDialog::updatePreview()
{
    QColor c = selectedColor();
    uint16_t cram = selectedCramWord();

    QPalette pal = thePreviewLabel->palette();
    pal.setColor(QPalette::Window, c);
    thePreviewLabel->setPalette(pal);

    theCramLabel->setText(QString("CRAM: 0x%1").arg(cram, 4, 16, QChar('0')).toUpper());
    theRgbLabel->setText(QString("RGB: (%1, %2, %3)").arg(c.red()).arg(c.green()).arg(c.blue()));
}

QColor GenesisColorDialog::selectedColor() const
{
    int r = theRedSlider->value();
    int g = theGreenSlider->value();
    int b = theBlueSlider->value();
    return QColor(r * 36, g * 36, b * 36);
}

uint16_t GenesisColorDialog::selectedCramWord() const
{
    int r = theRedSlider->value();
    int g = theGreenSlider->value();
    int b = theBlueSlider->value();
    return (uint16_t)((b << 9) | (g << 5) | (r << 1));
}
