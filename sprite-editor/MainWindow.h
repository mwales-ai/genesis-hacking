#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QSettings>
#include <QImage>
#include <QMap>
#include <iostream>

#include "RomFile.h"
#include "GameDefinition.h"
#include "CompressionHandler.h"
#include "ScreenCapturePanel.h"
#include "RawTileBrowserPanel.h"
#include "SpriteEditorPanel.h"
#include "SpriteAnimationPanel.h"
#include "SpriteViewerPanel.h"
#include "RomDataService.h"

#define AppDebug if(0) std::cout

namespace Ui { class MainWindow; }

/**
 * Thin shell that owns the ROM/definition data and wires together
 * the extracted panel widgets.  All tab logic lives in the panels.
 */
class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    void loadFromCommandLine(const QStringList & args);

protected:
    void closeEvent(QCloseEvent *event) override;

private slots:
    // File menu
    void openRom();
    void openGameDefinition();
    void saveRom();
    void saveRomAs();
    void onSaveGameDefinition();

    // Panel host slots (file dialogs, confirmations, cross-tab wiring)
    void onViewerEditRequested(int collectionIndex, const QMap<int, QString> & palLineToId);
    void onViewerJumpToRaw(uint32_t romOffset, int widthTiles, int heightTiles, int palComboIdx);
    void onRawExportPngRequested(const QImage & image, const QString & suggestedName);
    void onScreenCapLoadRequested();
    void onScreenCapRemoveRequested(int defIndex, const QString & name);
    void onScreenCapSaveRomRequested();
    void onAnimLoadRecordingRequested();
    void onEditorColorEditRequested(int paletteIndex, const QColor & current,
                                     bool hasRomOffset, uint32_t romOffset, int refCount);
    void onEditorSpriteDeleted(int collectionIndex);

    void showAbout();

private:
    void setupMenus();
    void setupConnections();
    void loadSettings();
    void saveSettings();
    void updateWindowTitle();
    void updateStatusLabel();
    bool promptSaveAsIfOriginal();

    Ui::MainWindow       *ui;
    QSettings             theSettings;

    RomFile               theRom;
    QString               theOriginalRomPath;
    GameDefinition        theDef;
    CompressionHandler    theCompressor;
    RomDataService        theDataService;

    // Extracted panels
    SpriteViewerPanel     *theViewerPanel;
    RawTileBrowserPanel   *theRawBrowserPanel;
    ScreenCapturePanel    *theScreenCapPanel;
    SpriteAnimationPanel  *theAnimationPanel;
    SpriteEditorPanel     *theEditorPanel;
};

#endif // MAINWINDOW_H
