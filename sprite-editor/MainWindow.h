#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QSettings>
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

    // Sprite Viewer tab
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

    // Help
    void showAbout();

private:
    void setupMenus();
    void setupConnections();
    void loadSettings();
    void saveSettings();
    void updateWindowTitle();
    void updateStatusLabel();

    void populateSpriteGroups();
    void populateRawRanges();
    void populateRawPalettes();

    void displaySpriteGroup(int groupIndex);
    void displaySpriteDetail(int groupIndex, int spriteIndex, int frameIndex = 0);
    void refreshRawBrowser();
    void populateScreenCaptures();
    void populateSpriteCollections();
    void displayAnimationFrame(int animIndex, int frameIndex);
    SpriteCollection buildCollectionFromFrame(const SpriteAnimation & anim, int frameIndex);

    QByteArray fetchTileData(const SpriteEntry & entry);
    GenesisPalette paletteForSprite(const SpriteEntry & entry, int groupIndex);

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

    // Sprite Collections tab: track which combo entries are animations
    // Combo entries: first N = regular collections, then M = animations
    int                   theCollectionCount;    // number of regular collections in combo
    int                   theActiveAnimIndex;    // which animation is selected (-1 if none)

    // Editor state: which collection + sprite is being edited
    int                   theEditCollectionIndex;
    int                   theEditSpriteIndex;
};

#endif // MAINWINDOW_H
