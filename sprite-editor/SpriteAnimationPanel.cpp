#include "SpriteAnimationPanel.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFileInfo>
#include <QInputDialog>
#include <QMessageBox>
#include <iostream>

#define AnimDebug if(0) std::cout

SpriteAnimationPanel::SpriteAnimationPanel(QWidget *parent)
    : QWidget(parent)
    , theDataService(nullptr)
    , theRomFile(nullptr)
    , theDef(nullptr)
    , theActiveRecIndex(-1)
    , theCaptureCounter(0)
{
    buildUi();
}

void SpriteAnimationPanel::buildUi()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(4, 4, 4, 4);

    // Top bar: collection selector + frame + zoom
    QHBoxLayout *controlsRow = new QHBoxLayout();

    controlsRow->addWidget(new QLabel("Collection:"));
    theCollectionCombo = new QComboBox();
    theCollectionCombo->setEnabled(false);
    theCollectionCombo->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    controlsRow->addWidget(theCollectionCombo);

    theFrameLabel = new QLabel("Frame:");
    theFrameLabel->setVisible(false);
    controlsRow->addWidget(theFrameLabel);

    theFrameSpin = new QSpinBox();
    theFrameSpin->setMinimum(0);
    theFrameSpin->setMaximum(0);
    theFrameSpin->setVisible(false);
    controlsRow->addWidget(theFrameSpin);

    theFrameCountLabel = new QLabel();
    theFrameCountLabel->setVisible(false);
    controlsRow->addWidget(theFrameCountLabel);

    theLoadRecordingButton = new QPushButton("Load Recording...");
    controlsRow->addWidget(theLoadRecordingButton);

    controlsRow->addWidget(new QLabel("Zoom:"));
    theZoomSpin = new QSpinBox();
    theZoomSpin->setMinimum(1);
    theZoomSpin->setMaximum(8);
    theZoomSpin->setValue(4);
    controlsRow->addWidget(theZoomSpin);

    mainLayout->addLayout(controlsRow);

    // Selection + capture/hide buttons
    QHBoxLayout *selRow = new QHBoxLayout();

    theSelectionLabel = new QLabel("No sprites selected");
    theSelectionLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
    selRow->addWidget(theSelectionLabel);
    selRow->addStretch(1);

    theCaptureButton = new QPushButton("Capture Sprite Group");
    theCaptureButton->setEnabled(false);
    theCaptureButton->setToolTip("Save selected sprites as a reusable sprite group");
    selRow->addWidget(theCaptureButton);

    theHideButton = new QPushButton("Hide Selected");
    theHideButton->setEnabled(false);
    selRow->addWidget(theHideButton);

    theUnhideSelectedButton = new QPushButton("Unhide Selected");
    theUnhideSelectedButton->setEnabled(false);
    selRow->addWidget(theUnhideSelectedButton);

    theUnhideAllButton = new QPushButton("Unhide All");
    theUnhideAllButton->setEnabled(false);
    selRow->addWidget(theUnhideAllButton);

    mainLayout->addLayout(selRow);

    // Sprite collection widget
    theScrollArea = new QScrollArea();
    theScrollArea->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    theScrollArea->setWidgetResizable(false);
    theSpriteColWidget = new SpriteCollectionWidget();
    theScrollArea->setWidget(theSpriteColWidget);
    mainLayout->addWidget(theScrollArea);

    // Connections
    connect(theCollectionCombo, SIGNAL(currentIndexChanged(int)),
            this,              SLOT(onCollectionSelected(int)));
    connect(theZoomSpin, SIGNAL(valueChanged(int)),
            this,        SLOT(onZoomChanged(int)));
    connect(theFrameSpin, SIGNAL(valueChanged(int)),
            this,         SLOT(onFrameChanged(int)));
    connect(theLoadRecordingButton, &QPushButton::clicked,
            this,                   &SpriteAnimationPanel::onLoadRecordingClicked);
    connect(theSpriteColWidget, &SpriteCollectionWidget::spriteClicked,
            this,               &SpriteAnimationPanel::onSpriteClicked);
    connect(theSpriteColWidget, &SpriteCollectionWidget::selectionChanged,
            this,               &SpriteAnimationPanel::onSelectionChanged);
    connect(theCaptureButton, &QPushButton::clicked,
            this,             &SpriteAnimationPanel::onCaptureGroup);
    connect(theHideButton, &QPushButton::clicked,
            this,          &SpriteAnimationPanel::onHideSelected);
    connect(theUnhideAllButton, &QPushButton::clicked,
            this,               &SpriteAnimationPanel::onUnhideAll);
    connect(theUnhideSelectedButton, &QPushButton::clicked,
            this,                    &SpriteAnimationPanel::onUnhideSelectedOnly);
}

void SpriteAnimationPanel::setDataService(RomDataService *service)
{
    theDataService = service;
}

void SpriteAnimationPanel::setRomFile(RomFile *rom)
{
    theRomFile = rom;
}

void SpriteAnimationPanel::setGameDefinition(GameDefinition *def)
{
    theDef = def;
}

void SpriteAnimationPanel::addRecording(const SpriteRecording & rec)
{
    theRecordings.append(rec);
}

void SpriteAnimationPanel::populateCollections()
{
    theCollectionCombo->blockSignals(true);
    theCollectionCombo->clear();

    for (int i = 0; i < theRecordings.size(); ++i)
    {
        const SpriteRecording & rec = theRecordings[i];
        QString label = QString("Recording: %1 (%2 frames)")
            .arg(rec.gameName()).arg(rec.frames().size());
        theCollectionCombo->addItem(label);
    }

    theCollectionCombo->setEnabled(theRecordings.size() > 0);
    theCollectionCombo->blockSignals(false);

    if (!theRecordings.isEmpty())
        onCollectionSelected(0);
    else
        theSpriteColWidget->clearCollection();
}

// ---------------------------------------------------------------------------
// Slots
// ---------------------------------------------------------------------------

void SpriteAnimationPanel::onCollectionSelected(int index)
{
    if (index < 0)
    {
        theSpriteColWidget->clearCollection();
        return;
    }

    theHiddenRomOffsets.clear();
    theSpriteColWidget->clearHiddenSprites();
    theSpriteColWidget->clearSelection();
    theSelectionLabel->setText("No sprites selected");
    theCaptureButton->setEnabled(false);
    theHideButton->setEnabled(false);
    theUnhideAllButton->setEnabled(false);
    theUnhideSelectedButton->setEnabled(false);
    theActiveRecIndex = -1;

    if (index < 0 || index >= theRecordings.size())
    {
        theSpriteColWidget->clearCollection();
        return;
    }

    theActiveRecIndex = index;
    const SpriteRecording & rec = theRecordings[index];
    int frameCount = rec.frames().size();

    theFrameLabel->setVisible(true);
    theFrameSpin->setVisible(true);
    theFrameCountLabel->setVisible(true);

    theFrameSpin->blockSignals(true);
    theFrameSpin->setMinimum(0);
    theFrameSpin->setMaximum(frameCount - 1);
    theFrameSpin->setValue(0);
    theFrameSpin->blockSignals(false);
    theFrameCountLabel->setText(QString("/ %1").arg(frameCount));

    displayRecordingFrame(index, 0);
}

void SpriteAnimationPanel::onZoomChanged(int value)
{
    theSpriteColWidget->setZoom(value);
}

void SpriteAnimationPanel::onFrameChanged(int frameIndex)
{
    theSpriteColWidget->clearSelection();
    theSelectionLabel->setText("No sprites selected");
    theCaptureButton->setEnabled(false);
    theHideButton->setEnabled(false);
    theUnhideSelectedButton->setEnabled(false);

    if (theActiveRecIndex >= 0)
    {
        displayRecordingFrame(theActiveRecIndex, frameIndex);
        applyPersistentHidden();
    }
}

void SpriteAnimationPanel::onLoadRecordingClicked()
{
    emit loadRecordingRequested();
}

void SpriteAnimationPanel::onSpriteClicked(int spriteIndex)
{
    Q_UNUSED(spriteIndex);

    // Show sprite info in status for quick reference
    const SpriteCollection & col = theSpriteColWidget->collection();
    if (spriteIndex >= 0 && spriteIndex < col.sprites.size())
    {
        const CollectionSprite & cs = col.sprites[spriteIndex];
        QString info = QString("Sprite %1: %2x%3 | VRAM: %4 | %5")
            .arg(spriteIndex)
            .arg(cs.widthTiles).arg(cs.heightTiles)
            .arg(cs.vramAddr.isEmpty() ? "?" : cs.vramAddr)
            .arg(cs.romOffset.isEmpty() ? cs.source : cs.romOffset);
        emit statusMessage(info);
    }
}

void SpriteAnimationPanel::onSelectionChanged(const QSet<int> & selectedIndices)
{
    int count = selectedIndices.size();
    if (count == 0)
    {
        theSelectionLabel->setText("No sprites selected");
    }
    else if (count == 1)
    {
        // Single sprite selected — show detailed address info
        int idx = *selectedIndices.begin();
        const SpriteCollection & col = theSpriteColWidget->collection();
        if (idx >= 0 && idx < col.sprites.size())
        {
            const CollectionSprite & cs = col.sprites[idx];
            QString info = QString("Sprite %1: %2x%3 tiles")
                .arg(idx).arg(cs.widthTiles).arg(cs.heightTiles);
            if (!cs.vramAddr.isEmpty())
                info += QString(" | VRAM: %1").arg(cs.vramAddr);
            if (!cs.romOffset.isEmpty())
            {
                bool isRam = false;
                QString offStr = cs.romOffset;
                if (offStr.startsWith("0x") || offStr.startsWith("0X"))
                    offStr = offStr.mid(2);
                uint32_t addr = offStr.toUInt(&isRam, 16);
                Q_UNUSED(addr);
                info += QString(" | %1: %2")
                    .arg(addr >= 0xFF0000 ? "RAM" : "ROM")
                    .arg(cs.romOffset);
            }
            info += QString(" | Palette: %1").arg(cs.paletteLine);

            // Show CRAM address for the palette line
            if (cs.paletteLine >= 0 && cs.paletteLine < col.palettes.size())
            {
                const ScreenCapturePalette & pal = col.palettes[cs.paletteLine];
                if (!pal.dmaSource.isEmpty())
                    info += QString(" (CRAM DMA: %1)").arg(pal.dmaSource);
            }

            theSelectionLabel->setText(info);
        }
        else
        {
            theSelectionLabel->setText("1 sprite selected");
        }
    }
    else
    {
        theSelectionLabel->setText(QString("%1 sprites selected").arg(count));
    }

    bool hasSelection = count > 0;
    theCaptureButton->setEnabled(hasSelection);
    theHideButton->setEnabled(hasSelection);
    theUnhideAllButton->setEnabled(!theHiddenRomOffsets.isEmpty());

    // Check if any selected sprites are hidden
    bool hasHiddenSelected = false;
    const SpriteCollection & col = theSpriteColWidget->collection();
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
    theUnhideSelectedButton->setEnabled(hasHiddenSelected);
}

void SpriteAnimationPanel::onCaptureGroup()
{
    if (!theDef)
        return;

    const QSet<int> & selectedIndices = theSpriteColWidget->selectedSpriteIndices();
    if (selectedIndices.isEmpty())
        return;

    // Get the current collection from the recording frame
    if (theActiveRecIndex < 0 || theActiveRecIndex >= theRecordings.size())
        return;

    int frameIndex = theFrameSpin->value();
    SpriteCollection col;
    if (theDataService)
    {
        TileBlockGroup group = theDataService->resolveRecordingFrame(
            theRecordings[theActiveRecIndex], frameIndex);
        // Convert TileBlockGroup back to SpriteCollection for capture
        // (The capture code still works with SpriteCollection)
        col.name = group.name;
        col.boundingBox = group.boundingBox;
        for (int i = 0; i < 4; ++i)
        {
            ScreenCapturePalette scp;
            scp.line = i;
            // We'd need CRAM values — use recording palettes directly
            col.palettes.append(scp);
        }
        // Use the recording's palettes directly
        col.palettes = theRecordings[theActiveRecIndex].palettes();

        for (const TileBlock & block : group.blocks)
        {
            CollectionSprite cs;
            cs.widthTiles = block.widthTiles;
            cs.heightTiles = block.heightTiles;
            cs.x = block.x;
            cs.y = block.y;
            cs.hFlip = block.hFlip;
            cs.vFlip = block.vFlip;
            cs.paletteLine = block.paletteLine;
            cs.romOffset = block.romOffset;
            cs.source = block.source;
            cs.tileData = block.tileData;
            col.sprites.append(cs);
        }
    }

    // Auto-generate name and ID
    ++theCaptureCounter;
    QString name = QString("Capture %1").arg(theCaptureCounter);
    QString id = QString("capture_%1").arg(theCaptureCounter);
    while (theDef->hasCollectionId(id))
    {
        ++theCaptureCounter;
        name = QString("Capture %1").arg(theCaptureCounter);
        id = QString("capture_%1").arg(theCaptureCounter);
    }

    if (!theDef->isLoaded())
        theDef->initEmpty("Captured Sprites", "captured");
    if (!theDef->isNormalized())
        theDef->ensureNormalized();

    // Calculate min X/Y for normalization
    int minX = 32767, minY = 32767;
    for (int idx : selectedIndices)
    {
        if (idx < 0 || idx >= col.sprites.size()) continue;
        const CollectionSprite & s = col.sprites[idx];
        if (s.x < minX) minX = s.x;
        if (s.y < minY) minY = s.y;
    }

    // Build palettes, patterns, and collection
    QMap<int, QString> palLineToId;
    QMap<QString, QString> romToPatId;
    NormalizedCollection normCol;
    normCol.id = id;
    normCol.name = name;

    for (int idx : selectedIndices)
    {
        if (idx < 0 || idx >= col.sprites.size()) continue;
        const CollectionSprite & s = col.sprites[idx];

        int palLine = qBound(0, s.paletteLine, 3);
        if (!palLineToId.contains(palLine))
        {
            QString palId = QString("%1_pal%2").arg(id).arg(palLine);
            if (!theDef->hasPaletteId(palId))
            {
                PoolPalette pp;
                pp.id = palId;
                pp.name = QString("%1 Palette Line %2").arg(name).arg(palLine);
                pp.romOffset = 0;
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
                theDef->addPoolPalette(pp);
            }
            palLineToId[palLine] = palId;
        }

        QString patKey = s.romOffset.isEmpty()
            ? QString("embedded_%1_%2").arg(idx).arg(s.vramAddr)
            : s.romOffset;
        if (!romToPatId.contains(patKey))
        {
            QString patId = QString("%1_pat_%2x%3_%4")
                .arg(id).arg(s.widthTiles).arg(s.heightTiles)
                .arg(romToPatId.size());
            if (!theDef->hasPatternId(patId))
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
                theDef->addPoolPattern(pat);
            }
            romToPatId[patKey] = patId;
        }

        NormalizedSprite ns;
        ns.patternId = romToPatId[patKey];
        ns.frame = 0;
        ns.paletteId = palLineToId[palLine];
        ns.x = s.x - minX;
        ns.y = s.y - minY;
        ns.hFlip = s.hFlip;
        ns.vFlip = s.vFlip;
        ns.priority = s.priority;
        ns.vramAddr = s.vramAddr;
        normCol.sprites.append(ns);
    }

    theDef->addNormalizedCollection(normCol);
    theDef->saveToFile(QString());

    emit statusMessage(
        QString("Captured %1 sprites as '%2'")
        .arg(normCol.sprites.size()).arg(name));
    emit collectionsCaptured();
}

void SpriteAnimationPanel::onHideSelected()
{
    const QSet<int> & sel = theSpriteColWidget->selectedSpriteIndices();
    const SpriteCollection & col = theSpriteColWidget->collection();
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
    theUnhideAllButton->setEnabled(!theHiddenRomOffsets.isEmpty());
    emit statusMessage(QString("Hidden %1 sprites").arg(sel.size()));
}

void SpriteAnimationPanel::onUnhideAll()
{
    theHiddenRomOffsets.clear();
    theSpriteColWidget->clearHiddenSprites();
    theUnhideAllButton->setEnabled(false);
    theUnhideSelectedButton->setEnabled(false);
}

void SpriteAnimationPanel::onUnhideSelectedOnly()
{
    const QSet<int> & sel = theSpriteColWidget->selectedSpriteIndices();
    const SpriteCollection & col = theSpriteColWidget->collection();
    for (int idx : sel)
    {
        if (idx >= 0 && idx < col.sprites.size())
        {
            const QString & rom = col.sprites[idx].romOffset;
            if (!rom.isEmpty())
                theHiddenRomOffsets.remove(rom);
        }
    }
    applyPersistentHidden();
    theUnhideAllButton->setEnabled(!theHiddenRomOffsets.isEmpty());
}

void SpriteAnimationPanel::displayRecordingFrame(int recIndex, int frameIndex)
{
    if (recIndex < 0 || recIndex >= theRecordings.size())
        return;

    const SpriteRecording & rec = theRecordings[recIndex];
    if (frameIndex < 0 || frameIndex >= rec.frames().size())
        return;

    // Build SpriteCollection from frame
    const AnimationFrame & frame = rec.frames()[frameIndex];
    SpriteCollection col;
    col.name = QString("%1 - frame %2").arg(rec.gameName()).arg(frame.frameNumber);
    col.boundingBox = frame.boundingBox;
    col.sprites = frame.sprites;
    col.palettes = rec.palettes();

    theSpriteColWidget->setCollection(col, theRomFile);
    theSpriteColWidget->setZoom(theZoomSpin->value());

    emit statusMessage(
        QString("Recording frame %1 (VDP frame %2): %3 sprites")
        .arg(frameIndex).arg(frame.frameNumber).arg(frame.sprites.size()));
}

void SpriteAnimationPanel::applyPersistentHidden()
{
    const SpriteCollection & col = theSpriteColWidget->collection();
    QSet<int> indexSet;
    for (int i = 0; i < col.sprites.size(); ++i)
    {
        const QString & rom = col.sprites[i].romOffset;
        if (!rom.isEmpty() && theHiddenRomOffsets.contains(rom))
            indexSet.insert(i);
    }
    theSpriteColWidget->setHiddenSprites(indexSet);
    theUnhideAllButton->setEnabled(!theHiddenRomOffsets.isEmpty());
}
