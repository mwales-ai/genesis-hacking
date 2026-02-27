#include "MainWindow.h"
#include "ui_MainWindow.h"
#include "SpriteSheetWidget.h"
#include "RawTileBrowserWidget.h"
#include "TileCanvasWidget.h"
#include "PaletteWidget.h"
#include "SpriteReplaceDialog.h"

#include <QFileDialog>
#include <QFileInfo>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QRegularExpression>
#include <QScrollArea>
#include <QVBoxLayout>
#include <QCloseEvent>
#include <iostream>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , theSettings("github-mwales", "SpriteEditor")
    , theCurrentGroupIndex(-1)
    , theCurrentSpriteIndex(-1)
{
    ui->setupUi(this);
    setupCustomWidgets();
    setupMenus();
    setupConnections();
    loadSettings();
    updateWindowTitle();
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::setupCustomWidgets()
{
    // --- Sprite Viewer Tab ---

    // SpriteSheetWidget inside the scroll area
    theSpriteSheet = new SpriteSheetWidget();
    ui->spriteScrollContents->layout()->addWidget(theSpriteSheet);

    // PaletteWidget inside the palette placeholder
    thePaletteDisplay = new PaletteWidget();
    ui->thePalettePlaceholder->layout()->addWidget(thePaletteDisplay);

    // TileCanvasWidget inside the detail placeholder
    theSpriteDetail = new TileCanvasWidget();
    ui->theDetailPlaceholder->layout()->addWidget(theSpriteDetail);

    // --- Raw Tile Browser Tab ---
    theRawBrowser = new RawTileBrowserWidget();
    ui->rawScrollContents->layout()->addWidget(theRawBrowser);
}

void MainWindow::setupMenus()
{
    connect(ui->actionOpenRom,      &QAction::triggered, this, &MainWindow::openRom);
    connect(ui->actionOpenGameDef,  &QAction::triggered, this, &MainWindow::openGameDefinition);
    connect(ui->actionSaveRom,      &QAction::triggered, this, &MainWindow::saveRom);
    connect(ui->actionSaveRomAs,    &QAction::triggered, this, &MainWindow::saveRomAs);
    connect(ui->actionQuit,         &QAction::triggered, this, &MainWindow::close);
    connect(ui->actionReplaceSprite,&QAction::triggered, this, &MainWindow::replaceSprite);
    connect(ui->actionAbout,        &QAction::triggered, this, &MainWindow::showAbout);
}

void MainWindow::setupConnections()
{
    connect(ui->theGroupCombo,  SIGNAL(currentIndexChanged(int)),
            this,               SLOT(onSpriteGroupChanged(int)));
    connect(theSpriteSheet,     &SpriteSheetWidget::spriteSelected,
            this,               &MainWindow::onSpriteSelected);
    connect(ui->theZoomSpin,    SIGNAL(valueChanged(int)),
            this,               SLOT(onZoomChanged(int)));
    connect(ui->theGridCheck,   &QCheckBox::toggled,
            this,               &MainWindow::onGridToggled);
    connect(ui->theReplaceButton, &QPushButton::clicked,
            this,               &MainWindow::replaceSprite);
    connect(ui->theExportButton, &QPushButton::clicked,
            this,               &MainWindow::exportSprite);

    connect(ui->theRawRangeCombo,   SIGNAL(currentIndexChanged(int)),
            this,                   SLOT(onRawRangeChanged(int)));
    connect(ui->theRawPaletteCombo, SIGNAL(currentIndexChanged(int)),
            this,                   SLOT(onRawPaletteChanged(int)));
    connect(ui->theRawZoomSpin,     SIGNAL(valueChanged(int)),
            this,                   SLOT(onRawZoomChanged(int)));
    connect(theRawBrowser,          &RawTileBrowserWidget::tileClicked,
            this,                   &MainWindow::onRawTileClicked);
    connect(ui->theJumpButton,      &QPushButton::clicked,
            this,                   &MainWindow::onJumpToOffset);
    connect(ui->theJumpOffsetEdit,  &QLineEdit::returnPressed,
            this,                   &MainWindow::onJumpToOffset);
    connect(ui->theRawSpriteWSpin,  SIGNAL(valueChanged(int)),
            this,                   SLOT(onRawSpriteSizeChanged(int)));
    connect(ui->theRawSpriteHSpin,  SIGNAL(valueChanged(int)),
            this,                   SLOT(onRawSpriteSizeChanged(int)));
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
        populateSpriteGroups();

    // Enable raw browser with default range even without a definition
    populateRawRanges();
    populateRawPalettes();

    statusBar()->showMessage("ROM loaded: " + theRom.gameTitle());
}

void MainWindow::openGameDefinition()
{
    QString lastPath = theSettings.value("lastDefPath", "").toString();
    QString path = QFileDialog::getOpenFileName(
        this, "Open Game Definition", lastPath,
        "Game Definition (*.json);;All Files (*)");

    if (path.isEmpty())
        return;

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
        populateSpriteGroups();
        populateRawRanges();
        populateRawPalettes();
    }

    statusBar()->showMessage("Game definition loaded: " + theDef.gameName());
}

void MainWindow::saveRom()
{
    if (!theRom.isOpen())
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
// Sprite Viewer tab
// ---------------------------------------------------------------------------

void MainWindow::populateSpriteGroups()
{
    ui->theGroupCombo->blockSignals(true);
    ui->theGroupCombo->clear();

    const auto & groups = theDef.spriteGroups();
    for (const auto & g : groups)
        ui->theGroupCombo->addItem(g.name);

    ui->theGroupCombo->setEnabled(!groups.isEmpty());
    ui->theGroupCombo->blockSignals(false);

    if (!groups.isEmpty())
    {
        int restoreIdx = theSettings.value("lastGroupIndex", 0).toInt();
        restoreIdx = qBound(0, restoreIdx, groups.size() - 1);
        ui->theGroupCombo->setCurrentIndex(restoreIdx);
        onSpriteGroupChanged(restoreIdx);
    }
}

void MainWindow::onSpriteGroupChanged(int index)
{
    if (!theDef.isLoaded() || !theRom.isOpen() || index < 0)
        return;
    if (index >= theDef.spriteGroups().size())
        return;

    theCurrentGroupIndex  = index;
    theCurrentSpriteIndex = -1;
    theSettings.setValue("lastGroupIndex", index);

    displaySpriteGroup(index);
}

void MainWindow::displaySpriteGroup(int groupIndex)
{
    const SpriteGroup & group = theDef.spriteGroups()[groupIndex];
    QVector<SpriteThumb> thumbs;

    for (int i = 0; i < group.sprites.size(); ++i)
    {
        const SpriteEntry & entry = group.sprites[i];
        QByteArray tileData = fetchTileData(entry);
        if (tileData.isEmpty())
            continue;

        GenesisPalette pal = paletteForSprite(entry, groupIndex);
        QImage img = TileDecoder::decodeSprite(tileData,
                                               entry.widthTiles,
                                               entry.heightTiles,
                                               pal);
        SpriteThumb thumb;
        thumb.image       = img;
        thumb.name        = entry.name;
        thumb.groupIndex  = groupIndex;
        thumb.spriteIndex = i;
        thumbs.append(thumb);
    }

    theSpriteSheet->setSprites(thumbs);
    theSpriteDetail->clearSprite();
    ui->theSpriteName->setText(group.name);
    ui->theOffsetLabel->clear();
    ui->theReplaceButton->setEnabled(false);
    ui->theExportButton->setEnabled(false);
    ui->actionReplaceSprite->setEnabled(false);

    // Show palette of first sprite in group
    if (!group.palettes.isEmpty())
    {
        QByteArray palData = theRom.readBytes(group.palettes[0].romOffset, 32);
        thePaletteDisplay->setPalette(TileDecoder::decodePalette(palData));
    }
}

void MainWindow::onSpriteSelected(int groupIdx, int spriteIdx)
{
    theCurrentGroupIndex  = groupIdx;
    theCurrentSpriteIndex = spriteIdx;
    displaySpriteDetail(groupIdx, spriteIdx);
}

void MainWindow::displaySpriteDetail(int groupIdx, int spriteIdx)
{
    if (!theDef.isLoaded() || !theRom.isOpen())
        return;
    if (groupIdx < 0 || groupIdx >= theDef.spriteGroups().size())
        return;

    const SpriteGroup & group  = theDef.spriteGroups()[groupIdx];
    if (spriteIdx < 0 || spriteIdx >= group.sprites.size())
        return;

    const SpriteEntry & entry = group.sprites[spriteIdx];
    QByteArray tileData = fetchTileData(entry);
    GenesisPalette pal  = paletteForSprite(entry, groupIdx);

    QImage img = TileDecoder::decodeSprite(tileData,
                                           entry.widthTiles,
                                           entry.heightTiles,
                                           pal);
    theSpriteDetail->setSprite(img);
    theSpriteDetail->setZoom(ui->theZoomSpin->value());
    theSpriteDetail->setShowGrid(ui->theGridCheck->isChecked());

    thePaletteDisplay->setPalette(pal);

    ui->theSpriteName->setText(entry.name);
    ui->theOffsetLabel->setText(
        QString("ROM 0x%1  |  %2×%3 tiles  |  %4 bytes  |  [%5]")
        .arg(entry.romOffset, 6, 16, QChar('0')).toUpper()
        .arg(entry.widthTiles)
        .arg(entry.heightTiles)
        .arg(entry.widthTiles * entry.heightTiles * 32)
        .arg(entry.compression));

    ui->theReplaceButton->setEnabled(true);
    ui->theExportButton->setEnabled(true);
    ui->actionReplaceSprite->setEnabled(true);
}

void MainWindow::onZoomChanged(int value)
{
    theSpriteDetail->setZoom(value);
}

void MainWindow::onGridToggled(bool checked)
{
    theSpriteDetail->setShowGrid(checked);
}

void MainWindow::replaceSprite()
{
    if (!theDef.isLoaded() || !theRom.isOpen())
        return;
    if (theCurrentGroupIndex < 0 || theCurrentSpriteIndex < 0)
        return;

    const SpriteGroup & group  = theDef.spriteGroups()[theCurrentGroupIndex];
    const SpriteEntry & entry  = group.sprites[theCurrentSpriteIndex];
    GenesisPalette pal = paletteForSprite(entry, theCurrentGroupIndex);

    if (entry.compression != "none")
    {
        QMessageBox::warning(this, "Compression Not Supported",
            QString("This sprite uses '%1' compression.\n\n"
                    "Direct replacement is only supported for sprites with "
                    "compression: \"none\".\n\n"
                    "To replace this sprite:\n"
                    "1. Locate the uncompressed tile data in the Raw Tile Browser\n"
                    "2. Update the JSON definition to use the uncompressed offset with "
                    "compression: \"none\"").arg(entry.compression));
        return;
    }

    SpriteReplaceDialog dlg(entry.widthTiles, entry.heightTiles, pal, this);
    if (dlg.exec() != QDialog::Accepted)
        return;

    QByteArray encoded = dlg.encodedTileData();
    if (encoded.isEmpty())
    {
        QMessageBox::critical(this, "Error", "Failed to encode replacement sprite.");
        return;
    }

    if (!theRom.writeBytes(entry.romOffset, encoded))
    {
        QMessageBox::critical(this, "Error",
            QString("Failed to write %1 bytes to ROM at offset 0x%2")
            .arg(encoded.size())
            .arg(entry.romOffset, 6, 16, QChar('0')).toUpper());
        return;
    }

    // Refresh display
    displaySpriteDetail(theCurrentGroupIndex, theCurrentSpriteIndex);
    displaySpriteGroup(theCurrentGroupIndex);
    updateWindowTitle();
    statusBar()->showMessage("Sprite replaced. Use File > Save ROM to save changes.");
}

void MainWindow::exportSprite()
{
    const QImage & img = theSpriteDetail->sprite();
    if (img.isNull())
        return;

    QString suggestedName;
    if (theDef.isLoaded() && theCurrentGroupIndex >= 0 && theCurrentSpriteIndex >= 0)
    {
        const SpriteEntry & e = theDef.spriteGroups()[theCurrentGroupIndex].sprites[theCurrentSpriteIndex];
        suggestedName = e.name;
        suggestedName.replace(QRegularExpression("[^A-Za-z0-9_\\-]"), "_");
        suggestedName += ".png";
    }
    else
    {
        suggestedName = "sprite.png";
    }

    QString path = QFileDialog::getSaveFileName(
        this, "Export Sprite as PNG", suggestedName,
        "PNG Images (*.png);;All Files (*)");

    if (path.isEmpty())
        return;

    if (!img.save(path, "PNG"))
    {
        QMessageBox::critical(this, "Export Failed", "Could not save PNG to: " + path);
        return;
    }

    statusBar()->showMessage("Exported sprite to: " + path);
}

// ---------------------------------------------------------------------------
// Raw Tile Browser tab
// ---------------------------------------------------------------------------

void MainWindow::populateRawRanges()
{
    ui->theRawRangeCombo->blockSignals(true);
    ui->theRawRangeCombo->clear();

    if (theDef.isLoaded())
    {
        for (const auto & r : theDef.tileRanges())
            ui->theRawRangeCombo->addItem(r.label);
    }
    else if (theRom.isOpen())
    {
        // Default range when no definition loaded
        ui->theRawRangeCombo->addItem(
            QString("Full ROM (0x200 - 0x%1)")
            .arg(theRom.romSize(), 0, 16).toUpper());
    }

    ui->theRawRangeCombo->setEnabled(ui->theRawRangeCombo->count() > 0);
    ui->theRawRangeCombo->blockSignals(false);
    ui->theJumpOffsetEdit->setEnabled(theRom.isOpen());
    ui->theJumpButton->setEnabled(theRom.isOpen());
    ui->theRawSpriteWSpin->setEnabled(theRom.isOpen());
    ui->theRawSpriteHSpin->setEnabled(theRom.isOpen());

    if (ui->theRawRangeCombo->count() > 0)
        refreshRawBrowser();
}

void MainWindow::populateRawPalettes()
{
    ui->theRawPaletteCombo->blockSignals(true);
    ui->theRawPaletteCombo->clear();

    ui->theRawPaletteCombo->addItem("Greyscale (default)");

    if (theDef.isLoaded())
    {
        for (const auto & g : theDef.spriteGroups())
        {
            for (const auto & pal : g.palettes)
                ui->theRawPaletteCombo->addItem(
                    QString("%1 / %2").arg(g.name, pal.name));
        }
    }

    ui->theRawPaletteCombo->setEnabled(theRom.isOpen());
    ui->theRawPaletteCombo->blockSignals(false);
}

void MainWindow::onRawRangeChanged(int)
{
    refreshRawBrowser();
}

void MainWindow::onRawPaletteChanged(int)
{
    refreshRawBrowser();
}

void MainWindow::onRawZoomChanged(int value)
{
    theRawBrowser->setZoom(value);
}

void MainWindow::onRawSpriteSizeChanged(int)
{
    theRawBrowser->setSpriteSize(ui->theRawSpriteWSpin->value(),
                                 ui->theRawSpriteHSpin->value());
}

void MainWindow::onRawTileClicked(int tileIndex, uint32_t romOffset)
{
    int w = ui->theRawSpriteWSpin->value();
    int h = ui->theRawSpriteHSpin->value();
    int tilesPerSprite = w * h;
    QString label;
    if (tilesPerSprite == 1)
    {
        label = QString("Tile %1  |  ROM offset: 0x%2")
            .arg(tileIndex)
            .arg(romOffset, 6, 16, QChar('0')).toUpper();
    }
    else
    {
        label = QString("Sprite %1  (tiles %2–%3, %4×%5)  |  ROM offset: 0x%6")
            .arg(tileIndex / tilesPerSprite)
            .arg(tileIndex)
            .arg(tileIndex + tilesPerSprite - 1)
            .arg(w).arg(h)
            .arg(romOffset, 6, 16, QChar('0')).toUpper();
    }
    ui->theRawTileInfoLabel->setText(label);
}

void MainWindow::onJumpToOffset()
{
    QString text = ui->theJumpOffsetEdit->text().trimmed();
    bool ok = false;
    uint32_t targetOffset = text.toUInt(&ok, 16);
    if (!ok)
        targetOffset = text.toUInt(&ok, 0);  // also accept decimal
    if (!ok) {
        ui->theRawTileInfoLabel->setText("Jump failed: enter a valid hex offset (e.g. 0x032D80)");
        return;
    }

    // Determine the current range's start offset
    int rangeIdx = ui->theRawRangeCombo->currentIndex();
    uint32_t rangeStart = 0x200;
    if (theDef.isLoaded() && rangeIdx < theDef.tileRanges().size()) {
        rangeStart = theDef.tileRanges()[rangeIdx].startOffset;
    }

    if (targetOffset < rangeStart) {
        ui->theRawTileInfoLabel->setText(
            QString("0x%1 is before the current range start (0x%2). Switch ranges.")
            .arg(targetOffset, 6, 16, QChar('0')).toUpper()
            .arg(rangeStart, 6, 16, QChar('0')).toUpper());
        return;
    }

    uint32_t byteOffset = targetOffset - rangeStart;
    int tileIndex = int(byteOffset / 32);
    if (tileIndex >= theRawBrowser->tileCount()) {
        ui->theRawTileInfoLabel->setText(
            QString("0x%1 is beyond the current range end.")
            .arg(targetOffset, 6, 16, QChar('0')).toUpper());
        return;
    }

    theRawBrowser->scrollToTile(tileIndex);
    uint32_t actualOffset = rangeStart + uint32_t(tileIndex) * 32;
    ui->theRawTileInfoLabel->setText(
        QString("Jumped to tile %1  |  ROM offset: 0x%2")
        .arg(tileIndex)
        .arg(actualOffset, 6, 16, QChar('0')).toUpper());
}

void MainWindow::refreshRawBrowser()
{
    if (!theRom.isOpen())
        return;

    // Determine range
    uint32_t startOffset = 0x200;
    uint32_t endOffset   = (uint32_t)theRom.romSize();

    int rangeIdx = ui->theRawRangeCombo->currentIndex();
    if (theDef.isLoaded() && rangeIdx >= 0 &&
        rangeIdx < theDef.tileRanges().size())
    {
        const TileRange & r = theDef.tileRanges()[rangeIdx];
        startOffset = r.startOffset;
        endOffset   = qMin(r.endOffset, (uint32_t)theRom.romSize());
    }

    if (endOffset <= startOffset)
        return;

    uint32_t length = endOffset - startOffset;
    // Clamp to a reasonable browser size (max 256 KB = 8192 tiles at once)
    const uint32_t MAX_BROWSE_BYTES = 256 * 1024;
    if (length > MAX_BROWSE_BYTES)
        length = MAX_BROWSE_BYTES;

    // Align to 32-byte tile boundary
    length = (length / 32) * 32;

    QByteArray tileData = theRom.readBytes(startOffset, length);

    // Determine palette
    GenesisPalette pal = TileDecoder::greyPalette();
    int palIdx = ui->theRawPaletteCombo->currentIndex();
    if (palIdx > 0 && theDef.isLoaded())
    {
        // palIdx 0 = greyscale, 1+ = from definition
        int flatIdx = 1;
        bool found = false;
        for (const auto & g : theDef.spriteGroups())
        {
            for (const auto & p : g.palettes)
            {
                if (flatIdx == palIdx)
                {
                    QByteArray palData = theRom.readBytes(p.romOffset, 32);
                    pal = TileDecoder::decodePalette(palData);
                    found = true;
                    break;
                }
                ++flatIdx;
            }
            if (found) break;
        }
    }

    theRawBrowser->setTileData(tileData, startOffset, pal);
    theRawBrowser->setZoom(ui->theRawZoomSpin->value());

    ui->theRawTileInfoLabel->setText(
        QString("Showing %1 tiles from ROM 0x%2 — 0x%3  |  Click a tile for its offset")
        .arg(tileData.size() / 32)
        .arg(startOffset, 6, 16, QChar('0')).toUpper()
        .arg(startOffset + tileData.size(), 6, 16, QChar('0')).toUpper());
}

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

QByteArray MainWindow::fetchTileData(const SpriteEntry & entry)
{
    int rawBytes = entry.widthTiles * entry.heightTiles * 32;
    // Pass everything from the entry offset to end-of-ROM; decompressor reads only what it needs
    QByteArray romSlice = theRom.readBytes(entry.romOffset,
                                           theRom.romSize() - entry.romOffset);
    uint32_t consumed = 0;
    QByteArray result = theCompressor.decompress(
        entry.compression, romSlice, 0, rawBytes, &consumed);

    AppDebug << "fetchTileData: " << entry.name.toStdString()
             << " @ 0x" << std::hex << entry.romOffset
             << " -> " << std::dec << result.size() << " bytes" << std::endl;
    return result;
}

GenesisPalette MainWindow::paletteForSprite(const SpriteEntry & entry, int groupIndex)
{
    if (!theDef.isLoaded())
        return TileDecoder::greyPalette();

    const SpriteGroup & group = theDef.spriteGroups()[groupIndex];
    int palIdx = qBound(0, entry.paletteIndex, group.palettes.size() - 1);

    if (palIdx < 0 || group.palettes.isEmpty())
        return TileDecoder::greyPalette();

    const PaletteDefinition & palDef = group.palettes[palIdx];
    QByteArray palData = theRom.readBytes(palDef.romOffset, 32);
    if (palData.size() < 32)
        return TileDecoder::greyPalette();

    return TileDecoder::decodePalette(palData);
}

// ---------------------------------------------------------------------------
// Window state
// ---------------------------------------------------------------------------

void MainWindow::updateWindowTitle()
{
    QString title = "Genesis Sprite Editor";
    if (theRom.isOpen())
    {
        QFileInfo fi(theRom.romPath());
        title += " — " + fi.fileName();
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
    ui->theGridCheck->setChecked(theSettings.value("showGrid", false).toBool());
}

void MainWindow::saveSettings()
{
    theSettings.setValue("windowGeometry", saveGeometry());
    theSettings.setValue("windowState", saveState());
    theSettings.setValue("zoom", ui->theZoomSpin->value());
    theSettings.setValue("showGrid", ui->theGridCheck->isChecked());
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
