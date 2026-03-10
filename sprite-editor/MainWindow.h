#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QSettings>
#include <QSet>
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

    // Sprite Viewer tab (Pattern Browser in normalized mode)
    void onSpriteGroupChanged(int index);
    void onSpriteSelected(int groupIdx, int spriteIdx, int frameIdx);
    void onZoomChanged(int value);
    void onGridToggled(bool checked);
    void replaceSprite();
    void exportSprite();
    void exportAllSprites();

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

    // Sprite Collections tab
    void onSpriteCollectionSelected(int index);
    void onSpriteCollectionZoomChanged(int value);
    void onAnimationFrameChanged(int frameIndex);

    // Sprite Editor tab
    void onCollectionSpriteClicked(int spriteIndex);
    void onEditorPaletteSelected(int index);
    void onEditorPaletteEditRequested(int index);
    void onEditorSave();
    void onEditorSavePalette();
    void onEditorClose();
    void onEditorZoomChanged(int value);
    void onEditorGridToggled(bool checked);

    // Palette editing
    void onPaletteColorSelected(int index);
    void onPaletteColorEditRequested(int index);

    // Capture workflow
    void onCaptureSpriteGroup();
    void onHideSelectedSprites();
    void onUnhideSelectedSprites();
    void onCollectionSelectionChanged(const QSet<int> & selectedIndices);
    void onSaveGameDefinition();

    // Help
    void showAbout();

private:
    void setupMenus();
    void setupConnections();
    void loadSettings();
    void saveSettings();
    void updateWindowTitle();
    void updateStatusLabel();

    // Sprite Viewer / Pattern Browser
    void populateSpriteGroups();
    void populatePatterns();
    void populateRawRanges();
    void populateRawPalettes();

    void displaySpriteGroup(int groupIndex);
    void displayPattern(int patternIndex);
    void displaySpriteDetail(int groupIndex, int spriteIndex, int frameIndex = 0);
    void refreshRawBrowser();
    void populateScreenCaptures();
    void populateSpriteCollections();
    void displayAnimationFrame(int animIndex, int frameIndex);
    void displayRecordingFrame(int recIndex, int frameIndex);
    SpriteCollection buildCollectionFromFrame(const SpriteAnimation & anim, int frameIndex);
    SpriteCollection buildCollectionFromRecording(const SpriteRecording & rec, int frameIndex);
    SpriteCollection buildFromNormalized(const NormalizedCollection & norm);

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

    // Hidden sprites per-frame state (capture workflow)
    QSet<int>             theHiddenSpriteIndices;
};

#endif // MAINWINDOW_H
