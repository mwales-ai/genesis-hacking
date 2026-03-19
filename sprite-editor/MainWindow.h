#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QSettings>
#include <QImage>
#include <QMap>
#include <QSet>
#include <QButtonGroup>
#include <QLabel>
#include <QPushButton>
#include <QSpinBox>
#include <iostream>

#include "RomFile.h"
#include "GameDefinition.h"
#include "TileDecoder.h"
#include "CompressionHandler.h"
#include "PaletteGridWidget.h"
#include "PaletteWidget.h"
#include "SpritePixelEditor.h"
#include "ScreenCapturePanel.h"
#include "RawTileBrowserPanel.h"
#include "SpriteEditorPanel.h"
#include "SpriteAnimationPanel.h"
#include "RomDataService.h"

#define AppDebug if(0) std::cout

namespace Ui { class MainWindow; }

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    // Load ROM and/or definition from command-line arguments
    void loadFromCommandLine(const QStringList & args);

protected:
    void closeEvent(QCloseEvent *event) override;

private slots:
    // File menu
    void openRom();
    void openGameDefinition();
    void saveRom();
    void saveRomAs();

    // Sprite Viewer tab (collection grid)
    void onCollectionGridSelected(int groupIndex, int spriteIndex, int frameIndex);
    void onZoomChanged(int value);
    void onThumbZoomChanged(int value);
    void onViewerSpriteReordered(int fromIndex, int toIndex);
    void onViewerNameEditFinished();
    void onViewerBordersToggled(bool checked);

    // Raw Tile Browser (delegated to RawTileBrowserPanel)
    void onRawExportPngRequested(const QImage & image, const QString & suggestedName);

    // Screen Captures tab (delegated to ScreenCapturePanel)
    void onScreenCapLoadRequested();
    void onScreenCapRemoveRequested(int defIndex, const QString & name);
    void onScreenCapSaveRomRequested();

    // Sprite Animations (delegated to SpriteAnimationPanel)
    void onAnimLoadRecordingRequested();

    // Sprite Editor (delegated to SpriteEditorPanel)
    void onEditorColorEditRequested(int paletteIndex, const QColor & current,
                                     bool hasRomOffset, uint32_t romOffset, int refCount);
    void onEditorSpriteDeleted(int collectionIndex);

    // Capture workflow moved to SpriteAnimationPanel
    void onSaveGameDefinition();

    // Sprite Viewer: edit, double-click
    void onEditFromViewer();
    void onDeleteCollection();
    void onViewerSpriteDoubleClicked(int groupIndex, int spriteIndex, int frameIndex);
    void onDetailDoubleClicked(int spriteX, int spriteY);

    // Help
    void showAbout();

private:
    void setupMenus();
    void setupConnections();
    void loadSettings();
    void saveSettings();
    void updateWindowTitle();
    void updateStatusLabel();

    // Sprite Viewer (collection grid)
    void populateCollectionGrid();
    void updateCollectionDetail(int collectionIndex);
    QImage renderCollectionComposite(const NormalizedCollection & norm, bool showBorders);

    // Legacy display helpers (still used by editor paths)
    void populateSpriteGroups();
    void displaySpriteGroup(int groupIndex);
    void displaySpriteDetail(int groupIndex, int spriteIndex, int frameIndex = 0);

    // populateRawRanges/Palettes/refreshRawBrowser moved to RawTileBrowserPanel
    // populateScreenCaptures() moved to ScreenCapturePanel
    void populateSpriteCollections();
    void displayAnimationFrame(int animIndex, int frameIndex);
    void displayRecordingFrame(int recIndex, int frameIndex);
    SpriteCollection buildCollectionFromFrame(const SpriteAnimation & anim, int frameIndex);
    SpriteCollection buildCollectionFromRecording(const SpriteRecording & rec, int frameIndex);
    SpriteCollection buildFromNormalized(const NormalizedCollection & norm);
    void applyPersistentHidden();

    QByteArray fetchTileData(const SpriteEntry & entry);
    QByteArray fetchPatternTileData(const PoolPattern & pat);
    GenesisPalette paletteForSprite(const SpriteEntry & entry, int groupIndex);
    GenesisPalette paletteFromPool(const QString & paletteId);

    Ui::MainWindow       *ui;
    QSettings             theSettings;

    RomFile               theRom;
    QString               theOriginalRomPath;  // path of the first-opened ROM
    GameDefinition        theDef;
    CompressionHandler    theCompressor;

    int                   theCurrentGroupIndex;
    int                   theCurrentSpriteIndex;
    int                   theCurrentFrameIndex;

    // Sprite Viewer: selected collection in grid
    int                   theSelectedCollectionIndex;
    bool                  theShowSpriteBorders;

    // Raw tile browser state moved to RawTileBrowserPanel

    // Animation state moved to SpriteAnimationPanel

    // Editor state moved to SpriteEditorPanel

    // Hidden sprites and capture counter moved to SpriteAnimationPanel

    // Extracted panels
    ScreenCapturePanel    *theScreenCapPanel;
    RawTileBrowserPanel   *theRawBrowserPanel;
    SpriteEditorPanel     *theEditorPanel;
    SpriteAnimationPanel  *theAnimationPanel;
    RomDataService         theDataService;

    // Editor tool members moved to SpriteEditorPanel

    // setupEditorToolPanel moved to SpriteEditorPanel
    bool promptSaveAsIfOriginal();
};

#endif // MAINWINDOW_H
