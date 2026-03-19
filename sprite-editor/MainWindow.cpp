#include "MainWindow.h"
#include "ui_MainWindow.h"
#include "SpritePixelEditor.h"
#include "GenesisColorDialog.h"
#include "PaletteGridWidget.h"
#include "TileCanvasWidget.h"

#include <QFileDialog>
#include <QFileInfo>
#include <QGridLayout>
#include <QInputDialog>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QRegularExpression>
#include <QScrollArea>
#include <QCloseEvent>
#include <QPainter>
#include <QPixmap>
#include <iostream>


MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , theSettings("github-mwales", "SpriteEditor")
    // All per-tab state moved to respective panel widgets
{
    ui->setupUi(this);

    // Initialize data service
    theDataService.setRom(&theRom);
    theDataService.setDefinition(&theDef);
    theDataService.setCompressor(&theCompressor);

    // Replace the Sprite Viewer tab with the new panel
    theViewerPanel = new SpriteViewerPanel();
    theViewerPanel->setDataService(&theDataService);
    theViewerPanel->setGameDefinition(&theDef);
    int viewerTabIdx = ui->theTabWidget->indexOf(ui->tabSpriteViewer);
    if (viewerTabIdx >= 0)
    {
        ui->theTabWidget->removeTab(viewerTabIdx);
        ui->theTabWidget->insertTab(viewerTabIdx, theViewerPanel, "Sprite Viewer");
    }
    else
    {
        ui->theTabWidget->addTab(theViewerPanel, "Sprite Viewer");
    }

    // Replace the Raw Tile Browser tab with the new panel
    theRawBrowserPanel = new RawTileBrowserPanel();
    theRawBrowserPanel->setDataService(&theDataService);
    int rawTabIdx = ui->theTabWidget->indexOf(ui->tabRawBrowser);
    if (rawTabIdx >= 0)
    {
        ui->theTabWidget->removeTab(rawTabIdx);
        ui->theTabWidget->insertTab(rawTabIdx, theRawBrowserPanel, "Raw Tile Browser");
    }
    else
    {
        ui->theTabWidget->addTab(theRawBrowserPanel, "Raw Tile Browser");
    }

    // Replace the Sprite Editor tab with the new panel
    theEditorPanel = new SpriteEditorPanel();
    theEditorPanel->setRomFile(&theRom);
    theEditorPanel->setGameDefinition(&theDef);
    int editorTabIdx = ui->theTabWidget->indexOf(ui->tabSpriteEditor);
    if (editorTabIdx >= 0)
    {
        ui->theTabWidget->removeTab(editorTabIdx);
        ui->theTabWidget->insertTab(editorTabIdx, theEditorPanel, "Sprite Editor");
    }
    else
    {
        ui->theTabWidget->addTab(theEditorPanel, "Sprite Editor");
    }

    // Replace the Sprite Animations tab with the new panel
    theAnimationPanel = new SpriteAnimationPanel();
    theAnimationPanel->setDataService(&theDataService);
    theAnimationPanel->setRomFile(&theRom);
    theAnimationPanel->setGameDefinition(&theDef);
    int animTabIdx = ui->theTabWidget->indexOf(ui->tabSpriteCollections);
    if (animTabIdx >= 0)
    {
        ui->theTabWidget->removeTab(animTabIdx);
        ui->theTabWidget->insertTab(animTabIdx, theAnimationPanel, "Sprite Animations");
    }
    else
    {
        ui->theTabWidget->addTab(theAnimationPanel, "Sprite Animations");
    }

    // Replace the Screen Captures tab with the new panel
    theScreenCapPanel = new ScreenCapturePanel();
    int capTabIdx = ui->theTabWidget->indexOf(ui->tabScreenCaptures);
    if (capTabIdx >= 0)
    {
        ui->theTabWidget->removeTab(capTabIdx);
        ui->theTabWidget->insertTab(capTabIdx, theScreenCapPanel, "Screen Captures");
    }
    else
    {
        ui->theTabWidget->addTab(theScreenCapPanel, "Screen Captures");
    }

    setupMenus();
    setupConnections();
    loadSettings();
    updateWindowTitle();
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::loadFromCommandLine(const QStringList & args)
{
    // args[0] = program name, rest = ROM path, definition path, or .sprec files
    QString romPath, defPath;
    QStringList sprecPaths;

    for (int i = 1; i < args.size(); ++i)
    {
        QString arg = args[i];
        if (arg.endsWith(".sprec", Qt::CaseInsensitive))
            sprecPaths.append(arg);
        else if (arg.endsWith(".json", Qt::CaseInsensitive))
            defPath = arg;
        else
            romPath = arg;
    }

    if (!romPath.isEmpty())
    {
        if (theRom.openRom(romPath))
        {
            theOriginalRomPath = romPath;
            theSettings.setValue("lastRomPath", romPath);
            updateWindowTitle();
            updateStatusLabel();
            statusBar()->showMessage("ROM loaded: " + theRom.gameTitle());
        }
        else
        {
            statusBar()->showMessage("Failed to open ROM: " + romPath);
        }
    }

    if (!defPath.isEmpty())
    {
        if (theDef.loadFromFile(defPath))
        {
            theSettings.setValue("lastDefPath", defPath);
            updateWindowTitle();
            updateStatusLabel();
            statusBar()->showMessage("Definition loaded: " + theDef.gameName());
        }
        else
        {
            statusBar()->showMessage("Failed to load definition: " + theDef.lastError());
        }
    }

    // Load .sprec files
    for (const QString & sp : sprecPaths)
    {
        SpriteRecording rec;
        if (rec.loadFromFile(sp))
        {
            theAnimationPanel->addRecording(rec);
            statusBar()->showMessage("Recording loaded: " + rec.gameName());
        }
        else
        {
            statusBar()->showMessage("Failed to load .sprec: " + rec.lastError());
        }
    }

    if (theRom.isOpen())
    {
        if (theDef.isLoaded())
            theViewerPanel->populateGrid();
        theRawBrowserPanel->populateRanges();
        theRawBrowserPanel->populatePalettes();
    }
    if (theDef.isLoaded())
    {
        theScreenCapPanel->populateCaptures();
        theAnimationPanel->populateCollections();
    }
}

void MainWindow::setupMenus()
{
    connect(ui->actionOpenRom,      &QAction::triggered, this, &MainWindow::openRom);
    connect(ui->actionOpenGameDef,  &QAction::triggered, this, &MainWindow::openGameDefinition);
    connect(ui->actionSaveRom,      &QAction::triggered, this, &MainWindow::saveRom);
    connect(ui->actionSaveRomAs,    &QAction::triggered, this, &MainWindow::saveRomAs);
    connect(ui->actionQuit,         &QAction::triggered, this, &MainWindow::close);
    connect(ui->actionSaveGameDef, &QAction::triggered, this, &MainWindow::onSaveGameDefinition);
    connect(ui->actionAbout,        &QAction::triggered, this, &MainWindow::showAbout);
}

void MainWindow::setupConnections()
{
    // Sprite Viewer panel (self-contained)
    connect(theViewerPanel, &SpriteViewerPanel::statusMessage,
            this,           [this](const QString & msg){ statusBar()->showMessage(msg); });
    connect(theViewerPanel, &SpriteViewerPanel::editRequested,
            this,           &MainWindow::onViewerEditRequested);
    connect(theViewerPanel, &SpriteViewerPanel::jumpToRawBrowser,
            this,           &MainWindow::onViewerJumpToRaw);
    connect(theViewerPanel, &SpriteViewerPanel::collectionDeleted,
            this,           [this](){ /* nothing extra needed */ });

    // Raw Tile Browser panel (self-contained)
    connect(theRawBrowserPanel, &RawTileBrowserPanel::statusMessage,
            this,               [this](const QString & msg){ statusBar()->showMessage(msg); });
    connect(theRawBrowserPanel, &RawTileBrowserPanel::exportPngRequested,
            this,               &MainWindow::onRawExportPngRequested);

    // Screen Captures panel (self-contained, just wire host signals)
    theScreenCapPanel->setRomFile(&theRom);
    theScreenCapPanel->setGameDefinition(&theDef);
    connect(theScreenCapPanel, &ScreenCapturePanel::statusMessage,
            this,              [this](const QString & msg){ statusBar()->showMessage(msg); });
    connect(theScreenCapPanel, &ScreenCapturePanel::loadCaptureRequested,
            this,              &MainWindow::onScreenCapLoadRequested);
    connect(theScreenCapPanel, &ScreenCapturePanel::removeCaptureRequested,
            this,              &MainWindow::onScreenCapRemoveRequested);
    connect(theScreenCapPanel, &ScreenCapturePanel::saveRomRequested,
            this,              &MainWindow::onScreenCapSaveRomRequested);

    // Sprite Animations panel (self-contained)
    connect(theAnimationPanel, &SpriteAnimationPanel::statusMessage,
            this,              [this](const QString & msg){ statusBar()->showMessage(msg); });
    connect(theAnimationPanel, &SpriteAnimationPanel::loadRecordingRequested,
            this,              &MainWindow::onAnimLoadRecordingRequested);
    connect(theAnimationPanel, &SpriteAnimationPanel::collectionsCaptured,
            this,              [this](){ theViewerPanel->populateGrid(); });

    // Viewer edit/double-click handled internally by SpriteViewerPanel

    // Sprite Editor panel (self-contained)
    theEditorPanel->setRomFile(&theRom);
    theEditorPanel->setGameDefinition(&theDef);
    connect(theEditorPanel, &SpriteEditorPanel::statusMessage,
            this,           [this](const QString & msg){ statusBar()->showMessage(msg); });
    connect(theEditorPanel, &SpriteEditorPanel::editorClosed,
            this,           [this](){ ui->theTabWidget->setCurrentIndex(0); });
    connect(theEditorPanel, &SpriteEditorPanel::colorEditRequested,
            this,           &MainWindow::onEditorColorEditRequested);
    connect(theEditorPanel, &SpriteEditorPanel::spriteDeletedFromGroup,
            this,           &MainWindow::onEditorSpriteDeleted);
}

// ---------------------------------------------------------------------------
// File menu
// ---------------------------------------------------------------------------

void MainWindow::openRom()
{
    QString lastPath = theSettings.value("lastRomPath", "").toString();
    QString path = QFileDialog::getOpenFileName(
        this, "Open Genesis ROM", lastPath,
        "Genesis ROMs (*.bin *.md *.smd *.gen);;All Files (*)");

    if (path.isEmpty())
        return;

    if (!theRom.openRom(path))
    {
        QMessageBox::critical(this, "Error", "Failed to open ROM: " + path);
        return;
    }

    theOriginalRomPath = path;

    if (!theRom.looksLikeGenesisRom())
    {
        QMessageBox::warning(this, "Warning",
            "This file does not appear to be a Genesis ROM (missing SEGA header at 0x100).\n"
            "Continuing anyway.");
    }

    theSettings.setValue("lastRomPath", path);
    updateWindowTitle();
    updateStatusLabel();

    if (theDef.isLoaded())
        theViewerPanel->populateGrid();

    // Enable raw browser with default range even without a definition
    theRawBrowserPanel->populateRanges();
    theRawBrowserPanel->populatePalettes();

    statusBar()->showMessage("ROM loaded: " + theRom.gameTitle());
}

void MainWindow::openGameDefinition()
{
    QString lastPath = theSettings.value("lastDefPath", "").toString();
    QString path = QFileDialog::getOpenFileName(
        this, "Open Game Definition / Recording", lastPath,
        "Game Files (*.json *.sprec);;Game Definition (*.json);;Sprite Recording (*.sprec);;All Files (*)");

    if (path.isEmpty())
        return;

    // Route .sprec files to SpriteRecording loader
    if (path.endsWith(".sprec", Qt::CaseInsensitive))
    {
        SpriteRecording rec;
        if (!rec.loadFromFile(path))
        {
            QMessageBox::critical(this, "Error",
                "Failed to load sprite recording:\n" + rec.lastError());
            return;
        }
        theAnimationPanel->addRecording(rec);
        theAnimationPanel->populateCollections();
        statusBar()->showMessage("Sprite recording loaded: " + rec.gameName());
        return;
    }

    if (!theDef.loadFromFile(path))
    {
        QMessageBox::critical(this, "Error",
            "Failed to load game definition:\n" + theDef.lastError());
        return;
    }

    theSettings.setValue("lastDefPath", path);
    updateWindowTitle();
    updateStatusLabel();

    if (theRom.isOpen())
    {
        theViewerPanel->populateGrid();
        theRawBrowserPanel->populateRanges();
        theRawBrowserPanel->populatePalettes();
    }
    theScreenCapPanel->populateCaptures();
    theAnimationPanel->populateCollections();

    statusBar()->showMessage("Game definition loaded: " + theDef.gameName());
}

bool MainWindow::promptSaveAsIfOriginal()
{
    if (theRom.romPath() != theOriginalRomPath)
        return false;  // Already saving to a different file

    QMessageBox::StandardButton reply = QMessageBox::question(
        this, "Save to Original ROM?",
        "You are about to overwrite the original ROM file:\n" +
        theOriginalRomPath +
        "\n\nWould you like to save to a different file instead?",
        QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel);

    if (reply == QMessageBox::Cancel)
        return true;  // User cancelled — don't save at all

    if (reply == QMessageBox::Yes)
    {
        // Redirect to Save As
        saveRomAs();
        return true;  // Handled via Save As
    }

    return false;  // User chose No — proceed with original file
}

void MainWindow::saveRom()
{
    if (!theRom.isOpen())
        return;

    if (promptSaveAsIfOriginal())
        return;

    if (!theRom.saveRom())
    {
        QMessageBox::critical(this, "Error", "Failed to save ROM.");
        return;
    }
    updateWindowTitle();
    statusBar()->showMessage("ROM saved: " + theRom.romPath());
}

void MainWindow::saveRomAs()
{
    if (!theRom.isOpen())
        return;

    QString path = QFileDialog::getSaveFileName(
        this, "Save ROM As", theRom.romPath(),
        "Genesis ROMs (*.bin *.md);;All Files (*)");

    if (path.isEmpty())
        return;

    if (!theRom.saveRom(path))
    {
        QMessageBox::critical(this, "Error", "Failed to save ROM to: " + path);
        return;
    }
    updateWindowTitle();
    statusBar()->showMessage("ROM saved as: " + path);
}

// ---------------------------------------------------------------------------
// Sprite Viewer tab (legacy mode)
// ---------------------------------------------------------------------------


// Viewer slots (new thin wiring)
void MainWindow::onViewerEditRequested(int collectionIndex, const QMap<int, QString> & palLineToId)
{
    theEditorPanel->editNormalizedCollection(collectionIndex, palLineToId);
    ui->theTabWidget->setCurrentWidget(theEditorPanel);
}

void MainWindow::onViewerJumpToRaw(uint32_t romOffset, int widthTiles, int heightTiles, int palComboIdx)
{
    theRawBrowserPanel->jumpToAddress(romOffset, widthTiles, heightTiles, palComboIdx);
    ui->theTabWidget->setCurrentWidget(theRawBrowserPanel);
}

// ---------------------------------------------------------------------------
// REMOVED: Sprite Viewer (moved to SpriteViewerPanel)
// ---------------------------------------------------------------------------

// ---------------------------------------------------------------------------
// Raw Tile Browser tab (moved to RawTileBrowserPanel)
// ---------------------------------------------------------------------------

void MainWindow::onRawExportPngRequested(const QImage & image, const QString & suggestedName)
{
    QString path = QFileDialog::getSaveFileName(
        this, "Export Sprite as PNG", suggestedName,
        "PNG Images (*.png);;All Files (*)");
    if (path.isEmpty())
        return;
    if (!image.save(path, "PNG"))
    {
        QMessageBox::critical(this, "Export Failed", "Could not save PNG to: " + path);
        return;
    }
    statusBar()->showMessage("Exported sprite to: " + path);
}


// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------





// ---------------------------------------------------------------------------
// Window state
// ---------------------------------------------------------------------------

void MainWindow::updateWindowTitle()
{
    QString title = "Genesis Sprite Editor";
    if (theRom.isOpen())
    {
        QFileInfo fi(theRom.romPath());
        title += " - " + fi.fileName();
        if (theRom.isModified())
            title += " *";
    }
    setWindowTitle(title);
}

void MainWindow::updateStatusLabel()
{
    QString rom = theRom.isOpen()
        ? QFileInfo(theRom.romPath()).fileName()
        : "None";
    QString def = theDef.isLoaded() ? theDef.gameName() : "None";
    ui->theStatusLabel->setText("ROM: " + rom + "   |   Definition: " + def);
}

void MainWindow::loadSettings()
{
    restoreGeometry(theSettings.value("windowGeometry").toByteArray());
    restoreState(theSettings.value("windowState").toByteArray());
    ui->theZoomSpin->setValue(theSettings.value("zoom", 4).toInt());
    ui->theViewerSplitter->restoreState(
        theSettings.value("viewerSplitter").toByteArray());
}

void MainWindow::saveSettings()
{
    theSettings.setValue("windowGeometry", saveGeometry());
    theSettings.setValue("windowState", saveState());
    theSettings.setValue("zoom", ui->theZoomSpin->value());
    theSettings.setValue("viewerSplitter", ui->theViewerSplitter->saveState());
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    if (theRom.isModified())
    {
        QMessageBox::StandardButton reply = QMessageBox::question(
            this, "Unsaved Changes",
            "The ROM has unsaved changes. Save before closing?",
            QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);

        if (reply == QMessageBox::Save)
            saveRom();
        else if (reply == QMessageBox::Cancel)
        {
            event->ignore();
            return;
        }
    }
    saveSettings();
    event->accept();
}

void MainWindow::showAbout()
{
    QMessageBox::about(this, "About Genesis Sprite Editor",
        "<b>Genesis Sprite Editor</b><br><br>"
        "A portable tool for viewing and replacing sprite artwork in Sega Genesis ROMs.<br><br>"
        "Sprite locations are described by a JSON game definition file, "
        "making the tool usable with any Genesis game.<br><br>"
        "Use the <b>Raw Tile Browser</b> tab to identify sprite offsets when "
        "no game definition exists yet.");
}

// ---------------------------------------------------------------------------
// Screen Captures tab
// ---------------------------------------------------------------------------

// Old screen capture methods removed — now in ScreenCapturePanel

void MainWindow::onScreenCapLoadRequested()
{
    QString lastPath = theSettings.value("lastDefPath", "").toString();
    QString path = QFileDialog::getOpenFileName(
        this, "Load Screen Capture", lastPath,
        "Screen Capture JSON (*.json);;All Files (*)");
    if (!path.isEmpty())
        theScreenCapPanel->loadCaptureFile(path);
}

void MainWindow::onScreenCapRemoveRequested(int defIndex, const QString & name)
{
    QMessageBox::StandardButton reply = QMessageBox::question(
        this, "Remove Screen Capture",
        QString("Remove screen capture '%1' from the game definition?").arg(name),
        QMessageBox::Yes | QMessageBox::No);
    if (reply == QMessageBox::Yes)
        theScreenCapPanel->confirmRemoveCapture(defIndex);
}

void MainWindow::onScreenCapSaveRomRequested()
{
    if (!theRom.isOpen())
    {
        statusBar()->showMessage("No ROM open");
        return;
    }
    if (!theRom.isModified())
    {
        statusBar()->showMessage("No ROM changes to save");
        return;
    }
    if (promptSaveAsIfOriginal())
        return;
    if (!theRom.saveRom())
    {
        QMessageBox::critical(this, "Error", "Failed to save ROM.");
        return;
    }
    statusBar()->showMessage("Tile changes saved to ROM");
}

// Removed old methods: onScreenCaptureSelected, onScreenCapZoomChanged,
// onLoadScreenCapture, onAddScreenCaptureToDef, onRemoveScreenCapture,
// onCapEditToggled, onCapColorPicked, onCapSaveToRom, onCapRevert,
// onCapToolChanged, onCapBrushSizeChanged, onCapPaletteSelected,
// onCapTileHovered — all now in ScreenCapturePanel

// Old screen capture slot bodies deleted — now in ScreenCapturePanel
// Screen capture slots moved to ScreenCapturePanel
// See git history (Phase 1 commit) for original implementations

// Animation tab slots moved to SpriteAnimationPanel

void MainWindow::onAnimLoadRecordingRequested()
{
    QString lastPath = theSettings.value("lastDefPath", "").toString();
    QString path = QFileDialog::getOpenFileName(
        this, "Load Sprite Recording", lastPath,
        "Sprite Recording (*.sprec);;All Files (*)");
    if (path.isEmpty())
        return;

    SpriteRecording rec;
    if (!rec.loadFromFile(path))
    {
        QMessageBox::critical(this, "Error",
            "Failed to load sprite recording:\n" + rec.lastError());
        return;
    }
    theAnimationPanel->addRecording(rec);
    theAnimationPanel->populateCollections();
    statusBar()->showMessage("Recording loaded: " + rec.gameName());
}

// ---------------------------------------------------------------------------
// REMOVED: Sprite Collections tab (moved to SpriteAnimationPanel)
// ---------------------------------------------------------------------------


// applyPersistentHidden moved to SpriteAnimationPanel

// ---------------------------------------------------------------------------
// Sprite Editor tab (moved to SpriteEditorPanel)
// ---------------------------------------------------------------------------

// Capture workflow moved to SpriteAnimationPanel

void MainWindow::onSaveGameDefinition()
{
    if (!theDef.isLoaded())
    {
        statusBar()->showMessage("No game definition loaded.");
        return;
    }

    QString path = theDef.definitionPath();
    if (path.isEmpty())
    {
        path = QFileDialog::getSaveFileName(this, "Save Game Definition",
            "", "Game Definition (*.json);;All Files (*)");
        if (path.isEmpty())
            return;
    }

    if (!theDef.saveToFile(path))
    {
        QMessageBox::critical(this, "Save Failed",
            "Failed to save game definition:\n" + theDef.lastError());
        return;
    }

    statusBar()->showMessage("Game definition saved: " + path);
}

// Viewer methods moved to SpriteViewerPanel

void MainWindow::onEditorColorEditRequested(int paletteIndex, const QColor & current,
                                              bool hasRomOffset, uint32_t romOffset, int refCount)
{
    Q_UNUSED(hasRomOffset);
    Q_UNUSED(romOffset);
    Q_UNUSED(refCount);

    GenesisColorDialog dlg(current, paletteIndex, this);
    if (dlg.exec() != QDialog::Accepted)
        return;

    theEditorPanel->applyColorEdit(paletteIndex, dlg.selectedColor(), dlg.selectedCramWord());
}

void MainWindow::onEditorSpriteDeleted(int collectionIndex)
{
    Q_UNUSED(collectionIndex);
    theViewerPanel->populateGrid();
}

