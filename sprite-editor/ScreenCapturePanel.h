#ifndef SCREENCAPTUREPANEL_H
#define SCREENCAPTUREPANEL_H

#include <QWidget>
#include <QComboBox>
#include <QSpinBox>
#include <QPushButton>
#include <QLabel>
#include <QScrollArea>

#include "GameDefinition.h"
#include "RomFile.h"
#include "TileMapWidget.h"
#include "PaintToolPanel.h"

/**
 * Self-contained panel for the Screen Captures tab.
 * Handles capture loading, display, editing, and save/revert.
 * Emits signals for status messages and file dialog requests.
 */
class ScreenCapturePanel : public QWidget
{
    Q_OBJECT
public:
    explicit ScreenCapturePanel(QWidget *parent = nullptr);

    void setRomFile(RomFile *rom);
    void setGameDefinition(GameDefinition *def);

    /** Populate the combo from the game definition + loaded captures. */
    void populateCaptures();

signals:
    void statusMessage(const QString & msg);
    void captureAddedToDefinition();
    void saveRomRequested();

    /** Request the host to show a file open dialog, then call loadCaptureFile(). */
    void loadCaptureRequested();

    /** Request the host to show a confirmation dialog for removal. */
    void removeCaptureRequested(int defIndex, const QString & name);

public slots:
    /** Load captures from a file path (called by host after file dialog). */
    void loadCaptureFile(const QString & path);

    /** Confirm removal (called by host after confirmation dialog). */
    void confirmRemoveCapture(int defIndex);

private slots:
    void onCaptureSelected(int index);
    void onZoomChanged(int value);
    void onLoadClicked();
    void onAddToDef();
    void onRemoveClicked();
    void onEditToggled(bool checked);
    void onColorPicked(int paletteIndex);
    void onSaveToRom();
    void onRevert();
    void onToolChanged(EditorTool tool);
    void onBrushSizeChanged(int size);
    void onPaletteSelected(int index);
    void onTileHovered(int row, int col, const QString & romOffset,
                       int pattern, int paletteLine, const QString & source);

private:
    void buildUi();

    QComboBox          *theCaptureCombo;
    QSpinBox           *theZoomSpin;
    QPushButton        *theLoadButton;
    QPushButton        *theAddToDefButton;
    QPushButton        *theRemoveButton;
    QPushButton        *theEditButton;
    QScrollArea        *theScrollArea;
    TileMapWidget      *theTileMapWidget;
    PaintToolPanel     *theToolPanel;
    QPushButton        *theSaveRomButton;
    QPushButton        *theRevertButton;
    QLabel             *theStatusLabel;

    RomFile            *theRomFile;
    GameDefinition     *theDef;

    QVector<ScreenCapture> theLoadedCaptures;
    int                    theDefCaptureCount;
};

#endif // SCREENCAPTUREPANEL_H
