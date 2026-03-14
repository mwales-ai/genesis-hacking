#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QSettings>
#include <QImage>
#include <QMap>
#include <QSet>
#include <QButtonGroup>
#include <iostream>

#include "RomFile.h"
#include "GameDefinition.h"
#include "TileDecoder.h"
#include "CompressionHandler.h"

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

    // Raw Tile Browser tab
    void onRawRangeChanged(int index);
    void onRawPaletteChanged(int index);
    void onRawZoomChanged(int value);
    void onRawSpriteSizeChanged(int value);
    void onRawTileClicked(int tileIndex, uint32_t romOffset);
    void onJumpToOffset();
    void onSetAssemblyStart();
    void onRawExportPng();

    // Screen Captures tab
    void onScreenCaptureSelected(int index);
    void onScreenCapZoomChanged(int value);
    void onLoadScreenCapture();
    void onAddScreenCaptureToDef();
    void onRemoveScreenCapture();

    // Sprite Collections tab
    void onSpriteCollectionSelected(int index);
    void onSpriteCollectionZoomChanged(int value);
    void onAnimationFrameChanged(int frameIndex);
    void onLoadRecording();

    // Sprite Editor tab
    void onCollectionSpriteClicked(int spriteIndex);
    void onEditorGroupPaletteLineChanged(int paletteLine);
    void onEditorPaletteSelected(int index);
    void onEditorPaletteEditRequested(int index);
    void onEditorSave();
    void onEditorSavePalette();
    void onEditorClose();
    void onEditorZoomChanged(int value);
    void onEditorGridToggled(bool checked);
    void onEditorToolChanged(int toolId);
    void onBrushSizeChanged(int size);
    void onColorPicked(int paletteIndex);

    // Capture workflow
    void onCaptureSpriteGroup();
    void onHideSelectedSprites();
    void onUnhideSelectedSprites();
    void onCollectionSelectionChanged(const QSet<int> & selectedIndices);
    void onSaveGameDefinition();

    // Unhide only selected hidden sprites
    void onUnhideSelectedOnly();

    // Sprite Viewer: edit, double-click
    void onEditFromViewer();
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

    void populateRawRanges();
    void populateRawPalettes();
    void refreshRawBrowser();
    void populateScreenCaptures();
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
    GameDefinition        theDef;
    CompressionHandler    theCompressor;

    int                   theCurrentGroupIndex;
    int                   theCurrentSpriteIndex;
    int                   theCurrentFrameIndex;

    // Sprite Viewer: selected collection in grid
    int                   theSelectedCollectionIndex;
    bool                  theShowSpriteBorders;

    // Raw tile browser selection state
    int                   theRawSelectedTileIndex;
    uint32_t              theRawSelectedRomOffset;

    // Sprite Collections tab: track combo segments
    // [0, theCollectionCount) = regular collections or normalized collections
    // [theCollectionCount, theCollectionCount + theAnimationCount) = .sprec recordings
    int                   theCollectionCount;
    int                   theAnimationCount;     // number of .sprec recording entries
    int                   theActiveAnimIndex;    // which animation is selected (-1 if none)
    int                   theActiveRecIndex;     // which recording is selected (-1 if none)

    // .sprec recordings loaded separately from game definition
    QVector<SpriteRecording> theSpriteRecordings;

    // Editor state: which collection + sprite is being edited
    int                   theEditCollectionIndex;
    int                   theEditSpriteIndex;
    int                   theEditorActivePaletteLine;
    QMap<int, QString>    theEditPalLineToId;  // palette line -> palette pool ID for current group

    // Hidden sprites by ROM offset (persists across frames)
    QSet<QString>         theHiddenRomOffsets;

    // Auto-incrementing counter for captured sprite groups
    int                   theCaptureCounter;

    // Standalone screen captures loaded from external JSON files
    QVector<ScreenCapture> theLoadedCaptures;
    int                    theDefCaptureCount;  // how many captures are from the game def

    // Editor tool selection
    QButtonGroup          *theToolButtonGroup;
};

#endif // MAINWINDOW_H
