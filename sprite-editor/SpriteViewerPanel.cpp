#include "SpriteViewerPanel.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPainter>
#include <QMessageBox>
#include <iostream>

#define ViewerDebug if(0) std::cout

SpriteViewerPanel::SpriteViewerPanel(QWidget *parent)
    : QWidget(parent)
    , theDataService(nullptr)
    , theDef(nullptr)
    , theSelectedIndex(-1)
    , theShowBorders(false)
{
    buildUi();
}

void SpriteViewerPanel::buildUi()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(4, 4, 4, 4);

    theSplitter = new QSplitter(Qt::Horizontal);

    // Left: collection grid + thumb zoom
    QWidget *leftPanel = new QWidget();
    QVBoxLayout *leftLayout = new QVBoxLayout(leftPanel);
    leftLayout->setContentsMargins(0, 0, 0, 0);

    QScrollArea *gridScroll = new QScrollArea();
    gridScroll->setWidgetResizable(true);
    theSpriteSheet = new SpriteSheetWidget();
    gridScroll->setWidget(theSpriteSheet);
    leftLayout->addWidget(gridScroll);

    QHBoxLayout *thumbRow = new QHBoxLayout();
    thumbRow->addWidget(new QLabel("Thumb Zoom:"));
    theThumbZoomSpin = new QSpinBox();
    theThumbZoomSpin->setMinimum(1);
    theThumbZoomSpin->setMaximum(8);
    theThumbZoomSpin->setValue(2);
    thumbRow->addWidget(theThumbZoomSpin);
    thumbRow->addStretch(1);
    leftLayout->addLayout(thumbRow);

    theSplitter->addWidget(leftPanel);

    // Right: detail panel
    QWidget *rightPanel = new QWidget();
    QVBoxLayout *rightLayout = new QVBoxLayout(rightPanel);
    rightLayout->setContentsMargins(0, 0, 0, 0);

    QHBoxLayout *nameRow = new QHBoxLayout();
    nameRow->addWidget(new QLabel("Name:"));
    theNameEdit = new QLineEdit();
    theNameEdit->setPlaceholderText("Select a collection");
    nameRow->addWidget(theNameEdit);
    rightLayout->addLayout(nameRow);

    QScrollArea *detailScroll = new QScrollArea();
    detailScroll->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);
    detailScroll->setWidgetResizable(false);
    theSpriteDetail = new TileCanvasWidget();
    detailScroll->setWidget(theSpriteDetail);
    rightLayout->addWidget(detailScroll);

    QHBoxLayout *controlsRow = new QHBoxLayout();
    controlsRow->addWidget(new QLabel("Zoom:"));
    theZoomSpin = new QSpinBox();
    theZoomSpin->setMinimum(1);
    theZoomSpin->setMaximum(8);
    theZoomSpin->setValue(4);
    controlsRow->addWidget(theZoomSpin);

    theBordersCheck = new QCheckBox("Show Borders");
    controlsRow->addWidget(theBordersCheck);

    theEditButton = new QPushButton("Edit");
    theEditButton->setEnabled(false);
    theEditButton->setToolTip("Open the selected collection in the Sprite Editor");
    controlsRow->addWidget(theEditButton);

    theDeleteButton = new QPushButton("Delete");
    theDeleteButton->setEnabled(false);
    theDeleteButton->setToolTip("Delete the selected sprite group from the game definition");
    controlsRow->addWidget(theDeleteButton);
    rightLayout->addLayout(controlsRow);

    theInfoLabel = new QLabel("No collection selected");
    theInfoLabel->setWordWrap(true);
    theInfoLabel->setFrameShape(QFrame::StyledPanel);
    theInfoLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
    rightLayout->addWidget(theInfoLabel);

    theSplitter->addWidget(rightPanel);
    mainLayout->addWidget(theSplitter);

    // Connections
    connect(theSpriteSheet, &SpriteSheetWidget::spriteSelected,
            this,           &SpriteViewerPanel::onGridSelected);
    connect(theSpriteSheet, &SpriteSheetWidget::spriteDoubleClicked,
            this,           &SpriteViewerPanel::onDoubleClicked);
    connect(theSpriteSheet, &SpriteSheetWidget::spriteReordered,
            this,           &SpriteViewerPanel::onReordered);
    connect(theSpriteDetail, &TileCanvasWidget::doubleClicked,
            this,            &SpriteViewerPanel::onDetailDoubleClicked);
    connect(theZoomSpin, SIGNAL(valueChanged(int)),
            this,        SLOT(onZoomChanged(int)));
    connect(theThumbZoomSpin, SIGNAL(valueChanged(int)),
            this,             SLOT(onThumbZoomChanged(int)));
    connect(theNameEdit, &QLineEdit::editingFinished,
            this,        &SpriteViewerPanel::onNameEditFinished);
    connect(theBordersCheck, &QCheckBox::toggled,
            this,            &SpriteViewerPanel::onBordersToggled);
    connect(theEditButton, &QPushButton::clicked,
            this,          &SpriteViewerPanel::onEditClicked);
    connect(theDeleteButton, &QPushButton::clicked,
            this,            &SpriteViewerPanel::onDeleteClicked);
}

void SpriteViewerPanel::setDataService(RomDataService *service)
{
    theDataService = service;
}

void SpriteViewerPanel::setGameDefinition(GameDefinition *def)
{
    theDef = def;
}

void SpriteViewerPanel::populateGrid()
{
    theSelectedIndex = -1;

    if (!theDef || !theDef->isLoaded() || !theDef->isNormalized() ||
        !theDataService || !theDataService->rom() || !theDataService->rom()->isOpen())
    {
        theSpriteSheet->clearSprites();
        theSpriteDetail->clearSprite();
        theNameEdit->clear();
        theInfoLabel->setText("No collections available");
        theEditButton->setEnabled(false);
        theDeleteButton->setEnabled(false);
        return;
    }

    const auto & normCols = theDef->normalizedCollections();
    QVector<SpriteThumb> thumbs;

    for (int i = 0; i < normCols.size(); ++i)
    {
        const NormalizedCollection & norm = normCols[i];
        TileBlockGroup group = theDataService->resolveNormalized(norm);
        QImage composite = theDataService->renderComposite(group);
        if (composite.isNull())
        {
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

    theSpriteSheet->setSprites(thumbs);
    theSpriteDetail->clearSprite();
    theNameEdit->clear();
    theInfoLabel->setText(QString("%1 collections loaded").arg(normCols.size()));
    theEditButton->setEnabled(false);
    theDeleteButton->setEnabled(false);
}

// ---------------------------------------------------------------------------
// Slots
// ---------------------------------------------------------------------------

void SpriteViewerPanel::onGridSelected(int groupIndex, int, int)
{
    theSelectedIndex = groupIndex;
    updateDetail(groupIndex);
}

void SpriteViewerPanel::onZoomChanged(int value)
{
    theSpriteDetail->setZoom(value);
}

void SpriteViewerPanel::onThumbZoomChanged(int value)
{
    theSpriteSheet->setThumbZoom(value);
}

void SpriteViewerPanel::onReordered(int fromIndex, int toIndex)
{
    if (!theDef) return;
    theDef->moveNormalizedCollection(fromIndex, toIndex);
    theDef->saveToFile(QString());
    populateGrid();
    emit statusMessage(QString("Moved collection from position %1 to %2")
                       .arg(fromIndex).arg(toIndex));
}

void SpriteViewerPanel::onNameEditFinished()
{
    if (theSelectedIndex < 0 || !theDef || !theDef->isNormalized())
        return;

    const auto & normCols = theDef->normalizedCollections();
    if (theSelectedIndex >= normCols.size())
        return;

    QString newName = theNameEdit->text().trimmed();
    if (newName.isEmpty() || newName == normCols[theSelectedIndex].name)
        return;

    theDef->renameCollection(theSelectedIndex, newName);
    theDef->saveToFile(QString());
    populateGrid();
    emit statusMessage(QString("Renamed to '%1'").arg(newName));
}

void SpriteViewerPanel::onBordersToggled(bool checked)
{
    theShowBorders = checked;
    if (theSelectedIndex >= 0)
        updateDetail(theSelectedIndex);
}

void SpriteViewerPanel::onEditClicked()
{
    if (!theDef || !theDef->isNormalized() || theSelectedIndex < 0)
        return;

    const auto & normCols = theDef->normalizedCollections();
    if (theSelectedIndex >= normCols.size())
        return;

    // Build palette line -> ID mapping
    const NormalizedCollection & norm = normCols[theSelectedIndex];
    QMap<QString, int> palLineMap;
    for (const auto & ns : norm.sprites)
    {
        if (!palLineMap.contains(ns.paletteId) && palLineMap.size() < 4)
            palLineMap.insert(ns.paletteId, palLineMap.size());
    }
    QMap<int, QString> palLineToId;
    for (auto it = palLineMap.begin(); it != palLineMap.end(); ++it)
        palLineToId[it.value()] = it.key();

    emit editRequested(theSelectedIndex, palLineToId);
}

void SpriteViewerPanel::onDeleteClicked()
{
    if (theSelectedIndex < 0 || !theDef || !theDef->isNormalized())
        return;

    const auto & normCols = theDef->normalizedCollections();
    if (theSelectedIndex >= normCols.size())
        return;

    QString name = normCols[theSelectedIndex].name;
    QMessageBox::StandardButton reply = QMessageBox::question(
        this, "Delete Sprite Group",
        QString("Delete '%1' from the game definition?").arg(name),
        QMessageBox::Yes | QMessageBox::No);

    if (reply != QMessageBox::Yes)
        return;

    theDef->removeNormalizedCollection(theSelectedIndex);
    theDef->saveToFile(QString());
    theSelectedIndex = -1;
    populateGrid();
    emit statusMessage(QString("Deleted sprite group '%1'").arg(name));
    emit collectionDeleted();
}

void SpriteViewerPanel::onDoubleClicked(int groupIndex, int, int)
{
    theSelectedIndex = groupIndex;
    onEditClicked();
}

void SpriteViewerPanel::onDetailDoubleClicked(int spriteX, int spriteY)
{
    if (theSelectedIndex < 0 || !theDef || !theDef->isNormalized() || !theDataService)
        return;

    const auto & normCols = theDef->normalizedCollections();
    if (theSelectedIndex >= normCols.size())
        return;

    const NormalizedCollection & norm = normCols[theSelectedIndex];
    TileBlockGroup group = theDataService->resolveNormalized(norm);

    // Convert composite pixel coords to world coords
    int worldX = spriteX + group.boundingBox.x();
    int worldY = spriteY + group.boundingBox.y();

    // Find which block contains this point (front-to-back)
    int hitIndex = -1;
    for (int i = 0; i < group.blocks.size(); ++i)
    {
        const TileBlock & block = group.blocks[i];
        int sprW = block.widthTiles * 8;
        int sprH = block.heightTiles * 8;
        QRect r(block.x, block.y, sprW, sprH);
        if (r.contains(worldX, worldY))
        {
            hitIndex = i;
            break;
        }
    }

    if (hitIndex < 0)
        return;

    const TileBlock & block = group.blocks[hitIndex];
    if (block.romOffset.isEmpty())
    {
        emit statusMessage("Sprite has no ROM offset — cannot jump to raw browser");
        return;
    }

    bool ok = false;
    QString offsetStr = block.romOffset;
    if (offsetStr.startsWith("0x") || offsetStr.startsWith("0X"))
        offsetStr = offsetStr.mid(2);
    uint32_t romOffset = offsetStr.toUInt(&ok, 16);
    if (!ok)
        return;

    // Find palette combo index
    int palComboIndex = 0;
    if (hitIndex < norm.sprites.size())
    {
        QString palId = norm.sprites[hitIndex].paletteId;
        if (!palId.isEmpty() && theDef->isNormalized())
        {
            const auto & pool = theDef->palettePool();
            int idx = 1;
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

    emit jumpToRawBrowser(romOffset, block.widthTiles, block.heightTiles, palComboIndex);
    emit statusMessage(
        QString("Jumped to sprite at 0x%1 (%2x%3 tiles)")
        .arg(romOffset, 6, 16, QChar('0')).toUpper()
        .arg(block.widthTiles).arg(block.heightTiles));
}

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

void SpriteViewerPanel::updateDetail(int collectionIndex)
{
    if (!theDef || !theDataService)
        return;

    const auto & normCols = theDef->normalizedCollections();
    if (collectionIndex < 0 || collectionIndex >= normCols.size())
        return;

    const NormalizedCollection & norm = normCols[collectionIndex];
    theNameEdit->setText(norm.name);

    // Render composite (never bake borders — use overlays)
    TileBlockGroup group = theDataService->resolveNormalized(norm);
    QImage composite = theDataService->renderComposite(group);
    if (!composite.isNull())
    {
        theSpriteDetail->setSprite(composite);
        theSpriteDetail->setZoom(theZoomSpin->value());

        if (theShowBorders)
        {
            static const QColor borderColors[4] = {
                QColor(255, 255, 0), QColor(0, 255, 255),
                QColor(255, 0, 255), QColor(0, 255, 0)
            };
            int originX = group.boundingBox.x();
            int originY = group.boundingBox.y();

            QVector<SpriteOverlayRect> overlays;
            for (const TileBlock & block : group.blocks)
            {
                SpriteOverlayRect ovr;
                ovr.rect = QRect(block.x - originX, block.y - originY,
                                 block.widthTiles * 8, block.heightTiles * 8);
                ovr.color = borderColors[qBound(0, block.paletteLine, 3)];
                overlays.append(ovr);
            }
            theSpriteDetail->setBorderOverlays(overlays);
        }
        else
        {
            theSpriteDetail->clearBorderOverlays();
        }
    }
    else
    {
        theSpriteDetail->clearSprite();
        theSpriteDetail->clearBorderOverlays();
    }

    // Build info
    int romOffsets = 0, ramOffsets = 0, embeddedCount = 0;
    for (const TileBlock & block : group.blocks)
    {
        if (block.source == "embedded" || block.romOffset.isEmpty())
            ++embeddedCount;
        else
        {
            bool ok = false;
            QString offStr = block.romOffset;
            if (offStr.startsWith("0x") || offStr.startsWith("0X"))
                offStr = offStr.mid(2);
            uint32_t addr = offStr.toUInt(&ok, 16);
            if (ok && addr >= 0xFF0000)
                ++ramOffsets;
            else
                ++romOffsets;
        }
    }

    QString addrInfo;
    if (ramOffsets > 0)
        addrInfo = QString("ROM: %1, RAM: %2, embedded: %3")
            .arg(romOffsets).arg(ramOffsets).arg(embeddedCount);
    else
        addrInfo = QString("ROM: %1/%2 known")
            .arg(romOffsets).arg(group.blocks.size());

    int imgW = group.boundingBox.width();
    int imgH = group.boundingBox.height();

    // Palette info
    QStringList palInfo;
    const auto & palPool = theDef->palettePool();
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
            {
                QString addrType = (pp.romOffset >= 0xFF0000) ? "RAM" : "ROM";
                palInfo.append(QString("  Line %1: %2 (%3: 0x%4)")
                    .arg(line).arg(pp.name).arg(addrType)
                    .arg(pp.romOffset, 0, 16).toUpper());
            }
            else
                palInfo.append(QString("  Line %1: %2 (embedded)")
                    .arg(line).arg(pp.name));
        }
    }

    // Collect VRAM addresses for display
    QStringList vramInfo;
    for (int i = 0; i < group.blocks.size() && i < norm.sprites.size(); ++i)
    {
        QString vram = norm.sprites[i].vramAddr;
        if (vram.isEmpty() && !group.blocks[i].vramAddr.isEmpty())
            vram = group.blocks[i].vramAddr;
        if (!vram.isEmpty())
            vramInfo.append(QString("  Spr %1: VRAM %2").arg(i).arg(vram));
    }

    QString info = QString("%1 sprites | %2x%3 pixels\n"
                           "Addresses: %4\n"
                           "Palettes: %5 used")
        .arg(norm.sprites.size())
        .arg(imgW).arg(imgH)
        .arg(addrInfo)
        .arg(palLineMap.size());
    if (!palInfo.isEmpty())
        info += "\n" + palInfo.join("\n");
    if (!vramInfo.isEmpty())
        info += "\nVRAM:\n" + vramInfo.join("\n");

    theInfoLabel->setText(info);
    theEditButton->setEnabled(true);
    theDeleteButton->setEnabled(true);
}

SpriteCollection SpriteViewerPanel::buildFromNormalized(const NormalizedCollection & norm)
{
    // Delegate to data service if available, otherwise minimal stub
    if (theDataService)
    {
        // Use data service's resolveNormalized but we need a SpriteCollection
        // For now, just build it the same way MainWindow did
    }

    // This method is kept for backward compatibility but the viewer
    // now primarily uses TileBlockGroup via RomDataService
    SpriteCollection col;
    col.name = norm.name;
    return col;
}

QImage SpriteViewerPanel::renderComposite(const NormalizedCollection & norm)
{
    if (!theDataService) return QImage();
    TileBlockGroup group = theDataService->resolveNormalized(norm);
    return theDataService->renderComposite(group);
}
