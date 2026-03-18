#ifndef RAWTILEBROWSERPANEL_H
#define RAWTILEBROWSERPANEL_H

#include <QWidget>
#include <QComboBox>
#include <QSpinBox>
#include <QPushButton>
#include <QLabel>
#include <QLineEdit>
#include <QScrollArea>

#include "GenesisTypes.h"
#include "RomDataService.h"
#include "RawTileBrowserWidget.h"

/**
 * Self-contained panel for the Raw Tile Browser tab.
 * Owns all controls (range, palette, zoom, sprite size, jump) and the
 * tile browser canvas.  Communicates with the rest of the app via signals.
 */
class RawTileBrowserPanel : public QWidget
{
    Q_OBJECT
public:
    explicit RawTileBrowserPanel(QWidget *parent = nullptr);

    void setDataService(RomDataService *service);

    /** Populate range and palette combos from game definition and ROM. */
    void populateRanges();
    void populatePalettes();

    /** Refresh the tile display from current settings. */
    void refresh();

    /** Jump to an address with specific W/H and palette (cross-tab nav). */
    void jumpToAddress(uint32_t romOffset, int widthTiles, int heightTiles,
                       int paletteComboIndex);

signals:
    void tileSelected(int tileIndex, uint32_t romOffset);
    void statusMessage(const QString & msg);

    /** Request host to show a file save dialog for PNG export. */
    void exportPngRequested(const QImage & image, const QString & suggestedName);

private slots:
    void onRangeChanged(int index);
    void onPaletteChanged(int index);
    void onZoomChanged(int value);
    void onSpriteSizeChanged(int value);
    void onTileClicked(int tileIndex, uint32_t romOffset);
    void onJumpToOffset();
    void onSetAssemblyStart();
    void onExportPng();

private:
    void buildUi();

    QComboBox              *theRangeCombo;
    QComboBox              *thePaletteCombo;
    QSpinBox               *theZoomSpin;
    QLineEdit              *theJumpOffsetEdit;
    QPushButton            *theJumpButton;
    QPushButton            *theAssemblyStartButton;
    QSpinBox               *theSpriteWSpin;
    QSpinBox               *theSpriteHSpin;
    QScrollArea            *theScrollArea;
    RawTileBrowserWidget   *theBrowser;
    QPushButton            *theExportButton;
    QLabel                 *theInfoLabel;

    RomDataService         *theDataService;

    int                     theSelectedTileIndex;
    uint32_t                theSelectedRomOffset;
};

#endif // RAWTILEBROWSERPANEL_H
