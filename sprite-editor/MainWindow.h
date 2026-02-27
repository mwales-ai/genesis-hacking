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

class SpriteSheetWidget;
class RawTileBrowserWidget;
class TileCanvasWidget;
class PaletteWidget;

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

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
    void onSpriteSelected(int groupIdx, int spriteIdx);
    void onZoomChanged(int value);
    void onGridToggled(bool checked);
    void replaceSprite();
    void exportSprite();

    // Raw Tile Browser tab
    void onRawRangeChanged(int index);
    void onRawPaletteChanged(int index);
    void onRawZoomChanged(int value);
    void onRawSpriteSizeChanged(int value);
    void onRawTileClicked(int tileIndex, uint32_t romOffset);
    void onJumpToOffset();

    // Help
    void showAbout();

private:
    void setupMenus();
    void setupCustomWidgets();
    void setupConnections();
    void loadSettings();
    void saveSettings();
    void updateWindowTitle();
    void updateStatusLabel();

    void populateSpriteGroups();
    void populateRawRanges();
    void populateRawPalettes();

    void displaySpriteGroup(int groupIndex);
    void displaySpriteDetail(int groupIndex, int spriteIndex);
    void refreshRawBrowser();

    QByteArray fetchTileData(const SpriteEntry & entry);
    GenesisPalette paletteForSprite(const SpriteEntry & entry, int groupIndex);

    Ui::MainWindow       *ui;
    QSettings             theSettings;

    RomFile               theRom;
    GameDefinition        theDef;
    CompressionHandler    theCompressor;

    int                   theCurrentGroupIndex;
    int                   theCurrentSpriteIndex;

    // Custom widgets added programmatically into placeholder containers in the .ui
    SpriteSheetWidget    *theSpriteSheet;
    RawTileBrowserWidget *theRawBrowser;
    TileCanvasWidget     *theSpriteDetail;
    PaletteWidget        *thePaletteDisplay;
};

#endif // MAINWINDOW_H
