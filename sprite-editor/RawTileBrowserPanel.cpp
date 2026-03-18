#include "RawTileBrowserPanel.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <iostream>

RawTileBrowserPanel::RawTileBrowserPanel(QWidget *parent)
    : QWidget(parent)
    , theDataService(nullptr)
    , theSelectedTileIndex(-1)
    , theSelectedRomOffset(0)
{
    buildUi();
}

void RawTileBrowserPanel::buildUi()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(4, 4, 4, 4);

    // Controls row
    QHBoxLayout *controlsRow = new QHBoxLayout();

    controlsRow->addWidget(new QLabel("Range:"));
    theRangeCombo = new QComboBox();
    theRangeCombo->setEnabled(false);
    theRangeCombo->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    controlsRow->addWidget(theRangeCombo);

    controlsRow->addWidget(new QLabel("Palette:"));
    thePaletteCombo = new QComboBox();
    thePaletteCombo->setEnabled(false);
    controlsRow->addWidget(thePaletteCombo);

    controlsRow->addWidget(new QLabel("Zoom:"));
    theZoomSpin = new QSpinBox();
    theZoomSpin->setMinimum(1);
    theZoomSpin->setMaximum(8);
    theZoomSpin->setValue(4);
    controlsRow->addWidget(theZoomSpin);

    controlsRow->addWidget(new QLabel("Jump:"));
    theJumpOffsetEdit = new QLineEdit();
    theJumpOffsetEdit->setPlaceholderText("0x000000");
    theJumpOffsetEdit->setMaximumWidth(90);
    theJumpOffsetEdit->setEnabled(false);
    controlsRow->addWidget(theJumpOffsetEdit);

    theJumpButton = new QPushButton("Go");
    theJumpButton->setMaximumWidth(36);
    theJumpButton->setEnabled(false);
    controlsRow->addWidget(theJumpButton);

    theAssemblyStartButton = new QPushButton("Start");
    theAssemblyStartButton->setToolTip("Set the Jump offset as the sprite assembly starting point");
    theAssemblyStartButton->setMaximumWidth(46);
    theAssemblyStartButton->setEnabled(false);
    controlsRow->addWidget(theAssemblyStartButton);

    controlsRow->addWidget(new QLabel("Tiles W x H:"));
    theSpriteWSpin = new QSpinBox();
    theSpriteWSpin->setMinimum(1);
    theSpriteWSpin->setMaximum(8);
    theSpriteWSpin->setValue(1);
    theSpriteWSpin->setMaximumWidth(50);
    theSpriteWSpin->setEnabled(false);
    controlsRow->addWidget(theSpriteWSpin);

    controlsRow->addWidget(new QLabel("x"));
    theSpriteHSpin = new QSpinBox();
    theSpriteHSpin->setMinimum(1);
    theSpriteHSpin->setMaximum(8);
    theSpriteHSpin->setValue(1);
    theSpriteHSpin->setMaximumWidth(50);
    theSpriteHSpin->setEnabled(false);
    controlsRow->addWidget(theSpriteHSpin);

    mainLayout->addLayout(controlsRow);

    // Tile browser canvas
    theScrollArea = new QScrollArea();
    theScrollArea->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    theScrollArea->setWidgetResizable(true);
    theBrowser = new RawTileBrowserWidget();
    theScrollArea->setWidget(theBrowser);
    mainLayout->addWidget(theScrollArea);

    // Action buttons
    QHBoxLayout *actionRow = new QHBoxLayout();
    theExportButton = new QPushButton("Export PNG");
    theExportButton->setEnabled(false);
    theExportButton->setToolTip("Export the selected sprite/tile as a PNG image");
    actionRow->addWidget(theExportButton);
    actionRow->addStretch(1);
    mainLayout->addLayout(actionRow);

    // Info label
    theInfoLabel = new QLabel("Click a tile to see its ROM offset");
    theInfoLabel->setFrameShape(QFrame::StyledPanel);
    mainLayout->addWidget(theInfoLabel);

    // Connections
    connect(theRangeCombo,   SIGNAL(currentIndexChanged(int)),
            this,            SLOT(onRangeChanged(int)));
    connect(thePaletteCombo, SIGNAL(currentIndexChanged(int)),
            this,            SLOT(onPaletteChanged(int)));
    connect(theZoomSpin,     SIGNAL(valueChanged(int)),
            this,            SLOT(onZoomChanged(int)));
    connect(theBrowser,      &RawTileBrowserWidget::tileClicked,
            this,            &RawTileBrowserPanel::onTileClicked);
    connect(theJumpButton,   &QPushButton::clicked,
            this,            &RawTileBrowserPanel::onJumpToOffset);
    connect(theJumpOffsetEdit, &QLineEdit::returnPressed,
            this,              &RawTileBrowserPanel::onJumpToOffset);
    connect(theSpriteWSpin,  SIGNAL(valueChanged(int)),
            this,            SLOT(onSpriteSizeChanged(int)));
    connect(theSpriteHSpin,  SIGNAL(valueChanged(int)),
            this,            SLOT(onSpriteSizeChanged(int)));
    connect(theAssemblyStartButton, &QPushButton::clicked,
            this,                   &RawTileBrowserPanel::onSetAssemblyStart);
    connect(theExportButton, &QPushButton::clicked,
            this,            &RawTileBrowserPanel::onExportPng);
}

void RawTileBrowserPanel::setDataService(RomDataService *service)
{
    theDataService = service;
}

void RawTileBrowserPanel::populateRanges()
{
    theRangeCombo->blockSignals(true);
    theRangeCombo->clear();

    if (!theDataService)
        return;

    GameDefinition *def = theDataService->definition();
    RomFile *rom = theDataService->rom();

    if (def && def->isLoaded())
    {
        for (const auto & r : def->tileRanges())
            theRangeCombo->addItem(r.label);
    }
    else if (rom && rom->isOpen())
    {
        theRangeCombo->addItem(
            QString("Full ROM (0x200 - 0x%1)")
            .arg(rom->romSize(), 0, 16).toUpper());
    }

    bool hasRom = rom && rom->isOpen();
    theRangeCombo->setEnabled(theRangeCombo->count() > 0);
    theRangeCombo->blockSignals(false);
    theJumpOffsetEdit->setEnabled(hasRom);
    theJumpButton->setEnabled(hasRom);
    theSpriteWSpin->setEnabled(hasRom);
    theSpriteHSpin->setEnabled(hasRom);
    theAssemblyStartButton->setEnabled(hasRom);

    if (theRangeCombo->count() > 0)
        refresh();
}

void RawTileBrowserPanel::populatePalettes()
{
    thePaletteCombo->blockSignals(true);
    thePaletteCombo->clear();

    thePaletteCombo->addItem("Greyscale (default)");

    if (theDataService)
    {
        QVector<PaletteInfo> pals = theDataService->availablePalettes();
        for (const PaletteInfo & pi : pals)
            thePaletteCombo->addItem(pi.name);
    }

    RomFile *rom = theDataService ? theDataService->rom() : nullptr;
    thePaletteCombo->setEnabled(rom && rom->isOpen());
    thePaletteCombo->blockSignals(false);
}

void RawTileBrowserPanel::refresh()
{
    if (!theDataService)
        return;

    RomFile *rom = theDataService->rom();
    GameDefinition *def = theDataService->definition();
    if (!rom || !rom->isOpen())
        return;

    uint32_t startOffset = 0x200;
    uint32_t endOffset = (uint32_t)rom->romSize();

    int rangeIdx = theRangeCombo->currentIndex();
    if (def && def->isLoaded() && rangeIdx >= 0 &&
        rangeIdx < def->tileRanges().size())
    {
        const TileRange & r = def->tileRanges()[rangeIdx];
        startOffset = r.startOffset;
        endOffset = qMin(r.endOffset, (uint32_t)rom->romSize());
    }

    if (endOffset <= startOffset)
        return;

    uint32_t length = endOffset - startOffset;
    const uint32_t MAX_BROWSE_BYTES = 256 * 1024;
    if (length > MAX_BROWSE_BYTES)
        length = MAX_BROWSE_BYTES;
    length = (length / 32) * 32;

    QByteArray tileData = rom->readBytes(startOffset, length);

    // Resolve palette
    GenesisPalette pal = TileDecoder::greyPalette();
    int palIdx = thePaletteCombo->currentIndex();
    if (palIdx > 0 && theDataService)
    {
        QVector<PaletteInfo> pals = theDataService->availablePalettes();
        int poolIdx = palIdx - 1;
        if (poolIdx >= 0 && poolIdx < pals.size())
            pal = pals[poolIdx].colors;
    }

    theBrowser->setTileData(tileData, startOffset, pal);
    theBrowser->setZoom(theZoomSpin->value());

    theInfoLabel->setText(
        QString("Showing %1 tiles from ROM 0x%2 - 0x%3  |  Click a tile for its offset")
        .arg(tileData.size() / 32)
        .arg(startOffset, 6, 16, QChar('0')).toUpper()
        .arg(startOffset + tileData.size(), 6, 16, QChar('0')).toUpper());
}

void RawTileBrowserPanel::jumpToAddress(uint32_t romOffset, int widthTiles,
                                         int heightTiles, int paletteComboIndex)
{
    theSpriteWSpin->setValue(widthTiles);
    theSpriteHSpin->setValue(heightTiles);

    if (paletteComboIndex > 0 && paletteComboIndex < thePaletteCombo->count())
        thePaletteCombo->setCurrentIndex(paletteComboIndex);

    theJumpOffsetEdit->setText(QString("0x%1").arg(romOffset, 6, 16, QChar('0')).toUpper());
    onJumpToOffset();
}

// ---------------------------------------------------------------------------
// Slots
// ---------------------------------------------------------------------------

void RawTileBrowserPanel::onRangeChanged(int)
{
    refresh();
}

void RawTileBrowserPanel::onPaletteChanged(int)
{
    refresh();
}

void RawTileBrowserPanel::onZoomChanged(int value)
{
    theBrowser->setZoom(value);
}

void RawTileBrowserPanel::onSpriteSizeChanged(int)
{
    theBrowser->setSpriteSize(theSpriteWSpin->value(), theSpriteHSpin->value());
}

void RawTileBrowserPanel::onTileClicked(int tileIndex, uint32_t romOffset)
{
    theSelectedTileIndex = tileIndex;
    theSelectedRomOffset = romOffset;

    int w = theSpriteWSpin->value();
    int h = theSpriteHSpin->value();
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
    theInfoLabel->setText(label);
    theExportButton->setEnabled(true);

    emit tileSelected(tileIndex, romOffset);
}

void RawTileBrowserPanel::onJumpToOffset()
{
    if (!theDataService)
        return;

    GameDefinition *def = theDataService->definition();
    QString text = theJumpOffsetEdit->text().trimmed();
    bool ok = false;
    uint32_t targetOffset = text.toUInt(&ok, 16);
    if (!ok)
        targetOffset = text.toUInt(&ok, 0);
    if (!ok)
    {
        theInfoLabel->setText("Jump failed: enter a valid hex offset (e.g. 0x032D80)");
        return;
    }

    int rangeIdx = theRangeCombo->currentIndex();
    uint32_t rangeStart = 0x200;
    if (def && def->isLoaded() && rangeIdx < def->tileRanges().size())
        rangeStart = def->tileRanges()[rangeIdx].startOffset;

    if (targetOffset < rangeStart)
    {
        theInfoLabel->setText(
            QString("0x%1 is before the current range start (0x%2). Switch ranges.")
            .arg(targetOffset, 6, 16, QChar('0')).toUpper()
            .arg(rangeStart, 6, 16, QChar('0')).toUpper());
        return;
    }

    uint32_t byteOffset = targetOffset - rangeStart;
    int tileIndex = int(byteOffset / 32);
    if (tileIndex >= theBrowser->tileCount())
    {
        theInfoLabel->setText(
            QString("0x%1 is beyond the current range end.")
            .arg(targetOffset, 6, 16, QChar('0')).toUpper());
        return;
    }

    theBrowser->scrollToTile(tileIndex);
    uint32_t actualOffset = rangeStart + uint32_t(tileIndex) * 32;
    theInfoLabel->setText(
        QString("Jumped to tile %1  |  ROM offset: 0x%2")
        .arg(tileIndex)
        .arg(actualOffset, 6, 16, QChar('0')).toUpper());
}

void RawTileBrowserPanel::onSetAssemblyStart()
{
    if (!theDataService)
        return;

    GameDefinition *def = theDataService->definition();
    QString text = theJumpOffsetEdit->text().trimmed();
    bool ok = false;
    uint32_t targetOffset = text.toUInt(&ok, 16);
    if (!ok)
        targetOffset = text.toUInt(&ok, 0);
    if (!ok)
    {
        theInfoLabel->setText("Set start failed: enter a valid hex offset");
        return;
    }

    int rangeIdx = theRangeCombo->currentIndex();
    uint32_t rangeStart = 0x200;
    if (def && def->isLoaded() && rangeIdx < def->tileRanges().size())
        rangeStart = def->tileRanges()[rangeIdx].startOffset;

    if (targetOffset < rangeStart)
    {
        theInfoLabel->setText(
            QString("0x%1 is before the current range start.")
            .arg(targetOffset, 6, 16, QChar('0')).toUpper());
        return;
    }

    uint32_t byteOffset = targetOffset - rangeStart;
    int tileIndex = int(byteOffset / 32);
    if (tileIndex >= theBrowser->tileCount())
    {
        theInfoLabel->setText(
            QString("0x%1 is beyond the current range end.")
            .arg(targetOffset, 6, 16, QChar('0')).toUpper());
        return;
    }

    theBrowser->setAssemblyStart(tileIndex);
    theBrowser->scrollToTile(tileIndex);
    uint32_t actualOffset = rangeStart + uint32_t(tileIndex) * 32;
    theInfoLabel->setText(
        QString("Assembly starts at tile %1  |  ROM offset: 0x%2")
        .arg(tileIndex)
        .arg(actualOffset, 6, 16, QChar('0')).toUpper());
}

void RawTileBrowserPanel::onExportPng()
{
    if (!theDataService || theSelectedTileIndex < 0)
        return;

    RomFile *rom = theDataService->rom();
    if (!rom || !rom->isOpen())
        return;

    int w = theSpriteWSpin->value();
    int h = theSpriteHSpin->value();
    int bytesNeeded = w * h * 32;

    QByteArray tileData = rom->readBytes(theSelectedRomOffset, bytesNeeded);
    if (tileData.size() < bytesNeeded)
    {
        emit statusMessage("Not enough ROM data to export this sprite.");
        return;
    }

    // Use current palette
    GenesisPalette pal = TileDecoder::greyPalette();
    int palIdx = thePaletteCombo->currentIndex();
    if (palIdx > 0)
    {
        QVector<PaletteInfo> pals = theDataService->availablePalettes();
        int poolIdx = palIdx - 1;
        if (poolIdx >= 0 && poolIdx < pals.size())
            pal = pals[poolIdx].colors;
    }

    QImage img = TileDecoder::decodeSprite(tileData, w, h, pal);
    if (img.isNull())
    {
        emit statusMessage("Failed to decode sprite for export.");
        return;
    }

    QString suggestedName = QString("sprite_0x%1_%2x%3.png")
        .arg(theSelectedRomOffset, 6, 16, QChar('0')).toUpper()
        .arg(w).arg(h);

    emit exportPngRequested(img, suggestedName);
}
