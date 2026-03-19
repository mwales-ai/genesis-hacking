#include "SpriteEditorPanel.h"
#include "GenesisColorDialog.h"
#include "TileDecoder.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QMessageBox>
#include <iostream>

#define EdPanelDebug if(0) std::cout

SpriteEditorPanel::SpriteEditorPanel(QWidget *parent)
    : QWidget(parent)
    , theRomFile(nullptr)
    , theDef(nullptr)
    , theCollectionIndex(-1)
    , theSpriteIndex(-1)
    , theActivePaletteLine(0)
    , theSelectedGroupSpriteIndex(-1)
{
    buildUi();
}

void SpriteEditorPanel::buildUi()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(4, 4, 4, 4);

    // Info label
    theInfoLabel = new QLabel("No sprite selected");
    theInfoLabel->setWordWrap(true);
    mainLayout->addWidget(theInfoLabel);

    // Main area: canvas on left, tool panel on right
    QHBoxLayout *mainArea = new QHBoxLayout();

    theScrollArea = new QScrollArea();
    theScrollArea->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    theScrollArea->setWidgetResizable(false);
    thePixelEditor = new SpritePixelEditor();
    theScrollArea->setWidget(thePixelEditor);
    mainArea->addWidget(theScrollArea);

    theToolPanel = new PaintToolPanel();
    theToolPanel->setFixedWidth(120);
    theToolPanel->setDeleteButtonVisible(true);
    mainArea->addWidget(theToolPanel);

    mainLayout->addLayout(mainArea);

    // Hidden PaletteWidget for CRAM data
    thePaletteHidden = new PaletteWidget();
    thePaletteHidden->setVisible(false);

    // Bottom bar
    QHBoxLayout *bottomBar = new QHBoxLayout();

    bottomBar->addWidget(new QLabel("Zoom:"));
    theZoomSpin = new QSpinBox();
    theZoomSpin->setMinimum(1);
    theZoomSpin->setMaximum(32);
    theZoomSpin->setValue(8);
    bottomBar->addWidget(theZoomSpin);

    theGridCheck = new QCheckBox("Grid");
    theGridCheck->setChecked(true);
    bottomBar->addWidget(theGridCheck);

    bottomBar->addStretch(1);

    theSaveButton = new QPushButton("Save Tiles to ROM");
    theSaveButton->setEnabled(false);
    bottomBar->addWidget(theSaveButton);

    theSavePaletteButton = new QPushButton("Save Palette to ROM");
    theSavePaletteButton->setEnabled(false);
    bottomBar->addWidget(theSavePaletteButton);

    theCloseButton = new QPushButton("Close");
    bottomBar->addWidget(theCloseButton);

    mainLayout->addLayout(bottomBar);

    // Connections
    connect(thePixelEditor, &SpritePixelEditor::groupPaletteLineChanged,
            this,           &SpriteEditorPanel::onGroupPaletteLineChanged);
    connect(thePixelEditor, &SpritePixelEditor::colorPicked,
            this,           &SpriteEditorPanel::onColorPicked);
    connect(thePixelEditor, &SpritePixelEditor::groupSpriteSelected,
            this,           &SpriteEditorPanel::onGroupSpriteSelected);

    connect(thePaletteHidden, &PaletteWidget::colorSelected,
            this,             &SpriteEditorPanel::onPaletteSelected);
    connect(thePaletteHidden, &PaletteWidget::colorEditRequested,
            this,             &SpriteEditorPanel::onPaletteEditRequested);

    connect(theToolPanel, &PaintToolPanel::toolChanged,
            this,         &SpriteEditorPanel::onToolChanged);
    connect(theToolPanel, &PaintToolPanel::brushSizeChanged,
            this,         &SpriteEditorPanel::onBrushSizeChanged);
    connect(theToolPanel, &PaintToolPanel::colorSelected,
            this,         &SpriteEditorPanel::onPaletteSelected);
    connect(theToolPanel, &PaintToolPanel::deleteRequested,
            this,         &SpriteEditorPanel::onDeleteSpriteFromGroup);

    connect(theSaveButton, &QPushButton::clicked,
            this,          &SpriteEditorPanel::onSave);
    connect(theSavePaletteButton, &QPushButton::clicked,
            this,                 &SpriteEditorPanel::onSavePalette);
    connect(theCloseButton, &QPushButton::clicked,
            this,           &SpriteEditorPanel::onClose);
    connect(theZoomSpin, SIGNAL(valueChanged(int)),
            this,        SLOT(onZoomChanged(int)));
    connect(theGridCheck, &QCheckBox::toggled,
            this,         &SpriteEditorPanel::onGridToggled);
}

void SpriteEditorPanel::setRomFile(RomFile *rom)
{
    theRomFile = rom;
}

void SpriteEditorPanel::setGameDefinition(GameDefinition *def)
{
    theDef = def;
}

void SpriteEditorPanel::editNormalizedCollection(int collectionIndex,
                                                  const QMap<int, QString> & palLineToId)
{
    if (!theDef || !theDef->isNormalized())
        return;

    const auto & normCols = theDef->normalizedCollections();
    if (collectionIndex < 0 || collectionIndex >= normCols.size())
        return;

    const NormalizedCollection & norm = normCols[collectionIndex];
    theCollectionIndex = collectionIndex;
    theSpriteIndex = 0;
    thePalLineToId = palLineToId;
    theActivePaletteLine = 0;
    theSelectedGroupSpriteIndex = -1;
    theToolPanel->setDeleteButtonEnabled(false);

    // Build SpriteCollection via same logic as buildFromNormalized
    // (reuse the existing code path through the definition)
    const auto & palPool = theDef->palettePool();
    const auto & patPool = theDef->patternPool();

    // Build EditorSprite list
    QVector<EditorSprite> edSprites;
    GenesisPalette pals[4];

    // Resolve palettes
    for (auto it = palLineToId.begin(); it != palLineToId.end(); ++it)
    {
        int line = it.key();
        const QString & palId = it.value();
        if (line < 0 || line > 3) continue;

        if (palPool.contains(palId))
        {
            const PoolPalette & pp = palPool[palId];
            if (!pp.cramValues.isEmpty())
                pals[line] = TileDecoder::decodePaletteFromCram(pp.cramValues);
            else if (pp.romOffset != 0 && theRomFile && theRomFile->isOpen())
            {
                QByteArray palData = theRomFile->readBytes(pp.romOffset, 32);
                QVector<uint16_t> cramVals;
                for (int i = 0; i + 1 < palData.size() && cramVals.size() < 16; i += 2)
                    cramVals.append((uint8_t(palData[i]) << 8) | uint8_t(palData[i + 1]));
                if (!cramVals.isEmpty())
                    pals[line] = TileDecoder::decodePaletteFromCram(cramVals);
            }
        }
    }

    // Build sprites
    QMap<QString, int> palIdToLine;
    for (auto it = palLineToId.begin(); it != palLineToId.end(); ++it)
        palIdToLine[it.value()] = it.key();

    for (const NormalizedSprite & ns : norm.sprites)
    {
        EditorSprite es;
        es.x = ns.x;
        es.y = ns.y;
        es.hFlip = ns.hFlip;
        es.vFlip = ns.vFlip;
        es.paletteLine = palIdToLine.value(ns.paletteId, 0);

        if (patPool.contains(ns.patternId))
        {
            const PoolPattern & pat = patPool[ns.patternId];
            es.widthTiles = pat.widthTiles;
            es.heightTiles = pat.heightTiles;

            if (!pat.tileData.isEmpty())
            {
                es.tileData = pat.tileData;
                es.romOffset = "";
            }
            else if (pat.romOffset != 0)
            {
                int bytesPerFrame = pat.widthTiles * pat.heightTiles * 32;
                uint32_t frameOffset = pat.romOffset + uint32_t(ns.frame * bytesPerFrame);
                es.romOffset = QString("0x%1").arg(frameOffset, 0, 16).toUpper();
                if (theRomFile && theRomFile->isOpen())
                    es.tileData = theRomFile->readBytes(frameOffset, bytesPerFrame);
            }
        }
        else
        {
            es.widthTiles = 1;
            es.heightTiles = 1;
        }

        edSprites.append(es);
    }

    thePixelEditor->loadSpriteGroup(edSprites, pals);
    thePixelEditor->setZoom(theZoomSpin->value());
    thePixelEditor->setShowGrid(theGridCheck->isChecked());

    thePaletteHidden->setPalette(pals[0]);
    theToolPanel->setPalette(pals[0]);

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
    theSaveButton->setEnabled(canSave && theRomFile && theRomFile->isOpen());

    bool canSavePal = false;
    QString firstPalId = palLineToId.value(0);
    if (!firstPalId.isEmpty() && palPool.contains(firstPalId))
        canSavePal = palPool[firstPalId].romOffset != 0;
    theSavePaletteButton->setEnabled(canSavePal && theRomFile && theRomFile->isOpen());

    theInfoLabel->setText(
        QString("Group: %1 | %2 sprites | Click to edit")
        .arg(norm.name).arg(edSprites.size()));
}

void SpriteEditorPanel::editLegacySprite(const SpriteCollection & col,
                                           int collectionIndex, int spriteIndex)
{
    if (spriteIndex < 0 || spriteIndex >= col.sprites.size())
        return;

    theCollectionIndex = collectionIndex;
    theSpriteIndex = spriteIndex;
    theActivePaletteLine = 0;
    thePalLineToId.clear();
    theSelectedGroupSpriteIndex = -1;
    theToolPanel->setDeleteButtonEnabled(false);

    const CollectionSprite & sprite = col.sprites[spriteIndex];
    int palLine = qBound(0, sprite.paletteLine, 3);

    GenesisPalette pal = TileDecoder::greyPalette();
    if (palLine < col.palettes.size() && !col.palettes[palLine].cramValues.isEmpty())
        pal = TileDecoder::decodePaletteFromCram(col.palettes[palLine].cramValues);

    QByteArray tileBytes = sprite.tileData;
    if (tileBytes.isEmpty() && !sprite.romOffset.isEmpty() && theRomFile && theRomFile->isOpen())
    {
        bool ok = false;
        QString offStr = sprite.romOffset;
        if (offStr.startsWith("0x") || offStr.startsWith("0X"))
            offStr = offStr.mid(2);
        uint32_t offset = offStr.toUInt(&ok, 16);
        if (ok)
            tileBytes = theRomFile->readBytes(offset, sprite.widthTiles * sprite.heightTiles * 32);
    }

    thePixelEditor->loadSprite(tileBytes, sprite.widthTiles, sprite.heightTiles,
                                pal, sprite.hFlip, sprite.vFlip);
    thePixelEditor->setZoom(theZoomSpin->value());
    thePixelEditor->setShowGrid(theGridCheck->isChecked());

    thePaletteHidden->setPalette(pal);
    theToolPanel->setPalette(pal);

    theSaveButton->setEnabled(!sprite.romOffset.isEmpty() && theRomFile && theRomFile->isOpen());
    theSavePaletteButton->setEnabled(false);

    theInfoLabel->setText(
        QString("Sprite: %1x%2 tiles | ROM: %3")
        .arg(sprite.widthTiles).arg(sprite.heightTiles)
        .arg(sprite.romOffset.isEmpty() ? "embedded" : sprite.romOffset));
}

bool SpriteEditorPanel::hasUnsavedEdits() const
{
    return thePixelEditor->isModified();
}

void SpriteEditorPanel::closeEditor()
{
    onClose();
}

// ---------------------------------------------------------------------------
// Slots
// ---------------------------------------------------------------------------

void SpriteEditorPanel::onGroupPaletteLineChanged(int paletteLine)
{
    theActivePaletteLine = paletteLine;
    GenesisPalette pal = thePixelEditor->groupPalette(paletteLine);
    thePaletteHidden->setPalette(pal);
    theToolPanel->setPalette(pal);

    // Update save palette button
    if (theDef && theDef->isNormalized())
    {
        QString palId = thePalLineToId.value(paletteLine);
        bool canSavePal = false;
        if (!palId.isEmpty() && theDef->palettePool().contains(palId))
            canSavePal = theDef->palettePool()[palId].romOffset != 0;
        theSavePaletteButton->setEnabled(canSavePal && theRomFile && theRomFile->isOpen());
    }

    emit statusMessage(QString("Palette line %1 selected").arg(paletteLine));
}

void SpriteEditorPanel::onPaletteSelected(int index)
{
    thePixelEditor->setPenIndex(index);
    thePaletteHidden->setSelectedIndex(index);
    theToolPanel->setSelectedColor(index);
    emit statusMessage(
        QString("Pen color: index %1  CRAM: 0x%2")
        .arg(index)
        .arg(thePaletteHidden->selectedCramWord(), 4, 16, QChar('0')).toUpper());
}

void SpriteEditorPanel::onPaletteEditRequested(int index)
{
    QColor current = thePaletteHidden->palette()[index];

    // Determine if we can save this palette
    bool hasRomOffset = false;
    uint32_t romOffset = 0;
    int refCount = 0;

    if (thePixelEditor->isGroupMode() && theDef && theDef->isNormalized())
    {
        QString palId = thePalLineToId.value(theActivePaletteLine);
        if (!palId.isEmpty() && theDef->palettePool().contains(palId))
        {
            romOffset = theDef->palettePool()[palId].romOffset;
            hasRomOffset = (romOffset != 0);
            refCount = theDef->countPaletteReferences(palId);
        }
    }

    emit colorEditRequested(index, current, hasRomOffset, romOffset, refCount);
}

void SpriteEditorPanel::applyColorEdit(int paletteIndex, const QColor & newColor, uint16_t cramWord)
{
    Q_UNUSED(cramWord);
    thePaletteHidden->setColorAt(paletteIndex, newColor);

    if (thePixelEditor->isGroupMode())
    {
        thePixelEditor->updateGroupPalette(theActivePaletteLine,
                                            thePaletteHidden->palette());
    }
    else
    {
        thePixelEditor->updatePalette(thePaletteHidden->palette());
    }

    theToolPanel->setPalette(thePaletteHidden->palette());
}

void SpriteEditorPanel::onSave()
{
    if (!theRomFile || !theRomFile->isOpen() || !thePixelEditor->isModified())
    {
        emit statusMessage("Nothing to save.");
        return;
    }

    if (thePixelEditor->isGroupMode())
    {
        int saved = 0;
        int count = thePixelEditor->groupSpriteCount();
        for (int i = 0; i < count; ++i)
        {
            const EditorSprite & es = thePixelEditor->groupSprite(i);
            if (es.romOffset.isEmpty())
                continue;

            bool ok = false;
            QString offStr = es.romOffset;
            if (offStr.startsWith("0x") || offStr.startsWith("0X"))
                offStr = offStr.mid(2);
            uint32_t offset = offStr.toUInt(&ok, 16);
            if (!ok) continue;

            QByteArray data = thePixelEditor->modifiedGroupTileData(i);
            theRomFile->writeBytes(offset, data);
            ++saved;
        }

        emit statusMessage(QString("Saved %1 sprite tile blocks to ROM").arg(saved));
    }
    else
    {
        // Single sprite mode would need the sprite's ROM offset
        // For now just report
        emit statusMessage("Single sprite save — use File > Save ROM to persist");
    }

    emit tilesSavedToRom();
}

void SpriteEditorPanel::onSavePalette()
{
    if (!theRomFile || !theRomFile->isOpen())
        return;

    if (!theDef || !theDef->isNormalized())
        return;

    QString palId = thePalLineToId.value(theActivePaletteLine);
    if (palId.isEmpty() || !theDef->palettePool().contains(palId))
    {
        emit statusMessage("No palette assigned to active line");
        return;
    }

    uint32_t romOffset = theDef->palettePool()[palId].romOffset;
    if (romOffset == 0)
    {
        emit statusMessage("Palette has no ROM offset — cannot save");
        return;
    }

    QByteArray palData = TileDecoder::encodePalette(thePaletteHidden->palette());
    theRomFile->writeBytes(romOffset, palData);

    emit statusMessage(
        QString("Saved palette to ROM at 0x%1").arg(romOffset, 0, 16).toUpper());
    emit paletteSavedToRom();
}

void SpriteEditorPanel::onClose()
{
    thePixelEditor->clearSprite();
    theSaveButton->setEnabled(false);
    theSavePaletteButton->setEnabled(false);
    theInfoLabel->setText("No sprite selected");
    theCollectionIndex = -1;
    theSpriteIndex = -1;
    theSelectedGroupSpriteIndex = -1;
    theToolPanel->setDeleteButtonEnabled(false);
    emit editorClosed();
}

void SpriteEditorPanel::onZoomChanged(int value)
{
    thePixelEditor->setZoom(value);
}

void SpriteEditorPanel::onGridToggled(bool checked)
{
    thePixelEditor->setShowGrid(checked);
}

void SpriteEditorPanel::onToolChanged(EditorTool tool)
{
    thePixelEditor->setTool(tool);
}

void SpriteEditorPanel::onBrushSizeChanged(int size)
{
    thePixelEditor->setBrushSize(size);
}

void SpriteEditorPanel::onColorPicked(int paletteIndex)
{
    thePixelEditor->setPenIndex(paletteIndex);
    thePaletteHidden->setSelectedIndex(paletteIndex);
    theToolPanel->setSelectedColor(paletteIndex);
}

void SpriteEditorPanel::onGroupSpriteSelected(int spriteIndex)
{
    theSelectedGroupSpriteIndex = spriteIndex;
    theToolPanel->setDeleteButtonEnabled(spriteIndex >= 0);

    if (!thePixelEditor->isGroupMode() || spriteIndex < 0)
        return;

    const EditorSprite & es = thePixelEditor->groupSprite(spriteIndex);
    QString info = QString("Sprite %1: %2x%3 tiles | ROM: %4 | Palette: %5")
        .arg(spriteIndex)
        .arg(es.widthTiles).arg(es.heightTiles)
        .arg(es.romOffset.isEmpty() ? "embedded" : es.romOffset)
        .arg(es.paletteLine);
    theInfoLabel->setText(info);
}

void SpriteEditorPanel::onDeleteSpriteFromGroup()
{
    if (theSelectedGroupSpriteIndex < 0 || !theDef || !theDef->isNormalized())
        return;

    const auto & normCols = theDef->normalizedCollections();
    if (theCollectionIndex < 0 || theCollectionIndex >= normCols.size())
        return;

    NormalizedCollection norm = normCols[theCollectionIndex];
    if (theSelectedGroupSpriteIndex >= norm.sprites.size())
        return;

    // Remove the sprite
    norm.sprites.removeAt(theSelectedGroupSpriteIndex);
    theDef->removeNormalizedCollection(theCollectionIndex);
    theDef->addNormalizedCollection(norm);
    int newIdx = theDef->normalizedCollections().size() - 1;
    if (newIdx != theCollectionIndex)
        theDef->moveNormalizedCollection(newIdx, theCollectionIndex);
    theDef->saveToFile(QString());

    theSelectedGroupSpriteIndex = -1;
    theToolPanel->setDeleteButtonEnabled(false);

    emit statusMessage("Sprite removed from group");
    emit spriteDeletedFromGroup(theCollectionIndex);
}
