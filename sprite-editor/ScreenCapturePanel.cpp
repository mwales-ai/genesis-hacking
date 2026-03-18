#include "ScreenCapturePanel.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFileInfo>
#include <QSet>

ScreenCapturePanel::ScreenCapturePanel(QWidget *parent)
    : QWidget(parent)
    , theRomFile(nullptr)
    , theDef(nullptr)
    , theDefCaptureCount(0)
{
    buildUi();
}

void ScreenCapturePanel::buildUi()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(4, 4, 4, 4);

    // Top bar: capture selector + zoom
    QHBoxLayout *controlsRow = new QHBoxLayout();
    controlsRow->addWidget(new QLabel("Capture:"));

    theCaptureCombo = new QComboBox();
    theCaptureCombo->setEnabled(false);
    theCaptureCombo->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    controlsRow->addWidget(theCaptureCombo);

    controlsRow->addWidget(new QLabel("Zoom:"));
    theZoomSpin = new QSpinBox();
    theZoomSpin->setMinimum(1);
    theZoomSpin->setMaximum(8);
    theZoomSpin->setValue(2);
    controlsRow->addWidget(theZoomSpin);

    mainLayout->addLayout(controlsRow);

    // Action buttons row
    QHBoxLayout *actionRow = new QHBoxLayout();

    theLoadButton = new QPushButton("Load Capture...");
    theLoadButton->setToolTip("Load a screen capture JSON file from blastem");
    actionRow->addWidget(theLoadButton);

    theAddToDefButton = new QPushButton("Add to Definition");
    theAddToDefButton->setEnabled(false);
    theAddToDefButton->setToolTip("Save the displayed screen capture into the game definition file");
    actionRow->addWidget(theAddToDefButton);

    theRemoveButton = new QPushButton("Remove");
    theRemoveButton->setEnabled(false);
    theRemoveButton->setToolTip("Remove this screen capture from the game definition");
    actionRow->addWidget(theRemoveButton);

    theEditButton = new QPushButton("Edit");
    theEditButton->setCheckable(true);
    theEditButton->setEnabled(false);
    theEditButton->setToolTip("Toggle tile editing mode for this screen capture");
    actionRow->addWidget(theEditButton);

    actionRow->addStretch(1);
    mainLayout->addLayout(actionRow);

    // Main area: tile map + optional tool panel
    QHBoxLayout *mainArea = new QHBoxLayout();

    theScrollArea = new QScrollArea();
    theScrollArea->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    theScrollArea->setWidgetResizable(false);
    theTileMapWidget = new TileMapWidget();
    theScrollArea->setWidget(theTileMapWidget);
    mainArea->addWidget(theScrollArea);

    theToolPanel = new PaintToolPanel();
    theToolPanel->setFixedWidth(120);
    theToolPanel->setVisible(false);
    mainArea->addWidget(theToolPanel);

    mainLayout->addLayout(mainArea);

    // Save/Revert row (shown in edit mode)
    QHBoxLayout *editBar = new QHBoxLayout();

    theSaveRomButton = new QPushButton("Save Changes to ROM");
    theSaveRomButton->setEnabled(false);
    theSaveRomButton->setVisible(false);
    theSaveRomButton->setToolTip("Write modified tile data to the ROM file");
    editBar->addWidget(theSaveRomButton);

    theRevertButton = new QPushButton("Revert to Original");
    theRevertButton->setEnabled(false);
    theRevertButton->setVisible(false);
    theRevertButton->setToolTip("Discard all painting changes and reload from ROM");
    editBar->addWidget(theRevertButton);

    editBar->addStretch(1);
    mainLayout->addLayout(editBar);

    // Status label
    theStatusLabel = new QLabel("No capture loaded");
    theStatusLabel->setWordWrap(true);
    theStatusLabel->setFrameShape(QFrame::StyledPanel);
    mainLayout->addWidget(theStatusLabel);

    // Connections
    connect(theCaptureCombo, SIGNAL(currentIndexChanged(int)),
            this,            SLOT(onCaptureSelected(int)));
    connect(theZoomSpin, SIGNAL(valueChanged(int)),
            this,        SLOT(onZoomChanged(int)));
    connect(theLoadButton, &QPushButton::clicked,
            this,          &ScreenCapturePanel::onLoadClicked);
    connect(theAddToDefButton, &QPushButton::clicked,
            this,              &ScreenCapturePanel::onAddToDef);
    connect(theRemoveButton, &QPushButton::clicked,
            this,            &ScreenCapturePanel::onRemoveClicked);
    connect(theEditButton, &QPushButton::toggled,
            this,          &ScreenCapturePanel::onEditToggled);
    connect(theSaveRomButton, &QPushButton::clicked,
            this,             &ScreenCapturePanel::onSaveToRom);
    connect(theRevertButton, &QPushButton::clicked,
            this,            &ScreenCapturePanel::onRevert);

    connect(theTileMapWidget, &TileMapWidget::colorPicked,
            this,             &ScreenCapturePanel::onColorPicked);
    connect(theTileMapWidget, &TileMapWidget::tileHovered,
            this,             &ScreenCapturePanel::onTileHovered);

    connect(theToolPanel, &PaintToolPanel::toolChanged,
            this,         &ScreenCapturePanel::onToolChanged);
    connect(theToolPanel, &PaintToolPanel::brushSizeChanged,
            this,         &ScreenCapturePanel::onBrushSizeChanged);
    connect(theToolPanel, &PaintToolPanel::colorSelected,
            this,         &ScreenCapturePanel::onPaletteSelected);
}

void ScreenCapturePanel::setRomFile(RomFile *rom)
{
    theRomFile = rom;
}

void ScreenCapturePanel::setGameDefinition(GameDefinition *def)
{
    theDef = def;
}

void ScreenCapturePanel::populateCaptures()
{
    theCaptureCombo->blockSignals(true);
    theCaptureCombo->clear();

    theDefCaptureCount = 0;
    if (theDef && theDef->isLoaded())
    {
        const auto & defCaptures = theDef->screenCaptures();
        theDefCaptureCount = defCaptures.size();
        for (const auto & cap : defCaptures)
            theCaptureCombo->addItem(cap.name);
    }

    for (const auto & cap : theLoadedCaptures)
        theCaptureCombo->addItem(QString("[ext] %1").arg(cap.name));

    bool hasEntries = (theDefCaptureCount + theLoadedCaptures.size()) > 0;
    theCaptureCombo->setEnabled(hasEntries);
    theCaptureCombo->blockSignals(false);

    if (hasEntries)
        onCaptureSelected(0);
    else
        theTileMapWidget->clearCapture();
}

void ScreenCapturePanel::loadCaptureFile(const QString & path)
{
    if (path.isEmpty())
        return;

    QVector<ScreenCapture> caps;
    if (!GameDefinition::loadScreenCaptureFromFile(path, caps))
    {
        emit statusMessage("Failed to load screen capture from: " + path);
        return;
    }

    theLoadedCaptures.append(caps);
    populateCaptures();

    int firstNew = theDefCaptureCount + theLoadedCaptures.size() - caps.size();
    theCaptureCombo->setCurrentIndex(firstNew);

    emit statusMessage(
        QString("Loaded %1 screen capture(s) from %2")
        .arg(caps.size()).arg(QFileInfo(path).fileName()));
}

void ScreenCapturePanel::confirmRemoveCapture(int defIndex)
{
    if (!theDef || defIndex < 0 || defIndex >= theDef->screenCaptures().size())
        return;

    theDef->removeScreenCapture(defIndex);
    theDef->saveToFile(QString());
    populateCaptures();
}

// ---------------------------------------------------------------------------
// Private slots
// ---------------------------------------------------------------------------

void ScreenCapturePanel::onCaptureSelected(int index)
{
    if (index < 0)
    {
        theTileMapWidget->clearCapture();
        theAddToDefButton->setEnabled(false);
        theRemoveButton->setEnabled(false);
        theStatusLabel->setText("No capture loaded");
        return;
    }

    const ScreenCapture *cap = nullptr;
    bool isFromDef = false;

    if (theDef && index < theDefCaptureCount)
    {
        const auto & captures = theDef->screenCaptures();
        if (index < captures.size())
        {
            cap = &captures[index];
            isFromDef = true;
        }
    }
    else
    {
        int extIdx = index - theDefCaptureCount;
        if (extIdx >= 0 && extIdx < theLoadedCaptures.size())
            cap = &theLoadedCaptures[extIdx];
    }

    if (!cap)
    {
        theTileMapWidget->clearCapture();
        theAddToDefButton->setEnabled(false);
        theRemoveButton->setEnabled(false);
        return;
    }

    theTileMapWidget->setScreenCapture(*cap, theRomFile);

    theAddToDefButton->setEnabled(!isFromDef && theDef && theDef->isLoaded());
    theRemoveButton->setEnabled(isFromDef);
    theEditButton->setEnabled(true);

    // Build status info
    int totalTiles = cap->tileMap.size();
    QSet<QString> uniqueRomOffsets;
    QSet<int> uniquePatterns;
    for (const TileMapEntry & e : cap->tileMap)
    {
        uniquePatterns.insert(e.pattern);
        if (!e.romOffset.isEmpty())
            uniqueRomOffsets.insert(e.romOffset);
    }

    int palCount = cap->palettes.size();
    int palWithRom = 0;
    for (const ScreenCapturePalette & pal : cap->palettes)
    {
        if (!pal.dmaSource.isEmpty())
            ++palWithRom;
    }

    QString status = QString("%1 tiles (%2 unique patterns) | ROM addresses: %3/%4\n"
                             "Palettes: %5 (%6 with ROM address)")
        .arg(totalTiles)
        .arg(uniquePatterns.size())
        .arg(uniqueRomOffsets.size())
        .arg(totalTiles)
        .arg(palCount)
        .arg(palWithRom);
    theStatusLabel->setText(status);

    emit statusMessage(
        QString("Screen capture: %1 (%2x%3 tiles)")
            .arg(cap->name)
            .arg(cap->widthTiles)
            .arg(cap->heightTiles));
}

void ScreenCapturePanel::onZoomChanged(int value)
{
    theTileMapWidget->setZoom(value);
}

void ScreenCapturePanel::onLoadClicked()
{
    emit loadCaptureRequested();
}

void ScreenCapturePanel::onAddToDef()
{
    int index = theCaptureCombo->currentIndex();
    if (index < theDefCaptureCount)
        return;

    int extIdx = index - theDefCaptureCount;
    if (extIdx < 0 || extIdx >= theLoadedCaptures.size())
        return;

    if (!theDef || !theDef->isLoaded())
    {
        emit statusMessage("Load or create a game definition first.");
        return;
    }

    ScreenCapture cap = theLoadedCaptures.takeAt(extIdx);
    theDef->addScreenCapture(cap);
    theDef->saveToFile(QString());
    populateCaptures();

    theCaptureCombo->setCurrentIndex(theDefCaptureCount - 1);
    emit statusMessage(QString("Added screen capture '%1' to game definition").arg(cap.name));
    emit captureAddedToDefinition();
}

void ScreenCapturePanel::onRemoveClicked()
{
    int index = theCaptureCombo->currentIndex();
    if (index < 0 || index >= theDefCaptureCount)
        return;

    if (!theDef)
        return;

    const auto & captures = theDef->screenCaptures();
    if (index >= captures.size())
        return;

    QString name = captures[index].name;
    emit removeCaptureRequested(index, name);
}

void ScreenCapturePanel::onEditToggled(bool checked)
{
    theTileMapWidget->setEditMode(checked);
    theToolPanel->setVisible(checked);
    theSaveRomButton->setVisible(checked);
    theRevertButton->setVisible(checked);

    if (checked)
    {
        int palLine = 0;
        theTileMapWidget->setActivePaletteLine(palLine);
        theToolPanel->setPalette(theTileMapWidget->palette(palLine));
        theTileMapWidget->setTool(TOOL_PENCIL);

        theSaveRomButton->setEnabled(theRomFile && theRomFile->isOpen());
        theRevertButton->setEnabled(true);
        emit statusMessage("Screen capture edit mode ON");
    }
    else
    {
        emit statusMessage("Screen capture edit mode OFF");
    }
}

void ScreenCapturePanel::onColorPicked(int paletteIndex)
{
    theTileMapWidget->setPenIndex(paletteIndex);
    theToolPanel->setSelectedColor(paletteIndex);
}

void ScreenCapturePanel::onSaveToRom()
{
    emit saveRomRequested();
}

void ScreenCapturePanel::onRevert()
{
    int index = theCaptureCombo->currentIndex();
    if (index >= 0)
    {
        onCaptureSelected(index);
        emit statusMessage("Reverted screen capture to original ROM data");
    }
}

void ScreenCapturePanel::onToolChanged(EditorTool tool)
{
    theTileMapWidget->setTool(tool);
}

void ScreenCapturePanel::onBrushSizeChanged(int size)
{
    theTileMapWidget->setBrushSize(size);
}

void ScreenCapturePanel::onPaletteSelected(int index)
{
    theTileMapWidget->setPenIndex(index);
    theToolPanel->setSelectedColor(index);
}

void ScreenCapturePanel::onTileHovered(int row, int col, const QString & romOffset,
                                        int pattern, int paletteLine, const QString & source)
{
    QString info = QString("Tile (%1,%2) | Pattern: %3 | Palette: %4 | Source: %5 | ROM: %6")
        .arg(col).arg(row)
        .arg(pattern)
        .arg(paletteLine)
        .arg(source)
        .arg(romOffset.isEmpty() ? "N/A" : romOffset);
    theStatusLabel->setText(info);
}
