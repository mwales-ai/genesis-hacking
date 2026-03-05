#ifndef GENESISCOLORDIALOG_H
#define GENESISCOLORDIALOG_H

#include <QDialog>
#include <QColor>
#include <stdint.h>

class QSlider;
class QSpinBox;
class QLabel;

/**
 * Modal dialog for editing a single Genesis palette color.
 * Genesis has 3-bit RGB (0-7 per channel), yielding 512 possible colors.
 */
class GenesisColorDialog : public QDialog
{
    Q_OBJECT
public:
    explicit GenesisColorDialog(const QColor & initial, int paletteIndex,
                                QWidget *parent = nullptr);

    QColor selectedColor() const;
    uint16_t selectedCramWord() const;

private slots:
    void onChannelChanged();

private:
    void updatePreview();

    int thePaletteIndex;

    QSlider  *theRedSlider;
    QSlider  *theGreenSlider;
    QSlider  *theBlueSlider;
    QSpinBox *theRedSpin;
    QSpinBox *theGreenSpin;
    QSpinBox *theBlueSpin;
    QLabel   *thePreviewLabel;
    QLabel   *theCramLabel;
    QLabel   *theRgbLabel;
};

#endif // GENESISCOLORDIALOG_H
