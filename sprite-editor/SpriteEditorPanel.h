#ifndef SPRITEEDITORPANEL_H
#define SPRITEEDITORPANEL_H

#include <QWidget>
#include <QSpinBox>
#include <QCheckBox>
#include <QPushButton>
#include <QLabel>
#include <QScrollArea>
#include <QMap>

#include "GenesisTypes.h"
#include "RomFile.h"
#include "GameDefinition.h"
#include "SpritePixelEditor.h"
#include "PaintToolPanel.h"
#include "PaletteWidget.h"

/**
 * Self-contained panel for the Sprite Editor tab.
 * Handles pixel-level editing of sprite groups and single sprites,
 * palette editing, save to ROM, and sprite deletion from groups.
 */
class SpriteEditorPanel : public QWidget
{
    Q_OBJECT
public:
    explicit SpriteEditorPanel(QWidget *parent = nullptr);

    void setRomFile(RomFile *rom);
    void setGameDefinition(GameDefinition *def);

    /**
     * Load a normalized collection for group editing.
     * palLineToId maps palette line (0-3) to palette pool ID.
     */
    void editNormalizedCollection(int collectionIndex,
                                  const QMap<int, QString> & palLineToId);

    /**
     * Load a legacy collection + specific sprite for single editing.
     */
    void editLegacySprite(const SpriteCollection & col,
                           int collectionIndex, int spriteIndex);

    bool hasUnsavedEdits() const;
    void closeEditor();

signals:
    void statusMessage(const QString & msg);
    void editorClosed();
    void tilesSavedToRom();
    void paletteSavedToRom();
    void spriteDeletedFromGroup(int collectionIndex);

    /** Request host to show color edit dialog. */
    void colorEditRequested(int paletteIndex, const QColor & current,
                            bool hasRomOffset, uint32_t romOffset, int refCount);

    /** Request host to confirm ROM save. */
    void saveRomRequested();

public slots:
    /** Called by host after color edit dialog completes. */
    void applyColorEdit(int paletteIndex, const QColor & newColor, uint16_t cramWord);

private slots:
    void onGroupPaletteLineChanged(int paletteLine);
    void onPaletteSelected(int index);
    void onPaletteEditRequested(int index);
    void onSave();
    void onSavePalette();
    void onClose();
    void onZoomChanged(int value);
    void onGridToggled(bool checked);
    void onToolChanged(EditorTool tool);
    void onBrushSizeChanged(int size);
    void onColorPicked(int paletteIndex);
    void onGroupSpriteSelected(int spriteIndex);
    void onDeleteSpriteFromGroup();

private:
    void buildUi();

    SpritePixelEditor  *thePixelEditor;
    PaintToolPanel     *theToolPanel;
    PaletteWidget      *thePaletteHidden;   // for CRAM data
    QLabel             *theInfoLabel;
    QSpinBox           *theZoomSpin;
    QCheckBox          *theGridCheck;
    QPushButton        *theSaveButton;
    QPushButton        *theSavePaletteButton;
    QPushButton        *theCloseButton;
    QScrollArea        *theScrollArea;

    RomFile            *theRomFile;
    GameDefinition     *theDef;

    int                 theCollectionIndex;
    int                 theSpriteIndex;
    int                 theActivePaletteLine;
    QMap<int, QString>  thePalLineToId;
    int                 theSelectedGroupSpriteIndex;
};

#endif // SPRITEEDITORPANEL_H
