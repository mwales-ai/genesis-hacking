#include "MainWindow.h"
#include "ui_MainWindow.h"
#include "SpritePixelEditor.h"
#include "GenesisColorDialog.h"

#include <QFileDialog>
#include <QFileInfo>
#include <QInputDialog>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QRegularExpression>
#include <QScrollArea>
#include <QCloseEvent>
#include <QPainter>
#include <iostream>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , theSettings("github-mwales", "SpriteEditor")
    , theCurrentGroupIndex(-1)
    , theCurrentSpriteIndex(-1)
    , theCurrentFrameIndex(0)
    , theSelectedCollectionIndex(-1)
    , theShowSpriteBorders(false)
    , theRawSelectedTileIndex(-1)
    , theRawSelectedRomOffset(0)
    , theCollectionCount(0)
    , theAnimationCount(0)
    , theActiveAnimIndex(-1)
    , theActiveRecIndex(-1)
    , theEditCollectionIndex(-1)
    , theEditSpriteIndex(-1)
    , theEditorActivePaletteLine(0)
    , theCaptureCounter(0)
{
    ui->setupUi(this);
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
            theSpriteRecordings.append(rec);
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
            populateCollectionGrid();
        populateRawRanges();
        populateRawPalettes();
    }
    if (theDef.isLoaded() || !theSpriteRecordings.isEmpty())
    {
        populateScreenCaptures();
        populateSpriteCollections();
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
    // Sprite Viewer: collection grid + detail
    connect(ui->theSpriteSheet, &SpriteSheetWidget::spriteSelected,
            this,               &MainWindow::onCollectionGridSelected);
    connect(ui->theZoomSpin,    SIGNAL(valueChanged(int)),
            this,               SLOT(onZoomChanged(int)));
    connect(ui->theThumbZoomSpin, SIGNAL(valueChanged(int)),
            this,                 SLOT(onThumbZoomChanged(int)));
    connect(ui->theSpriteSheet, &SpriteSheetWidget::spriteReordered,
            this,               &MainWindow::onViewerSpriteReordered);
    connect(ui->theViewerNameEdit, &QLineEdit::editingFinished,
            this,               &MainWindow::onViewerNameEditFinished);
    connect(ui->theViewerBordersCheck, &QCheckBox::toggled,
            this,               &MainWindow::onViewerBordersToggled);

    connect(ui->theRawRangeCombo,   SIGNAL(currentIndexChanged(int)),
            this,                   SLOT(onRawRangeChanged(int)));
    connect(ui->theRawPaletteCombo, SIGNAL(currentIndexChanged(int)),
            this,                   SLOT(onRawPaletteChanged(int)));
    connect(ui->theRawZoomSpin,     SIGNAL(valueChanged(int)),
            this,                   SLOT(onRawZoomChanged(int)));
    connect(ui->theRawBrowser,      &RawTileBrowserWidget::tileClicked,
            this,                   &MainWindow::onRawTileClicked);
    connect(ui->theJumpButton,      &QPushButton::clicked,
            this,                   &MainWindow::onJumpToOffset);
    connect(ui->theJumpOffsetEdit,  &QLineEdit::returnPressed,
            this,                   &MainWindow::onJumpToOffset);
    connect(ui->theRawSpriteWSpin,  SIGNAL(valueChanged(int)),
            this,                   SLOT(onRawSpriteSizeChanged(int)));
    connect(ui->theRawSpriteHSpin,  SIGNAL(valueChanged(int)),
            this,                   SLOT(onRawSpriteSizeChanged(int)));
    connect(ui->theAssemblyStartButton, &QPushButton::clicked,
            this,                       &MainWindow::onSetAssemblyStart);
    connect(ui->theRawExportButton, &QPushButton::clicked,
            this,                   &MainWindow::onRawExportPng);

    // Screen Captures tab
    connect(ui->theScreenCapCombo,    SIGNAL(currentIndexChanged(int)),
            this,                     SLOT(onScreenCaptureSelected(int)));
    connect(ui->theScreenCapZoomSpin, SIGNAL(valueChanged(int)),
            this,                     SLOT(onScreenCapZoomChanged(int)));

    // Sprite Collections tab
    connect(ui->theSpriteColCombo,    SIGNAL(currentIndexChanged(int)),
            this,                     SLOT(onSpriteCollectionSelected(int)));
    connect(ui->theSpriteColZoomSpin, SIGNAL(valueChanged(int)),
            this,                     SLOT(onSpriteCollectionZoomChanged(int)));
    connect(ui->theSpriteColWidget,   &SpriteCollectionWidget::spriteClicked,
            this,                     &MainWindow::onCollectionSpriteClicked);
    connect(ui->theSpriteColWidget,   &SpriteCollectionWidget::selectionChanged,
            this,                     &MainWindow::onCollectionSelectionChanged);
    connect(ui->theColFrameSpin,      SIGNAL(valueChanged(int)),
            this,                     SLOT(onAnimationFrameChanged(int)));

    // Load recording button on Sprite Animations tab
    connect(ui->theLoadRecordingButton, &QPushButton::clicked,
            this,                       &MainWindow::onLoadRecording);

    // Capture workflow buttons
    connect(ui->theCaptureGroupButton,  &QPushButton::clicked,
            this,                       &MainWindow::onCaptureSpriteGroup);
    connect(ui->theHideSpritesButton,   &QPushButton::clicked,
            this,                       &MainWindow::onHideSelectedSprites);
    connect(ui->theUnhideSpritesButton, &QPushButton::clicked,
            this,                       &MainWindow::onUnhideSelectedSprites);
    connect(ui->theUnhideSelectedButton, &QPushButton::clicked,
            this,                       &MainWindow::onUnhideSelectedOnly);

    // Sprite Viewer: edit, double-click
    connect(ui->theEditFromViewerButton, &QPushButton::clicked,
            this,                       &MainWindow::onEditFromViewer);
    connect(ui->theSpriteSheet,         &SpriteSheetWidget::spriteDoubleClicked,
            this,                       &MainWindow::onViewerSpriteDoubleClicked);
    connect(ui->theSpriteDetail,        &TileCanvasWidget::doubleClicked,
            this,                       &MainWindow::onDetailDoubleClicked);

    // Sprite Editor tab
    connect(ui->theSpritePixelEditor,      &SpritePixelEditor::groupPaletteLineChanged,
            this,                          &MainWindow::onEditorGroupPaletteLineChanged);
    connect(ui->theEditorPalette,          &PaletteWidget::colorSelected,
            this,                          &MainWindow::onEditorPaletteSelected);
    connect(ui->theEditorPalette,          &PaletteWidget::colorEditRequested,
            this,                          &MainWindow::onEditorPaletteEditRequested);
    connect(ui->theEditorSaveButton,       &QPushButton::clicked,
            this,                          &MainWindow::onEditorSave);
    connect(ui->theEditorSavePaletteButton, &QPushButton::clicked,
            this,                          &MainWindow::onEditorSavePalette);
    connect(ui->theEditorCloseButton,      &QPushButton::clicked,
            this,                          &MainWindow::onEditorClose);
    connect(ui->theEditorZoomSpin,  SIGNAL(valueChanged(int)),
            this,                   SLOT(onEditorZoomChanged(int)));
    connect(ui->theEditorGridCheck, &QCheckBox::toggled,
            this,                   &MainWindow::onEditorGridToggled);
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
        populateCollectionGrid();

    // Enable raw browser with default range even without a definition
    populateRawRanges();
    populateRawPalettes();

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
        theSpriteRecordings.append(rec);
        populateSpriteCollections();
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
        populateCollectionGrid();
        populateRawRanges();
        populateRawPalettes();
    }
    populateScreenCaptures();
    populateSpriteCollections();

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
// Sprite Viewer tab (legacy mode)
// ---------------------------------------------------------------------------

void MainWindow::populateSpriteGroups()
{
    // Legacy mode: no longer has a combo box UI.
    // Sprite groups are still accessible via the Sprite Collections tab.
}

// ---------------------------------------------------------------------------
// Sprite Viewer: Collection Grid
// ---------------------------------------------------------------------------

void MainWindow::populateCollectionGrid()
{
    theSelectedCollectionIndex = -1;

    if (!theDef.isLoaded() || !theDef.isNormalized() || !theRom.isOpen())
    {
        ui->theSpriteSheet->clearSprites();
        ui->theSpriteDetail->clearSprite();
        ui->theViewerNameEdit->clear();
        ui->theViewerInfoLabel->setText("No collections available");
        ui->theEditFromViewerButton->setEnabled(false);
        return;
    }

    const auto & normCols = theDef.normalizedCollections();
    QVector<SpriteThumb> thumbs;

    for (int i = 0; i < normCols.size(); ++i)
    {
        const NormalizedCollection & norm = normCols[i];
        QImage composite = renderCollectionComposite(norm, false);
        if (composite.isNull())
        {
            // Create a small placeholder
            composite = QImage(8, 8, QImage::Format_ARGB32);
            composite.fill(Qt::darkGray);
        }

        SpriteThumb thumb;
        thumb.image = composite;
        thumb.name = norm.name;
        thumb.groupIndex = i;
        thumb.spriteIndex = 0;
        thumb.frameIndex = 0;
        thumbs.append(thumb);
    }

    ui->theSpriteSheet->setSprites(thumbs);
    ui->theSpriteDetail->clearSprite();
    ui->theViewerNameEdit->clear();
    ui->theViewerInfoLabel->setText(
        QString("%1 collections loaded").arg(normCols.size()));
    ui->theEditFromViewerButton->setEnabled(false);
}

QImage MainWindow::renderCollectionComposite(const NormalizedCollection & norm, bool showBorders)
{
    SpriteCollection col = buildFromNormalized(norm);

    int imgW = col.boundingBox.width();
    int imgH = col.boundingBox.height();
    if (imgW <= 0 || imgH <= 0)
        return QImage();

    int originX = col.boundingBox.x();
    int originY = col.boundingBox.y();

    QImage composite(imgW, imgH, QImage::Format_ARGB32);
    composite.fill(Qt::transparent);

    // Render sprites in reverse order (last = back, first = front)
    for (int i = col.sprites.size() - 1; i >= 0; --i)
    {
        const CollectionSprite & cs = col.sprites[i];
        int palLine = qBound(0, cs.paletteLine, 3);

        GenesisPalette pal = TileDecoder::greyPalette();
        if (palLine < col.palettes.size() && !col.palettes[palLine].cramValues.isEmpty())
            pal = TileDecoder::decodePaletteFromCram(col.palettes[palLine].cramValues);

        QByteArray tileData;
        if (!cs.romOffset.isEmpty() && theRom.isOpen())
        {
            bool ok = false;
            QString offsetStr = cs.romOffset;
            if (offsetStr.startsWith("0x") || offsetStr.startsWith("0X"))
                offsetStr = offsetStr.mid(2);
            uint32_t offset = offsetStr.toUInt(&ok, 16);
            if (ok)
            {
                int totalBytes = cs.widthTiles * cs.heightTiles * 32;
                tileData = theRom.readBytes(offset, totalBytes);
            }
        }
        else if (!cs.tileData.isEmpty())
        {
            tileData = cs.tileData;
        }

        if (tileData.isEmpty())
            continue;

        QImage sprImg = TileDecoder::decodeSprite(
            tileData, cs.widthTiles, cs.heightTiles, pal);
        if (cs.hFlip || cs.vFlip)
            sprImg = sprImg.mirrored(cs.hFlip, cs.vFlip);

        int destX = cs.x - originX;
        int destY = cs.y - originY;
        QPainter painter(&composite);
        painter.drawImage(destX, destY, sprImg);

        if (showBorders)
        {
            // Border colors per palette line: yellow, cyan, magenta, green
            static const QColor borderColors[4] = {
                QColor(255, 255, 0),
                QColor(0, 255, 255),
                QColor(255, 0, 255),
                QColor(0, 255, 0)
            };
            int colorIdx = qBound(0, palLine, 3);
            QPen pen(borderColors[colorIdx]);
            pen.setWidth(1);
            painter.setPen(pen);
            painter.setBrush(Qt::NoBrush);
            int sprW = cs.widthTiles * 8;
            int sprH = cs.heightTiles * 8;
            painter.drawRect(destX, destY, sprW - 1, sprH - 1);
        }
    }

    return composite;
}

void MainWindow::onCollectionGridSelected(int groupIndex, int /*spriteIndex*/, int /*frameIndex*/)
{
    theSelectedCollectionIndex = groupIndex;
    updateCollectionDetail(groupIndex);
}

void MainWindow::updateCollectionDetail(int collectionIndex)
{
    const auto & normCols = theDef.normalizedCollections();
    if (collectionIndex < 0 || collectionIndex >= normCols.size())
        return;

    const NormalizedCollection & norm = normCols[collectionIndex];

    // Update name field
    ui->theViewerNameEdit->setText(norm.name);

    // Render composite with optional borders
    QImage composite = renderCollectionComposite(norm, theShowSpriteBorders);
    if (!composite.isNull())
    {
        ui->theSpriteDetail->setSprite(composite);
        ui->theSpriteDetail->setZoom(ui->theZoomSpin->value());
    }
    else
    {
        ui->theSpriteDetail->clearSprite();
    }

    // Build metadata info
    SpriteCollection col = buildFromNormalized(norm);
    int imgW = col.boundingBox.width();
    int imgH = col.boundingBox.height();

    // Count ROM offsets
    int knownOffsets = 0;
    for (const auto & cs : col.sprites)
    {
        if (!cs.romOffset.isEmpty())
            ++knownOffsets;
    }

    // Build palette info
    QStringList palInfo;
    const auto & palPool = theDef.palettePool();
    QMap<QString, int> palLineMap;
    for (const auto & ns : norm.sprites)
    {
        if (!palLineMap.contains(ns.paletteId) && palLineMap.size() < 4)
            palLineMap.insert(ns.paletteId, palLineMap.size());
    }
    for (auto it = palLineMap.begin(); it != palLineMap.end(); ++it)
    {
        QString palId = it.key();
        int line = it.value();
        if (palPool.contains(palId))
        {
            const PoolPalette & pp = palPool[palId];
            if (pp.romOffset != 0)
                palInfo.append(QString("  Line %1: %2 (ROM: 0x%3)")
                    .arg(line).arg(pp.name)
                    .arg(pp.romOffset, 0, 16).toUpper());
            else
                palInfo.append(QString("  Line %1: %2 (embedded)")
                    .arg(line).arg(pp.name));
        }
    }

    QString info = QString("%1 sprites | %2x%3 pixels\n"
                           "ROM offsets: %4/%5 known\n"
                           "Palettes: %6 used")
        .arg(norm.sprites.size())
        .arg(imgW).arg(imgH)
        .arg(knownOffsets).arg(col.sprites.size())
        .arg(palLineMap.size());
    if (!palInfo.isEmpty())
        info += "\n" + palInfo.join("\n");

    ui->theViewerInfoLabel->setText(info);
    ui->theEditFromViewerButton->setEnabled(true);
}

void MainWindow::displaySpriteGroup(int groupIndex)
{
    const SpriteGroup & group = theDef.spriteGroups()[groupIndex];
    QVector<SpriteThumb> thumbs;

    for (int i = 0; i < group.sprites.size(); ++i)
    {
        const SpriteEntry & entry = group.sprites[i];
        GenesisPalette pal = paletteForSprite(entry, groupIndex);
        int bytesPerFrame = entry.widthTiles * entry.heightTiles * 32;

        for (int f = 0; f < entry.frameCount; ++f)
        {
            uint32_t frameOffset = entry.romOffset + uint32_t(f * bytesPerFrame);
            QByteArray frameData = theRom.readBytes(frameOffset, bytesPerFrame);
            if (frameData.size() < bytesPerFrame)
                continue;

            // For compressed entries, only the first frame uses the decompressor
            if (f == 0 && entry.compression != "none")
            {
                frameData = fetchTileData(entry);
                if (frameData.isEmpty())
                    continue;
            }

            QImage img = TileDecoder::decodeSprite(frameData,
                                                   entry.widthTiles,
                                                   entry.heightTiles,
                                                   pal);
            SpriteThumb thumb;
            thumb.image       = img;
            thumb.groupIndex  = groupIndex;
            thumb.spriteIndex = i;
            thumb.frameIndex  = f;

            if (entry.frameCount == 1)
                thumb.name = entry.name;
            else
                thumb.name = QString("%1 [%2/%3]")
                    .arg(entry.name).arg(f + 1).arg(entry.frameCount);

            thumbs.append(thumb);
        }
    }

    ui->theSpriteSheet->setSprites(thumbs);
    ui->theSpriteDetail->clearSprite();
}

void MainWindow::displaySpriteDetail(int groupIdx, int spriteIdx, int frameIdx)
{
    if (!theDef.isLoaded() || !theRom.isOpen())
        return;

    // Legacy mode only — normalized uses updateCollectionDetail()
    if (theDef.isNormalized())
        return;

    if (groupIdx < 0 || groupIdx >= theDef.spriteGroups().size())
        return;

    const SpriteGroup & group  = theDef.spriteGroups()[groupIdx];
    if (spriteIdx < 0 || spriteIdx >= group.sprites.size())
        return;

    const SpriteEntry & entry = group.sprites[spriteIdx];
    GenesisPalette pal  = paletteForSprite(entry, groupIdx);
    int bytesPerFrame = entry.widthTiles * entry.heightTiles * 32;

    QByteArray tileData;
    uint32_t frameOffset = entry.romOffset + uint32_t(frameIdx * bytesPerFrame);

    if (entry.compression == "none")
    {
        tileData = theRom.readBytes(frameOffset, bytesPerFrame);
    }
    else
    {
        QByteArray fullData = fetchTileData(entry);
        int start = frameIdx * bytesPerFrame;
        if (start + bytesPerFrame <= fullData.size())
            tileData = fullData.mid(start, bytesPerFrame);
        else
            tileData = fullData.left(bytesPerFrame);
        frameOffset = entry.romOffset;
    }

    QImage img = TileDecoder::decodeSprite(tileData,
                                           entry.widthTiles,
                                           entry.heightTiles,
                                           pal);
    ui->theSpriteDetail->setSprite(img);
    ui->theSpriteDetail->setZoom(ui->theZoomSpin->value());
}

void MainWindow::onZoomChanged(int value)
{
    ui->theSpriteDetail->setZoom(value);
}

void MainWindow::onThumbZoomChanged(int value)
{
    ui->theSpriteSheet->setThumbZoom(value);
}

void MainWindow::onViewerSpriteReordered(int fromIndex, int toIndex)
{
    AppDebug << "Reorder collection: " << fromIndex << " -> " << toIndex << std::endl;
    theDef.moveNormalizedCollection(fromIndex, toIndex);
    theDef.saveToFile(QString());
    populateCollectionGrid();
    statusBar()->showMessage(QString("Moved collection from position %1 to %2").arg(fromIndex).arg(toIndex));
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
    ui->theAssemblyStartButton->setEnabled(theRom.isOpen());

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
        if (theDef.isNormalized())
        {
            // Populate from palette pool
            const auto & pool = theDef.palettePool();
            for (auto it = pool.begin(); it != pool.end(); ++it)
            {
                ui->theRawPaletteCombo->addItem(it.value().name);
            }
        }
        else
        {
            // Legacy: from sprite groups
            for (const auto & g : theDef.spriteGroups())
            {
                for (const auto & pal : g.palettes)
                    ui->theRawPaletteCombo->addItem(
                        QString("%1 / %2").arg(g.name, pal.name));
            }
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
    ui->theRawBrowser->setZoom(value);
}

void MainWindow::onRawSpriteSizeChanged(int)
{
    ui->theRawBrowser->setSpriteSize(ui->theRawSpriteWSpin->value(),
                                 ui->theRawSpriteHSpin->value());
}

void MainWindow::onRawTileClicked(int tileIndex, uint32_t romOffset)
{
    theRawSelectedTileIndex = tileIndex;
    theRawSelectedRomOffset = romOffset;

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
        label = QString("Sprite %1  (tiles %2-%3, %4x%5)  |  ROM offset: 0x%6")
            .arg(tileIndex / tilesPerSprite)
            .arg(tileIndex)
            .arg(tileIndex + tilesPerSprite - 1)
            .arg(w).arg(h)
            .arg(romOffset, 6, 16, QChar('0')).toUpper();
    }
    ui->theRawTileInfoLabel->setText(label);
    ui->theRawExportButton->setEnabled(true);
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
    if (tileIndex >= ui->theRawBrowser->tileCount()) {
        ui->theRawTileInfoLabel->setText(
            QString("0x%1 is beyond the current range end.")
            .arg(targetOffset, 6, 16, QChar('0')).toUpper());
        return;
    }

    ui->theRawBrowser->scrollToTile(tileIndex);
    uint32_t actualOffset = rangeStart + uint32_t(tileIndex) * 32;
    ui->theRawTileInfoLabel->setText(
        QString("Jumped to tile %1  |  ROM offset: 0x%2")
        .arg(tileIndex)
        .arg(actualOffset, 6, 16, QChar('0')).toUpper());
}

void MainWindow::onSetAssemblyStart()
{
    QString text = ui->theJumpOffsetEdit->text().trimmed();
    bool ok = false;
    uint32_t targetOffset = text.toUInt(&ok, 16);
    if (!ok)
        targetOffset = text.toUInt(&ok, 0);
    if (!ok) {
        ui->theRawTileInfoLabel->setText("Set start failed: enter a valid hex offset (e.g. 0x0220E0)");
        return;
    }

    int rangeIdx = ui->theRawRangeCombo->currentIndex();
    uint32_t rangeStart = 0x200;
    if (theDef.isLoaded() && rangeIdx < theDef.tileRanges().size()) {
        rangeStart = theDef.tileRanges()[rangeIdx].startOffset;
    }

    if (targetOffset < rangeStart) {
        ui->theRawTileInfoLabel->setText(
            QString("0x%1 is before the current range start (0x%2).")
            .arg(targetOffset, 6, 16, QChar('0')).toUpper()
            .arg(rangeStart, 6, 16, QChar('0')).toUpper());
        return;
    }

    uint32_t byteOffset = targetOffset - rangeStart;
    int tileIndex = int(byteOffset / 32);
    if (tileIndex >= ui->theRawBrowser->tileCount()) {
        ui->theRawTileInfoLabel->setText(
            QString("0x%1 is beyond the current range end.")
            .arg(targetOffset, 6, 16, QChar('0')).toUpper());
        return;
    }

    ui->theRawBrowser->setAssemblyStart(tileIndex);
    ui->theRawBrowser->scrollToTile(tileIndex);
    uint32_t actualOffset = rangeStart + uint32_t(tileIndex) * 32;
    ui->theRawTileInfoLabel->setText(
        QString("Assembly starts at tile %1  |  ROM offset: 0x%2")
        .arg(tileIndex)
        .arg(actualOffset, 6, 16, QChar('0')).toUpper());
}

void MainWindow::onRawExportPng()
{
    if (!theRom.isOpen() || theRawSelectedTileIndex < 0)
        return;

    int w = ui->theRawSpriteWSpin->value();
    int h = ui->theRawSpriteHSpin->value();
    int bytesNeeded = w * h * 32;

    QByteArray tileData = theRom.readBytes(theRawSelectedRomOffset, bytesNeeded);
    if (tileData.size() < bytesNeeded)
    {
        statusBar()->showMessage("Not enough ROM data to export this sprite.");
        return;
    }

    // Use whatever palette is currently selected in the raw browser
    GenesisPalette pal = TileDecoder::greyPalette();
    int palIdx = ui->theRawPaletteCombo->currentIndex();
    if (palIdx > 0 && theDef.isLoaded())
    {
        if (theDef.isNormalized())
        {
            const auto & pool = theDef.palettePool();
            QList<QString> keys = pool.keys();
            int poolIdx = palIdx - 1;
            if (poolIdx >= 0 && poolIdx < keys.size())
                pal = paletteFromPool(keys[poolIdx]);
        }
        else
        {
            int flatIdx = 1;
            for (const auto & g : theDef.spriteGroups())
            {
                for (const auto & p : g.palettes)
                {
                    if (flatIdx == palIdx)
                    {
                        QByteArray palData = theRom.readBytes(p.romOffset, 32);
                        pal = TileDecoder::decodePalette(palData);
                        break;
                    }
                    ++flatIdx;
                }
            }
        }
    }

    QImage img = TileDecoder::decodeSprite(tileData, w, h, pal);
    if (img.isNull())
    {
        statusBar()->showMessage("Failed to decode sprite for export.");
        return;
    }

    QString suggestedName = QString("sprite_0x%1_%2x%3.png")
        .arg(theRawSelectedRomOffset, 6, 16, QChar('0')).toUpper()
        .arg(w).arg(h);

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
        if (theDef.isNormalized())
        {
            const auto & pool = theDef.palettePool();
            QList<QString> keys = pool.keys();
            int poolIdx = palIdx - 1;
            if (poolIdx >= 0 && poolIdx < keys.size())
                pal = paletteFromPool(keys[poolIdx]);
        }
        else
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
    }

    ui->theRawBrowser->setTileData(tileData, startOffset, pal);
    ui->theRawBrowser->setZoom(ui->theRawZoomSpin->value());

    ui->theRawTileInfoLabel->setText(
        QString("Showing %1 tiles from ROM 0x%2 - 0x%3  |  Click a tile for its offset")
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

QByteArray MainWindow::fetchPatternTileData(const PoolPattern & pat)
{
    if (!pat.tileData.isEmpty())
        return pat.tileData;

    int rawBytes = pat.widthTiles * pat.heightTiles * pat.frameCount * 32;
    QByteArray romSlice = theRom.readBytes(pat.romOffset,
                                           theRom.romSize() - pat.romOffset);
    if (pat.compression == "none")
        return romSlice.left(rawBytes);

    uint32_t consumed = 0;
    return theCompressor.decompress(pat.compression, romSlice, 0, rawBytes, &consumed);
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

GenesisPalette MainWindow::paletteFromPool(const QString & paletteId)
{
    if (!theDef.isLoaded() || !theDef.palettePool().contains(paletteId))
        return TileDecoder::greyPalette();

    const PoolPalette & pp = theDef.palettePool()[paletteId];

    // Prefer CRAM values if available
    if (!pp.cramValues.isEmpty())
        return TileDecoder::decodePaletteFromCram(pp.cramValues);

    // Otherwise read from ROM
    if (pp.romOffset != 0 && theRom.isOpen())
    {
        QByteArray palData = theRom.readBytes(pp.romOffset, 32);
        if (palData.size() >= 32)
            return TileDecoder::decodePalette(palData);
    }

    return TileDecoder::greyPalette();
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

void MainWindow::populateScreenCaptures()
{
    ui->theScreenCapCombo->blockSignals(true);
    ui->theScreenCapCombo->clear();

    const auto & captures = theDef.screenCaptures();
    for (const auto & cap : captures)
        ui->theScreenCapCombo->addItem(cap.name);

    ui->theScreenCapCombo->setEnabled(!captures.isEmpty());
    ui->theScreenCapCombo->blockSignals(false);

    if (!captures.isEmpty())
        onScreenCaptureSelected(0);
    else
        ui->theTileMapWidget->clearCapture();
}

void MainWindow::onScreenCaptureSelected(int index)
{
    const auto & captures = theDef.screenCaptures();
    if (index < 0 || index >= captures.size())
    {
        ui->theTileMapWidget->clearCapture();
        return;
    }

    RomFile *rom = theRom.isOpen() ? &theRom : nullptr;
    ui->theTileMapWidget->setScreenCapture(captures[index], rom);

    statusBar()->showMessage(
        QString("Screen capture: %1 (%2x%3 tiles)")
            .arg(captures[index].name)
            .arg(captures[index].widthTiles)
            .arg(captures[index].heightTiles));
}

void MainWindow::onScreenCapZoomChanged(int value)
{
    ui->theTileMapWidget->setZoom(value);
}

// ---------------------------------------------------------------------------
// Sprite Collections tab
// ---------------------------------------------------------------------------

void MainWindow::populateSpriteCollections()
{
    ui->theSpriteColCombo->blockSignals(true);
    ui->theSpriteColCombo->clear();

    theCollectionCount = 0;
    theAnimationCount  = 0;

    // Sprite groups are now in the Sprite Viewer tab, so only show recordings here

    // Add .sprec recordings
    for (int i = 0; i < theSpriteRecordings.size(); ++i)
    {
        const SpriteRecording & rec = theSpriteRecordings[i];
        QString label = QString("Recording: %1 (%2 frames)")
            .arg(rec.gameName()).arg(rec.frames().size());
        ui->theSpriteColCombo->addItem(label);
    }
    theAnimationCount = theSpriteRecordings.size();

    bool hasEntries = (theCollectionCount + theAnimationCount) > 0;
    ui->theSpriteColCombo->setEnabled(hasEntries);
    ui->theSpriteColCombo->blockSignals(false);

    if (hasEntries)
        onSpriteCollectionSelected(0);
    else
        ui->theSpriteColWidget->clearCollection();
}

void MainWindow::onSpriteCollectionSelected(int index)
{
    if (index < 0)
    {
        ui->theSpriteColWidget->clearCollection();
        return;
    }

    // Clear hidden and selection state on collection change
    theHiddenRomOffsets.clear();
    ui->theSpriteColWidget->clearHiddenSprites();
    ui->theSpriteColWidget->clearSelection();
    ui->theColSelectionLabel->setText("No sprites selected");
    ui->theCaptureGroupButton->setEnabled(false);
    ui->theHideSpritesButton->setEnabled(false);
    ui->theUnhideSpritesButton->setEnabled(false);
    ui->theUnhideSelectedButton->setEnabled(false);

    theActiveAnimIndex = -1;
    theActiveRecIndex  = -1;

    if (index < theCollectionCount)
    {
        // Regular collection (legacy or normalized)
        ui->colFrameLabel->setVisible(false);
        ui->theColFrameSpin->setVisible(false);
        ui->theColFrameCountLabel->setVisible(false);

        if (theDef.isNormalized())
        {
            const auto & normCols = theDef.normalizedCollections();
            if (index >= normCols.size())
            {
                ui->theSpriteColWidget->clearCollection();
                return;
            }

            SpriteCollection col = buildFromNormalized(normCols[index]);
            RomFile *rom = theRom.isOpen() ? &theRom : nullptr;
            ui->theSpriteColWidget->setCollection(col, rom);
            ui->theSpriteColWidget->setZoom(ui->theSpriteColZoomSpin->value());

            statusBar()->showMessage(
                QString("Collection: %1 (%2 sprites)")
                    .arg(col.name)
                    .arg(col.sprites.size()));
        }
        else
        {
            const auto & collections = theDef.spriteCollections();
            if (index >= collections.size())
            {
                ui->theSpriteColWidget->clearCollection();
                return;
            }

            RomFile *rom = theRom.isOpen() ? &theRom : nullptr;
            ui->theSpriteColWidget->setCollection(collections[index], rom);
            ui->theSpriteColWidget->setZoom(ui->theSpriteColZoomSpin->value());

            const SpriteCollection & col = collections[index];
            statusBar()->showMessage(
                QString("Sprite collection: %1 (%2 sprites, %3x%4 bounding box)")
                    .arg(col.name)
                    .arg(col.sprites.size())
                    .arg(col.boundingBox.width())
                    .arg(col.boundingBox.height()));
        }
    }
    else
    {
        // .sprec recording entry
        int recIndex = index - theCollectionCount;
        if (recIndex < 0 || recIndex >= theSpriteRecordings.size())
        {
            ui->theSpriteColWidget->clearCollection();
            return;
        }

        theActiveRecIndex = recIndex;
        const SpriteRecording & rec = theSpriteRecordings[recIndex];
        int frameCount = rec.frames().size();

        // Show and configure the frame selector
        ui->colFrameLabel->setVisible(true);
        ui->theColFrameSpin->setVisible(true);
        ui->theColFrameCountLabel->setVisible(true);

        ui->theColFrameSpin->blockSignals(true);
        ui->theColFrameSpin->setMinimum(0);
        ui->theColFrameSpin->setMaximum(frameCount - 1);
        ui->theColFrameSpin->setValue(0);
        ui->theColFrameSpin->blockSignals(false);
        ui->theColFrameCountLabel->setText(QString("/ %1").arg(frameCount));

        // Display the first frame
        displayRecordingFrame(recIndex, 0);
    }
}

void MainWindow::onSpriteCollectionZoomChanged(int value)
{
    ui->theSpriteColWidget->setZoom(value);
}

void MainWindow::onLoadRecording()
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

    theSpriteRecordings.append(rec);
    populateSpriteCollections();
    statusBar()->showMessage("Sprite recording loaded: " + rec.gameName());

    // Select the newly added recording in the combo
    int lastIdx = ui->theSpriteColCombo->count() - 1;
    if (lastIdx >= 0)
        ui->theSpriteColCombo->setCurrentIndex(lastIdx);
}

void MainWindow::onAnimationFrameChanged(int frameIndex)
{
    // Clear selection but preserve hidden state across frames
    ui->theSpriteColWidget->clearSelection();
    ui->theColSelectionLabel->setText("No sprites selected");
    ui->theCaptureGroupButton->setEnabled(false);
    ui->theHideSpritesButton->setEnabled(false);
    ui->theUnhideSelectedButton->setEnabled(false);

    if (theActiveRecIndex >= 0)
    {
        displayRecordingFrame(theActiveRecIndex, frameIndex);
        applyPersistentHidden();
        return;
    }
    if (theActiveAnimIndex >= 0)
    {
        displayAnimationFrame(theActiveAnimIndex, frameIndex);
        applyPersistentHidden();
        return;
    }
}

void MainWindow::displayAnimationFrame(int animIndex, int frameIndex)
{
    // This is kept for backward compat but recordings use displayRecordingFrame
    (void)animIndex;
    (void)frameIndex;
}

void MainWindow::displayRecordingFrame(int recIndex, int frameIndex)
{
    if (recIndex < 0 || recIndex >= theSpriteRecordings.size())
        return;

    const SpriteRecording & rec = theSpriteRecordings[recIndex];
    if (frameIndex < 0 || frameIndex >= rec.frames().size())
        return;

    SpriteCollection col = buildCollectionFromRecording(rec, frameIndex);

    RomFile *rom = theRom.isOpen() ? &theRom : nullptr;
    ui->theSpriteColWidget->setCollection(col, rom);
    ui->theSpriteColWidget->setZoom(ui->theSpriteColZoomSpin->value());

    const AnimationFrame & frame = rec.frames()[frameIndex];
    statusBar()->showMessage(
        QString("Recording frame %1 (VDP frame %2): %3 sprites, %4x%5 bounding box")
            .arg(frameIndex)
            .arg(frame.frameNumber)
            .arg(frame.sprites.size())
            .arg(frame.boundingBox.width())
            .arg(frame.boundingBox.height()));
}

SpriteCollection MainWindow::buildCollectionFromFrame(const SpriteAnimation & anim, int frameIndex)
{
    SpriteCollection col;
    const AnimationFrame & frame = anim.frames[frameIndex];

    col.name = QString("%1 - frame %2").arg(anim.gameName).arg(frame.frameNumber);
    col.boundingBox = frame.boundingBox;
    col.sprites     = frame.sprites;

    // Copy shared palettes from the animation
    col.palettes = anim.palettes;

    return col;
}

SpriteCollection MainWindow::buildCollectionFromRecording(const SpriteRecording & rec, int frameIndex)
{
    SpriteCollection col;
    const AnimationFrame & frame = rec.frames()[frameIndex];

    col.name = QString("%1 - frame %2").arg(rec.gameName()).arg(frame.frameNumber);
    col.boundingBox = frame.boundingBox;
    col.sprites     = frame.sprites;

    // Copy shared palettes from the recording
    col.palettes = rec.palettes();

    return col;
}

SpriteCollection MainWindow::buildFromNormalized(const NormalizedCollection & norm)
{
    SpriteCollection col;
    col.name = norm.name;

    const auto & palPool = theDef.palettePool();
    const auto & patPool = theDef.patternPool();

    // Track unique palettes used (max 4 lines)
    QMap<QString, int> palLineMap;  // paletteId -> assigned palette line
    QVector<ScreenCapturePalette> usedPalettes;

    // First pass: assign palette lines
    for (const auto & ns : norm.sprites)
    {
        if (!palLineMap.contains(ns.paletteId) && palLineMap.size() < 4)
        {
            int line = palLineMap.size();
            palLineMap.insert(ns.paletteId, line);

            ScreenCapturePalette scp;
            scp.line = line;

            if (palPool.contains(ns.paletteId))
            {
                const PoolPalette & pp = palPool[ns.paletteId];
                if (!pp.cramValues.isEmpty())
                {
                    scp.cramValues = pp.cramValues;
                }
                else if (pp.romOffset != 0 && theRom.isOpen())
                {
                    QByteArray palData = theRom.readBytes(pp.romOffset, 32);
                    // Convert ROM palette data to CRAM values
                    for (int i = 0; i + 1 < palData.size() && scp.cramValues.size() < 16; i += 2)
                    {
                        uint16_t word = (uint8_t(palData[i]) << 8) | uint8_t(palData[i + 1]);
                        scp.cramValues.append(word);
                    }
                }
                if (pp.romOffset != 0)
                    scp.dmaSource = QString("0x%1").arg(pp.romOffset, 0, 16).toUpper();
            }

            usedPalettes.append(scp);
        }
    }
    col.palettes = usedPalettes;

    // Calculate bounding box and build sprites
    int16_t minX = 32767, minY = 32767, maxX = -32768, maxY = -32768;

    for (int i = 0; i < norm.sprites.size(); ++i)
    {
        const NormalizedSprite & ns = norm.sprites[i];
        CollectionSprite cs;
        cs.index = i;
        cs.x = ns.x;
        cs.y = ns.y;
        cs.hFlip = ns.hFlip;
        cs.vFlip = ns.vFlip;
        cs.priority = ns.priority;
        cs.paletteLine = palLineMap.value(ns.paletteId, 0);

        if (patPool.contains(ns.patternId))
        {
            const PoolPattern & pat = patPool[ns.patternId];
            cs.widthTiles = pat.widthTiles;
            cs.heightTiles = pat.heightTiles;
            cs.pattern = 0;

            int bytesPerFrame = pat.widthTiles * pat.heightTiles * 32;
            uint32_t frameRomOffset = pat.romOffset + uint32_t(ns.frame * bytesPerFrame);
            cs.romOffset = QString("0x%1").arg(frameRomOffset, 0, 16).toUpper();
            cs.source = "dma";
            cs.vramAddr = "";
        }
        else
        {
            cs.widthTiles = 1;
            cs.heightTiles = 1;
            cs.source = "embedded";
        }

        if (cs.x < minX) minX = cs.x;
        if (cs.y < minY) minY = cs.y;
        int right = cs.x + cs.widthTiles * 8;
        int bottom = cs.y + cs.heightTiles * 8;
        if (right > maxX) maxX = right;
        if (bottom > maxY) maxY = bottom;

        col.sprites.append(cs);
    }

    if (!norm.sprites.isEmpty())
        col.boundingBox = QRect(minX, minY, maxX - minX, maxY - minY);

    return col;
}

void MainWindow::applyPersistentHidden()
{
    const SpriteCollection & col = ui->theSpriteColWidget->collection();
    QSet<int> indexSet;
    for (int i = 0; i < col.sprites.size(); ++i)
    {
        const QString & rom = col.sprites[i].romOffset;
        if (!rom.isEmpty() && theHiddenRomOffsets.contains(rom))
            indexSet.insert(i);
    }
    ui->theSpriteColWidget->setHiddenSprites(indexSet);
    ui->theUnhideSpritesButton->setEnabled(!theHiddenRomOffsets.isEmpty());
}

// ---------------------------------------------------------------------------
// Sprite Editor tab
// ---------------------------------------------------------------------------

void MainWindow::onCollectionSpriteClicked(int spriteIndex)
{
    int comboIndex = ui->theSpriteColCombo->currentIndex();
    if (comboIndex < 0)
        return;

    // Resolve the active collection
    SpriteCollection col;
    bool isNormCol = false;
    int normColIndex = -1;

    if (comboIndex < theCollectionCount)
    {
        if (theDef.isNormalized())
        {
            const auto & normCols = theDef.normalizedCollections();
            if (comboIndex >= normCols.size())
                return;
            col = buildFromNormalized(normCols[comboIndex]);
            isNormCol = true;
            normColIndex = comboIndex;
        }
        else
        {
            const auto & collections = theDef.spriteCollections();
            if (comboIndex >= collections.size())
                return;
            col = collections[comboIndex];
        }
    }
    else
    {
        // Recording frame
        int recIndex = comboIndex - theCollectionCount;
        if (recIndex < 0 || recIndex >= theSpriteRecordings.size())
            return;
        int frameIndex = ui->theColFrameSpin->value();
        col = buildCollectionFromRecording(theSpriteRecordings[recIndex], frameIndex);
    }

    if (spriteIndex < 0 || spriteIndex >= col.sprites.size())
        return;

    // Store which collection we're editing
    theEditCollectionIndex = comboIndex;
    theEditSpriteIndex = spriteIndex;

    // Check if this is a multi-sprite normalized collection -> group mode
    if (isNormCol && col.sprites.size() > 1)
    {
        // Build EditorSprite list from the collection
        QVector<EditorSprite> edSprites;
        for (int i = 0; i < col.sprites.size(); ++i)
        {
            const CollectionSprite & cs = col.sprites[i];
            EditorSprite es;
            es.widthTiles = cs.widthTiles;
            es.heightTiles = cs.heightTiles;
            es.x = cs.x;
            es.y = cs.y;
            es.hFlip = cs.hFlip;
            es.vFlip = cs.vFlip;
            es.paletteLine = cs.paletteLine;
            es.romOffset = cs.romOffset;

            // Get tile data
            if (cs.source == "embedded" && !cs.tileData.isEmpty())
            {
                es.tileData = cs.tileData;
            }
            else if (!cs.romOffset.isEmpty() && theRom.isOpen())
            {
                bool ok = false;
                QString offsetStr = cs.romOffset;
                if (offsetStr.startsWith("0x") || offsetStr.startsWith("0X"))
                    offsetStr = offsetStr.mid(2);
                uint32_t offset = offsetStr.toUInt(&ok, 16);
                if (ok)
                {
                    int totalBytes = cs.widthTiles * cs.heightTiles * 32;
                    es.tileData = theRom.readBytes(offset, totalBytes);
                }
            }

            if (es.tileData.isEmpty())
            {
                int totalBytes = cs.widthTiles * cs.heightTiles * 32;
                es.tileData = QByteArray(totalBytes, '\0');
            }

            edSprites.append(es);
        }

        // Decode palettes
        GenesisPalette pals[4];
        for (int i = 0; i < 4; ++i)
        {
            if (i < col.palettes.size() && !col.palettes[i].cramValues.isEmpty())
                pals[i] = TileDecoder::decodePaletteFromCram(col.palettes[i].cramValues);
            else
                pals[i] = TileDecoder::greyPalette();
        }

        // Load group into pixel editor
        ui->theSpritePixelEditor->loadSpriteGroup(edSprites, pals);
        ui->theSpritePixelEditor->setZoom(ui->theEditorZoomSpin->value());
        ui->theSpritePixelEditor->setShowGrid(ui->theEditorGridCheck->isChecked());

        // Init multi-palette state
        theEditorActivePaletteLine = 0;
        theEditPalLineToId.clear();
        if (isNormCol && comboIndex < theDef.normalizedCollections().size())
        {
            const NormalizedCollection & norm = theDef.normalizedCollections()[comboIndex];
            // Build palette line -> ID mapping (same logic as buildFromNormalized)
            QMap<QString, int> palLineMap;
            for (const auto & ns : norm.sprites)
            {
                if (!palLineMap.contains(ns.paletteId) && palLineMap.size() < 4)
                    palLineMap.insert(ns.paletteId, palLineMap.size());
            }
            for (auto it = palLineMap.begin(); it != palLineMap.end(); ++it)
                theEditPalLineToId[it.value()] = it.key();
        }

        // Set editor palette to line 0 (middle-click sprite to switch)
        ui->theEditorPalette->setPalette(pals[0]);

        // Enable save if any sprite has a ROM offset
        bool canSave = false;
        for (const EditorSprite & es : edSprites)
        {
            if (!es.romOffset.isEmpty())
            {
                canSave = true;
                break;
            }
        }
        ui->theEditorSaveButton->setEnabled(canSave && theRom.isOpen());

        // Enable palette save if active palette has a known ROM offset
        bool canSavePal = false;
        QString firstPalId = theEditPalLineToId.value(0);
        if (!firstPalId.isEmpty() && theDef.palettePool().contains(firstPalId))
            canSavePal = theDef.palettePool()[firstPalId].romOffset != 0;
        ui->theEditorSavePaletteButton->setEnabled(canSavePal && theRom.isOpen());

        QString info = QString("Group: %1 | %2 sprites | Click to edit")
            .arg(col.name).arg(col.sprites.size());
        ui->theEditorInfoLabel->setText(info);

        int editorTabIndex = ui->theTabWidget->indexOf(ui->tabSpriteEditor);
        if (editorTabIndex >= 0)
            ui->theTabWidget->setCurrentIndex(editorTabIndex);

        statusBar()->showMessage(
            QString("Editing sprite group \"%1\" (%2 sprites)")
            .arg(col.name).arg(col.sprites.size()));
        return;
    }

    // Single sprite mode (original behavior)
    const CollectionSprite & sprite = col.sprites[spriteIndex];
    int palLine = qBound(0, sprite.paletteLine, 3);

    // Get tile data for this sprite
    QByteArray tileBytes;
    if (sprite.source == "embedded" && !sprite.tileData.isEmpty())
    {
        tileBytes = sprite.tileData;
    }
    else if (!sprite.romOffset.isEmpty() && theRom.isOpen())
    {
        bool ok = false;
        QString offsetStr = sprite.romOffset;
        if (offsetStr.startsWith("0x") || offsetStr.startsWith("0X"))
            offsetStr = offsetStr.mid(2);
        uint32_t offset = offsetStr.toUInt(&ok, 16);
        if (ok)
        {
            int totalBytes = sprite.widthTiles * sprite.heightTiles * 32;
            tileBytes = theRom.readBytes(offset, totalBytes);
        }
    }

    if (tileBytes.isEmpty())
    {
        int totalBytes = sprite.widthTiles * sprite.heightTiles * 32;
        tileBytes = QByteArray(totalBytes, '\0');
    }

    // Get palette from the collection
    GenesisPalette pal = TileDecoder::greyPalette();
    if (palLine < col.palettes.size())
        pal = TileDecoder::decodePaletteFromCram(col.palettes[palLine].cramValues);

    // Load into pixel editor
    ui->theSpritePixelEditor->loadSprite(tileBytes, sprite.widthTiles, sprite.heightTiles,
                                     pal, sprite.hFlip, sprite.vFlip);
    ui->theSpritePixelEditor->setZoom(ui->theEditorZoomSpin->value());
    ui->theSpritePixelEditor->setShowGrid(ui->theEditorGridCheck->isChecked());

    // Set palette display
    ui->theEditorPalette->setPalette(pal);

    // Update info label
    bool canSaveTiles = !sprite.romOffset.isEmpty() && theRom.isOpen();
    bool canSavePalette = false;
    if (palLine < col.palettes.size())
        canSavePalette = !col.palettes[palLine].dmaSource.isEmpty() && theRom.isOpen();

    QString info = QString("Sprite #%1 | %2x%3 tiles | Palette: %4 | ROM: %5 | %6")
        .arg(sprite.index)
        .arg(sprite.widthTiles).arg(sprite.heightTiles)
        .arg(palLine)
        .arg(sprite.romOffset.isEmpty() ? "N/A" : sprite.romOffset)
        .arg(canSaveTiles ? "Editable" : "Read-only (no ROM offset)");
    ui->theEditorInfoLabel->setText(info);

    // Enable/disable save buttons
    ui->theEditorSaveButton->setEnabled(canSaveTiles);
    ui->theEditorSavePaletteButton->setEnabled(canSavePalette);

    // Switch to the editor tab
    int editorTabIndex = ui->theTabWidget->indexOf(ui->tabSpriteEditor);
    if (editorTabIndex >= 0)
        ui->theTabWidget->setCurrentIndex(editorTabIndex);

    statusBar()->showMessage(
        QString("Editing sprite #%1 from collection \"%2\"")
            .arg(sprite.index).arg(col.name));
}

void MainWindow::onEditorGroupPaletteLineChanged(int paletteLine)
{
    theEditorActivePaletteLine = paletteLine;
    ui->theEditorPalette->setPalette(
        ui->theSpritePixelEditor->groupPalette(paletteLine));

    // Update save palette button based on whether this palette has a ROM offset
    bool canSavePal = false;
    QString palId = theEditPalLineToId.value(paletteLine);
    if (!palId.isEmpty() && theDef.palettePool().contains(palId))
        canSavePal = theDef.palettePool()[palId].romOffset != 0;
    ui->theEditorSavePaletteButton->setEnabled(canSavePal && theRom.isOpen());

    statusBar()->showMessage(
        QString("Palette line %1 selected (middle-click sprite to switch)")
        .arg(paletteLine));
}

void MainWindow::onEditorPaletteSelected(int index)
{
    ui->theSpritePixelEditor->setPenIndex(index);
    statusBar()->showMessage(
        QString("Pen color: index %1  CRAM: 0x%2")
        .arg(index)
        .arg(ui->theEditorPalette->selectedCramWord(), 4, 16, QChar('0')).toUpper());
}

void MainWindow::onEditorPaletteEditRequested(int index)
{
    // Only allow editing if we can save the palette
    int colIndex = theEditCollectionIndex;

    // Get the active collection to find palette info
    SpriteCollection col;
    if (colIndex >= 0 && colIndex < theCollectionCount)
    {
        if (theDef.isNormalized())
        {
            const auto & normCols = theDef.normalizedCollections();
            if (colIndex < normCols.size())
                col = buildFromNormalized(normCols[colIndex]);
            else
                return;
        }
        else
        {
            const auto & collections = theDef.spriteCollections();
            if (colIndex < collections.size())
                col = collections[colIndex];
            else
                return;
        }
    }
    else
    {
        int recIndex = colIndex - theCollectionCount;
        if (recIndex >= 0 && recIndex < theSpriteRecordings.size())
        {
            int frameIndex = ui->theColFrameSpin->value();
            col = buildCollectionFromRecording(theSpriteRecordings[recIndex], frameIndex);
        }
        else
        {
            return;
        }
    }

    if (theEditSpriteIndex < 0 || theEditSpriteIndex >= col.sprites.size())
        return;

    // Group mode: use active palette line and check ROM offset from pool
    if (ui->theSpritePixelEditor->isGroupMode())
    {
        QString palId = theEditPalLineToId.value(theEditorActivePaletteLine);
        if (palId.isEmpty())
        {
            statusBar()->showMessage("No palette assigned to this line");
            return;
        }

        const auto & palPool = theDef.palettePool();
        if (!palPool.contains(palId))
        {
            statusBar()->showMessage("Palette ID not found in pool: " + palId);
            return;
        }

        const PoolPalette & pp = palPool[palId];
        if (pp.romOffset == 0)
        {
            statusBar()->showMessage(
                "Palette has no known ROM location - color editing disabled");
            return;
        }

        // Show shared palette reference count
        int totalRefs = theDef.countPaletteReferences(palId);
        // Count how many sprites in the current group use this palette
        int localCount = 0;
        if (colIndex < theDef.normalizedCollections().size())
        {
            const NormalizedCollection & norm = theDef.normalizedCollections()[colIndex];
            for (const auto & ns : norm.sprites)
                if (ns.paletteId == palId)
                    ++localCount;
        }
        if (totalRefs > localCount)
        {
            statusBar()->showMessage(
                QString("Note: this palette is shared by %1 sprites across all collections")
                .arg(totalRefs));
        }

        QColor current = ui->theEditorPalette->palette()[index];
        GenesisColorDialog dlg(current, index, this);
        if (dlg.exec() != QDialog::Accepted)
            return;

        ui->theEditorPalette->setColorAt(index, dlg.selectedColor());
        ui->theSpritePixelEditor->updateGroupPalette(
            theEditorActivePaletteLine, ui->theEditorPalette->palette());

        statusBar()->showMessage(
            QString("Color %1 changed to CRAM 0x%2")
            .arg(index)
            .arg(dlg.selectedCramWord(), 4, 16, QChar('0')).toUpper());
        return;
    }

    // Single sprite mode
    int palLine = qBound(0, col.sprites[theEditSpriteIndex].paletteLine, 3);
    bool canSavePalette = false;
    if (palLine < col.palettes.size())
        canSavePalette = !col.palettes[palLine].dmaSource.isEmpty() && theRom.isOpen();

    if (!canSavePalette)
    {
        statusBar()->showMessage("Palette editing disabled - no DMA source address known");
        return;
    }

    QColor current = ui->theEditorPalette->palette()[index];
    GenesisColorDialog dlg(current, index, this);
    if (dlg.exec() != QDialog::Accepted)
        return;

    ui->theEditorPalette->setColorAt(index, dlg.selectedColor());
    ui->theSpritePixelEditor->updatePalette(ui->theEditorPalette->palette());

    statusBar()->showMessage(
        QString("Color %1 changed to CRAM 0x%2")
        .arg(index)
        .arg(dlg.selectedCramWord(), 4, 16, QChar('0')).toUpper());
}

void MainWindow::onEditorSave()
{
    if (!theRom.isOpen() || !ui->theSpritePixelEditor->isModified())
    {
        statusBar()->showMessage("Nothing to save.");
        return;
    }

    int colIndex = theEditCollectionIndex;

    // Group mode: save each sprite's tile data to its ROM offset
    if (ui->theSpritePixelEditor->isGroupMode())
    {
        int saved = 0;
        int count = ui->theSpritePixelEditor->groupSpriteCount();
        for (int i = 0; i < count; ++i)
        {
            const EditorSprite & es = ui->theSpritePixelEditor->groupSprite(i);
            if (es.romOffset.isEmpty())
                continue;

            bool ok = false;
            QString offsetStr = es.romOffset;
            if (offsetStr.startsWith("0x") || offsetStr.startsWith("0X"))
                offsetStr = offsetStr.mid(2);
            uint32_t offset = offsetStr.toUInt(&ok, 16);
            if (!ok)
                continue;

            QByteArray data = ui->theSpritePixelEditor->modifiedGroupTileData(i);
            if (theRom.writeBytes(offset, data))
                ++saved;
        }

        updateWindowTitle();
        statusBar()->showMessage(
            QString("Saved %1 of %2 sprite tile data blocks to ROM. Use File > Save ROM to persist.")
            .arg(saved).arg(count));

        onSpriteCollectionSelected(colIndex);
        return;
    }

    // Single sprite mode
    SpriteCollection col;
    if (colIndex >= 0 && colIndex < theCollectionCount)
    {
        if (theDef.isNormalized())
        {
            const auto & normCols = theDef.normalizedCollections();
            if (colIndex < normCols.size())
                col = buildFromNormalized(normCols[colIndex]);
            else
                return;
        }
        else
        {
            const auto & collections = theDef.spriteCollections();
            if (colIndex < collections.size())
                col = collections[colIndex];
            else
                return;
        }
    }
    else
    {
        int recIndex = colIndex - theCollectionCount;
        if (recIndex >= 0 && recIndex < theSpriteRecordings.size())
        {
            int frameIndex = ui->theColFrameSpin->value();
            col = buildCollectionFromRecording(theSpriteRecordings[recIndex], frameIndex);
        }
        else
        {
            return;
        }
    }

    if (theEditSpriteIndex < 0 || theEditSpriteIndex >= col.sprites.size())
        return;

    const CollectionSprite & sprite = col.sprites[theEditSpriteIndex];
    if (sprite.romOffset.isEmpty())
    {
        QMessageBox::warning(this, "Cannot Save",
            "This sprite has no ROM offset - tile data cannot be saved.");
        return;
    }

    bool ok = false;
    QString offsetStr = sprite.romOffset;
    if (offsetStr.startsWith("0x") || offsetStr.startsWith("0X"))
        offsetStr = offsetStr.mid(2);
    uint32_t offset = offsetStr.toUInt(&ok, 16);
    if (!ok)
    {
        QMessageBox::critical(this, "Error", "Invalid ROM offset: " + sprite.romOffset);
        return;
    }

    QByteArray data = ui->theSpritePixelEditor->modifiedTileData();
    if (!theRom.writeBytes(offset, data))
    {
        QMessageBox::critical(this, "Error",
            QString("Failed to write %1 bytes at ROM offset 0x%2")
            .arg(data.size())
            .arg(offset, 6, 16, QChar('0')).toUpper());
        return;
    }

    updateWindowTitle();
    statusBar()->showMessage(
        QString("Saved %1 bytes of tile data to ROM at 0x%2. Use File > Save ROM to persist.")
        .arg(data.size())
        .arg(offset, 6, 16, QChar('0')).toUpper());

    // Refresh the collection view to show the changes
    onSpriteCollectionSelected(colIndex);
}

void MainWindow::onEditorSavePalette()
{
    if (!theRom.isOpen())
        return;

    int colIndex = theEditCollectionIndex;

    // Group mode: save palette via pool romOffset
    if (ui->theSpritePixelEditor->isGroupMode())
    {
        QString palId = theEditPalLineToId.value(theEditorActivePaletteLine);
        if (palId.isEmpty())
        {
            statusBar()->showMessage("No palette assigned to active line");
            return;
        }

        const auto & palPool = theDef.palettePool();
        if (!palPool.contains(palId) || palPool[palId].romOffset == 0)
        {
            QMessageBox::warning(this, "Cannot Save Palette",
                "No ROM offset known for palette: " + palId);
            return;
        }

        uint32_t offset = palPool[palId].romOffset;
        QByteArray palData = TileDecoder::encodePalette(ui->theEditorPalette->palette());
        if (!theRom.writeBytes(offset, palData))
        {
            QMessageBox::critical(this, "Error",
                QString("Failed to write palette at ROM offset 0x%1")
                .arg(offset, 6, 16, QChar('0')).toUpper());
            return;
        }

        updateWindowTitle();
        statusBar()->showMessage(
            QString("Saved palette to ROM at 0x%1. Use File > Save ROM to persist.")
            .arg(offset, 6, 16, QChar('0')).toUpper());
        return;
    }

    // Get the active collection
    SpriteCollection col;
    if (colIndex >= 0 && colIndex < theCollectionCount)
    {
        if (theDef.isNormalized())
        {
            const auto & normCols = theDef.normalizedCollections();
            if (colIndex < normCols.size())
                col = buildFromNormalized(normCols[colIndex]);
            else
                return;
        }
        else
        {
            const auto & collections = theDef.spriteCollections();
            if (colIndex < collections.size())
                col = collections[colIndex];
            else
                return;
        }
    }
    else
    {
        int recIndex = colIndex - theCollectionCount;
        if (recIndex >= 0 && recIndex < theSpriteRecordings.size())
        {
            int frameIndex = ui->theColFrameSpin->value();
            col = buildCollectionFromRecording(theSpriteRecordings[recIndex], frameIndex);
        }
        else
        {
            return;
        }
    }

    if (theEditSpriteIndex < 0 || theEditSpriteIndex >= col.sprites.size())
        return;

    int palLine = qBound(0, col.sprites[theEditSpriteIndex].paletteLine, 3);
    if (palLine >= col.palettes.size())
        return;

    const ScreenCapturePalette & scp = col.palettes[palLine];
    if (scp.dmaSource.isEmpty())
    {
        QMessageBox::warning(this, "Cannot Save Palette",
            "No DMA source address known for this palette line.");
        return;
    }

    bool ok = false;
    QString offsetStr = scp.dmaSource;
    if (offsetStr.startsWith("0x") || offsetStr.startsWith("0X"))
        offsetStr = offsetStr.mid(2);
    uint32_t offset = offsetStr.toUInt(&ok, 16);
    if (!ok)
    {
        QMessageBox::critical(this, "Error", "Invalid palette offset: " + scp.dmaSource);
        return;
    }

    QByteArray palData = TileDecoder::encodePalette(ui->theEditorPalette->palette());
    if (!theRom.writeBytes(offset, palData))
    {
        QMessageBox::critical(this, "Error",
            QString("Failed to write palette at ROM offset 0x%1")
            .arg(offset, 6, 16, QChar('0')).toUpper());
        return;
    }

    updateWindowTitle();
    statusBar()->showMessage(
        QString("Saved palette to ROM at 0x%1. Use File > Save ROM to persist.")
        .arg(offset, 6, 16, QChar('0')).toUpper());
}

void MainWindow::onEditorClose()
{
    if (ui->theSpritePixelEditor->isModified())
    {
        QMessageBox::StandardButton reply = QMessageBox::question(
            this, "Unsaved Pixel Edits",
            "You have unsaved pixel edits. Discard them?",
            QMessageBox::Discard | QMessageBox::Cancel);

        if (reply == QMessageBox::Cancel)
            return;
    }

    ui->theSpritePixelEditor->clearSprite();
    ui->theEditorInfoLabel->setText("No sprite selected - click a sprite in the Collections tab");
    ui->theEditorSaveButton->setEnabled(false);
    ui->theEditorSavePaletteButton->setEnabled(false);

    // Switch back to collections tab
    int colTabIndex = ui->theTabWidget->indexOf(ui->tabSpriteCollections);
    if (colTabIndex >= 0)
        ui->theTabWidget->setCurrentIndex(colTabIndex);
}

void MainWindow::onEditorZoomChanged(int value)
{
    ui->theSpritePixelEditor->setZoom(value);
}

void MainWindow::onEditorGridToggled(bool checked)
{
    ui->theSpritePixelEditor->setShowGrid(checked);
}

// ---------------------------------------------------------------------------
// Capture workflow
// ---------------------------------------------------------------------------

void MainWindow::onCollectionSelectionChanged(const QSet<int> & selectedIndices)
{
    int count = selectedIndices.size();
    if (count == 0)
        ui->theColSelectionLabel->setText("No sprites selected");
    else
        ui->theColSelectionLabel->setText(QString("%1 sprite%2 selected")
            .arg(count).arg(count == 1 ? "" : "s"));

    bool hasSelection = count > 0;
    ui->theCaptureGroupButton->setEnabled(hasSelection);
    ui->theHideSpritesButton->setEnabled(hasSelection);
    ui->theUnhideSpritesButton->setEnabled(!theHiddenRomOffsets.isEmpty());

    // Check if any selected sprites are currently hidden
    bool hasHiddenSelected = false;
    const SpriteCollection & col = ui->theSpriteColWidget->collection();
    for (int idx : selectedIndices)
    {
        if (idx >= 0 && idx < col.sprites.size())
        {
            const QString & rom = col.sprites[idx].romOffset;
            if (!rom.isEmpty() && theHiddenRomOffsets.contains(rom))
            {
                hasHiddenSelected = true;
                break;
            }
        }
    }
    ui->theUnhideSelectedButton->setEnabled(hasHiddenSelected);
}

void MainWindow::onCaptureSpriteGroup()
{
    const QSet<int> & selectedIndices = ui->theSpriteColWidget->selectedSpriteIndices();
    if (selectedIndices.isEmpty())
        return;

    // Get the current collection (we need the sprites and palettes)
    int comboIndex = ui->theSpriteColCombo->currentIndex();
    if (comboIndex < 0)
        return;

    SpriteCollection col;
    if (comboIndex < theCollectionCount)
    {
        if (theDef.isNormalized())
        {
            const auto & normCols = theDef.normalizedCollections();
            if (comboIndex < normCols.size())
                col = buildFromNormalized(normCols[comboIndex]);
            else
                return;
        }
        else
        {
            const auto & collections = theDef.spriteCollections();
            if (comboIndex < collections.size())
                col = collections[comboIndex];
            else
                return;
        }
    }
    else
    {
        int recIndex = comboIndex - theCollectionCount;
        if (recIndex >= 0 && recIndex < theSpriteRecordings.size())
        {
            int frameIndex = ui->theColFrameSpin->value();
            col = buildCollectionFromRecording(theSpriteRecordings[recIndex], frameIndex);
        }
        else
            return;
    }

    // Auto-generate a unique name and ID
    ++theCaptureCounter;
    QString name = QString("Capture %1").arg(theCaptureCounter);
    QString id = QString("capture_%1").arg(theCaptureCounter);
    while (theDef.hasCollectionId(id))
    {
        ++theCaptureCounter;
        name = QString("Capture %1").arg(theCaptureCounter);
        id = QString("capture_%1").arg(theCaptureCounter);
    }

    // Auto-promote to normalized format if needed
    if (!theDef.isLoaded())
    {
        theDef.initEmpty("Captured Sprites", "captured");
    }
    if (!theDef.isNormalized())
    {
        theDef.ensureNormalized();
    }

    // Calculate min X/Y for normalization
    int minX = 32767, minY = 32767;
    for (int idx : selectedIndices)
    {
        if (idx < 0 || idx >= col.sprites.size())
            continue;
        const CollectionSprite & s = col.sprites[idx];
        if (s.x < minX) minX = s.x;
        if (s.y < minY) minY = s.y;
    }

    // Build palettes, patterns, and the collection
    QMap<int, QString> palLineToId;   // palette_line -> assigned palette ID
    QMap<QString, QString> romToPatId; // romOffset -> assigned pattern ID

    NormalizedCollection normCol;
    normCol.id = id;
    normCol.name = name;

    for (int idx : selectedIndices)
    {
        if (idx < 0 || idx >= col.sprites.size())
            continue;
        const CollectionSprite & s = col.sprites[idx];

        // Create or reuse palette
        int palLine = qBound(0, s.paletteLine, 3);
        if (!palLineToId.contains(palLine))
        {
            QString palId = QString("%1_pal%2").arg(id).arg(palLine);
            if (!theDef.hasPaletteId(palId))
            {
                PoolPalette pp;
                pp.id = palId;
                pp.name = QString("%1 Palette Line %2").arg(name).arg(palLine);
                pp.romOffset = 0;

                // Get CRAM values from the collection palette
                if (palLine < col.palettes.size())
                {
                    pp.cramValues = col.palettes[palLine].cramValues;
                    if (!col.palettes[palLine].dmaSource.isEmpty())
                    {
                        bool romOk = false;
                        pp.romOffset = GameDefinition::parseOffset(
                            col.palettes[palLine].dmaSource, &romOk);
                        if (!romOk) pp.romOffset = 0;
                    }
                }
                theDef.addPoolPalette(pp);
            }
            palLineToId[palLine] = palId;
        }

        // Create or reuse pattern
        QString patKey = s.romOffset.isEmpty()
            ? QString("embedded_%1_%2").arg(idx).arg(s.vramAddr)
            : s.romOffset;
        if (!romToPatId.contains(patKey))
        {
            QString patId = QString("%1_pat_%2x%3_%4")
                .arg(id).arg(s.widthTiles).arg(s.heightTiles)
                .arg(romToPatId.size());
            if (!theDef.hasPatternId(patId))
            {
                PoolPattern pat;
                pat.id = patId;
                pat.name = QString("%1 %2x%3 #%4")
                    .arg(name).arg(s.widthTiles).arg(s.heightTiles)
                    .arg(romToPatId.size());
                pat.widthTiles = s.widthTiles;
                pat.heightTiles = s.heightTiles;
                pat.frameCount = 1;
                pat.compression = "none";
                pat.romOffset = 0;

                if (!s.romOffset.isEmpty() && (s.source == "dma" || s.source == "search"))
                {
                    bool romOk = false;
                    pat.romOffset = GameDefinition::parseOffset(s.romOffset, &romOk);
                    if (!romOk) pat.romOffset = 0;
                }
                else if (s.source == "embedded" && !s.tileData.isEmpty())
                {
                    pat.tileData = s.tileData;
                }

                theDef.addPoolPattern(pat);
            }
            romToPatId[patKey] = patId;
        }

        // Build the normalized sprite entry
        NormalizedSprite ns;
        ns.patternId = romToPatId[patKey];
        ns.frame = 0;
        ns.paletteId = palLineToId[palLine];
        ns.x = s.x - minX;
        ns.y = s.y - minY;
        ns.hFlip = s.hFlip;
        ns.vFlip = s.vFlip;
        ns.priority = s.priority;
        normCol.sprites.append(ns);
    }

    theDef.addNormalizedCollection(normCol);

    // Save the game definition
    QString savePath = theDef.definitionPath();
    if (savePath.isEmpty())
    {
        savePath = QFileDialog::getSaveFileName(this, "Save Game Definition",
            QString(), "JSON Files (*.json)");
        if (savePath.isEmpty())
            return;
    }
    if (!theDef.saveToFile(savePath))
    {
        QMessageBox::critical(this, "Save Failed",
            "Failed to save game definition:\n" + theDef.lastError());
        return;
    }

    // Don't refresh combos — the new collection is saved to the definition
    // file and will appear when the user next loads or switches tabs.
    // Refreshing here would reset the Sprite Collections tab (losing the
    // current recording, frame, selection, and hidden state).

    statusBar()->showMessage(
        QString("Captured %1 sprites as '%2'")
        .arg(normCol.sprites.size()).arg(name));
}

void MainWindow::onHideSelectedSprites()
{
    const QSet<int> & sel = ui->theSpriteColWidget->selectedSpriteIndices();
    const SpriteCollection & col = ui->theSpriteColWidget->collection();
    for (int idx : sel)
    {
        if (idx >= 0 && idx < col.sprites.size())
        {
            const QString & rom = col.sprites[idx].romOffset;
            if (!rom.isEmpty())
                theHiddenRomOffsets.insert(rom);
        }
    }
    applyPersistentHidden();
    statusBar()->showMessage(
        QString("%1 ROM offsets now hidden").arg(theHiddenRomOffsets.size()));
}

void MainWindow::onUnhideSelectedSprites()
{
    theHiddenRomOffsets.clear();
    applyPersistentHidden();
    statusBar()->showMessage("All sprites unhidden");
}

void MainWindow::onUnhideSelectedOnly()
{
    const QSet<int> & sel = ui->theSpriteColWidget->selectedSpriteIndices();
    const SpriteCollection & col = ui->theSpriteColWidget->collection();
    int count = 0;
    for (int idx : sel)
    {
        if (idx >= 0 && idx < col.sprites.size())
        {
            const QString & rom = col.sprites[idx].romOffset;
            if (!rom.isEmpty() && theHiddenRomOffsets.remove(rom))
                ++count;
        }
    }
    applyPersistentHidden();
    statusBar()->showMessage(
        QString("Unhid %1 sprite(s), %2 still hidden")
        .arg(count).arg(theHiddenRomOffsets.size()));
}

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

// ---------------------------------------------------------------------------
// Sprite Viewer: Name Edit, Borders, Edit, Double-click
// ---------------------------------------------------------------------------

void MainWindow::onViewerNameEditFinished()
{
    if (theSelectedCollectionIndex < 0 || !theDef.isNormalized())
        return;

    const auto & normCols = theDef.normalizedCollections();
    if (theSelectedCollectionIndex >= normCols.size())
        return;

    QString newName = ui->theViewerNameEdit->text().trimmed();
    if (newName.isEmpty() || newName == normCols[theSelectedCollectionIndex].name)
        return;

    theDef.renameCollection(theSelectedCollectionIndex, newName);
    theDef.saveToFile(QString());

    // Refresh grid to show new name
    populateCollectionGrid();

    statusBar()->showMessage(QString("Renamed to '%1'").arg(newName));
}

void MainWindow::onViewerBordersToggled(bool checked)
{
    theShowSpriteBorders = checked;
    if (theSelectedCollectionIndex >= 0)
        updateCollectionDetail(theSelectedCollectionIndex);
}

void MainWindow::onEditFromViewer()
{
    if (!theDef.isLoaded() || !theRom.isOpen() || !theDef.isNormalized())
        return;
    if (theSelectedCollectionIndex < 0)
        return;

    int colIndex = theSelectedCollectionIndex;
    const auto & normCols = theDef.normalizedCollections();
    if (colIndex >= normCols.size())
        return;

    SpriteCollection col = buildFromNormalized(normCols[colIndex]);

    theEditCollectionIndex = colIndex;
    theEditSpriteIndex = 0;

    // Build EditorSprite list
    QVector<EditorSprite> edSprites;
    for (int i = 0; i < col.sprites.size(); ++i)
    {
        const CollectionSprite & cs = col.sprites[i];
        EditorSprite es;
        es.widthTiles = cs.widthTiles;
        es.heightTiles = cs.heightTiles;
        es.x = cs.x;
        es.y = cs.y;
        es.hFlip = cs.hFlip;
        es.vFlip = cs.vFlip;
        es.paletteLine = cs.paletteLine;
        es.romOffset = cs.romOffset;

        if (cs.source == "embedded" && !cs.tileData.isEmpty())
            es.tileData = cs.tileData;
        else if (!cs.romOffset.isEmpty() && theRom.isOpen())
        {
            bool ok = false;
            QString offsetStr = cs.romOffset;
            if (offsetStr.startsWith("0x") || offsetStr.startsWith("0X"))
                offsetStr = offsetStr.mid(2);
            uint32_t offset = offsetStr.toUInt(&ok, 16);
            if (ok)
                es.tileData = theRom.readBytes(offset,
                    cs.widthTiles * cs.heightTiles * 32);
        }
        if (es.tileData.isEmpty())
            es.tileData = QByteArray(cs.widthTiles * cs.heightTiles * 32, '\0');

        edSprites.append(es);
    }

    GenesisPalette pals[4];
    for (int i = 0; i < 4; ++i)
    {
        if (i < col.palettes.size() && !col.palettes[i].cramValues.isEmpty())
            pals[i] = TileDecoder::decodePaletteFromCram(col.palettes[i].cramValues);
        else
            pals[i] = TileDecoder::greyPalette();
    }

    ui->theSpritePixelEditor->loadSpriteGroup(edSprites, pals);
    ui->theSpritePixelEditor->setZoom(ui->theEditorZoomSpin->value());
    ui->theSpritePixelEditor->setShowGrid(ui->theEditorGridCheck->isChecked());

    theEditorActivePaletteLine = 0;
    theEditPalLineToId.clear();
    const NormalizedCollection & norm = normCols[colIndex];
    QMap<QString, int> palLineMap;
    for (const auto & ns : norm.sprites)
    {
        if (!palLineMap.contains(ns.paletteId) && palLineMap.size() < 4)
            palLineMap.insert(ns.paletteId, palLineMap.size());
    }
    for (auto it = palLineMap.begin(); it != palLineMap.end(); ++it)
        theEditPalLineToId[it.value()] = it.key();

    ui->theEditorPalette->setPalette(pals[0]);

    bool canSave = false;
    for (const EditorSprite & es : edSprites)
        if (!es.romOffset.isEmpty()) { canSave = true; break; }
    ui->theEditorSaveButton->setEnabled(canSave && theRom.isOpen());

    bool canSavePal = false;
    QString firstPalId = theEditPalLineToId.value(0);
    if (!firstPalId.isEmpty() && theDef.palettePool().contains(firstPalId))
        canSavePal = theDef.palettePool()[firstPalId].romOffset != 0;
    ui->theEditorSavePaletteButton->setEnabled(canSavePal && theRom.isOpen());

    ui->theEditorInfoLabel->setText(
        QString("Group: %1 | %2 sprites | Click to edit")
        .arg(col.name).arg(col.sprites.size()));

    ui->theTabWidget->setCurrentWidget(ui->tabSpriteEditor);
    statusBar()->showMessage(
        QString("Editing sprite group \"%1\" (%2 sprites)")
        .arg(col.name).arg(col.sprites.size()));
}

void MainWindow::onViewerSpriteDoubleClicked(int groupIndex, int spriteIndex, int frameIndex)
{
    (void)spriteIndex;
    (void)frameIndex;
    theSelectedCollectionIndex = groupIndex;
    onEditFromViewer();
}

void MainWindow::onDetailDoubleClicked(int spriteX, int spriteY)
{
    if (theSelectedCollectionIndex < 0 || !theDef.isNormalized())
        return;

    const auto & normCols = theDef.normalizedCollections();
    if (theSelectedCollectionIndex >= normCols.size())
        return;

    const NormalizedCollection & norm = normCols[theSelectedCollectionIndex];
    SpriteCollection col = buildFromNormalized(norm);

    // Convert composite pixel coords to world coords (add bounding box origin)
    int worldX = spriteX + col.boundingBox.x();
    int worldY = spriteY + col.boundingBox.y();

    // Find which sprite contains this point (front-to-back, first = front)
    int hitIndex = -1;
    for (int i = 0; i < col.sprites.size(); ++i)
    {
        const CollectionSprite & cs = col.sprites[i];
        int sprW = cs.widthTiles * 8;
        int sprH = cs.heightTiles * 8;
        QRect sprRect(cs.x, cs.y, sprW, sprH);
        if (sprRect.contains(worldX, worldY))
        {
            hitIndex = i;
            break;
        }
    }

    if (hitIndex < 0)
        return;

    const CollectionSprite & cs = col.sprites[hitIndex];
    if (cs.romOffset.isEmpty())
    {
        statusBar()->showMessage("Sprite has no ROM offset — cannot jump to raw browser");
        return;
    }

    // Parse the ROM offset
    bool ok = false;
    QString offsetStr = cs.romOffset;
    if (offsetStr.startsWith("0x") || offsetStr.startsWith("0X"))
        offsetStr = offsetStr.mid(2);
    uint32_t romOffset = offsetStr.toUInt(&ok, 16);
    if (!ok)
        return;

    // Find the palette for this sprite and its combo index
    int palComboIndex = 0;  // Default greyscale
    if (hitIndex < norm.sprites.size())
    {
        QString palId = norm.sprites[hitIndex].paletteId;
        if (!palId.isEmpty() && theDef.isNormalized())
        {
            const auto & pool = theDef.palettePool();
            int idx = 1;  // Index 0 is "Greyscale (default)"
            for (auto it = pool.begin(); it != pool.end(); ++it, ++idx)
            {
                if (it.key() == palId)
                {
                    palComboIndex = idx;
                    break;
                }
            }
        }
    }

    // Set W and H spinners
    ui->theRawSpriteWSpin->blockSignals(true);
    ui->theRawSpriteHSpin->blockSignals(true);
    ui->theRawSpriteWSpin->setValue(cs.widthTiles);
    ui->theRawSpriteHSpin->setValue(cs.heightTiles);
    ui->theRawSpriteWSpin->blockSignals(false);
    ui->theRawSpriteHSpin->blockSignals(false);

    // Set palette combo
    if (palComboIndex > 0 && palComboIndex < ui->theRawPaletteCombo->count())
        ui->theRawPaletteCombo->setCurrentIndex(palComboIndex);

    // Set jump offset and jump
    ui->theJumpOffsetEdit->setText(QString("0x%1").arg(romOffset, 6, 16, QChar('0')).toUpper());

    // Switch to Raw Tile Browser tab
    ui->theTabWidget->setCurrentWidget(ui->tabRawBrowser);

    // Trigger the jump
    onJumpToOffset();

    statusBar()->showMessage(
        QString("Jumped to sprite at 0x%1 (%2x%3 tiles)")
        .arg(romOffset, 6, 16, QChar('0')).toUpper()
        .arg(cs.widthTiles).arg(cs.heightTiles));
}
