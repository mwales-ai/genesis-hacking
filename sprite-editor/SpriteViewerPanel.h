#ifndef SPRITEVIEWERPANEL_H
#define SPRITEVIEWERPANEL_H

#include <QWidget>
#include <QSplitter>
#include <QScrollArea>
#include <QSpinBox>
#include <QCheckBox>
#include <QPushButton>
#include <QLineEdit>
#include <QLabel>

#include "GenesisTypes.h"
#include "RomDataService.h"
#include "GameDefinition.h"
#include "SpriteSheetWidget.h"
#include "TileCanvasWidget.h"

/**
 * Self-contained panel for the Sprite Viewer tab.
 * Shows collection thumbnails in a grid on the left, detail composite
 * on the right, with zoom, borders, rename, reorder, delete, and
 * cross-tab navigation.
 */
class SpriteViewerPanel : public QWidget
{
    Q_OBJECT
public:
    explicit SpriteViewerPanel(QWidget *parent = nullptr);

    void setDataService(RomDataService *service);
    void setGameDefinition(GameDefinition *def);

    /** Refresh the collection grid from the game definition. */
    void populateGrid();

    int selectedCollectionIndex() const { return theSelectedIndex; }

signals:
    void statusMessage(const QString & msg);

    /** User wants to edit the selected collection. */
    void editRequested(int collectionIndex, const QMap<int, QString> & palLineToId);

    /** User double-clicked detail to jump to raw browser. */
    void jumpToRawBrowser(uint32_t romOffset, int widthTiles, int heightTiles,
                          int paletteComboIndex);

    /** A collection was deleted — other panels may need to refresh. */
    void collectionDeleted();

private slots:
    void onGridSelected(int groupIndex, int spriteIndex, int frameIndex);
    void onZoomChanged(int value);
    void onThumbZoomChanged(int value);
    void onReordered(int fromIndex, int toIndex);
    void onNameEditFinished();
    void onBordersToggled(bool checked);
    void onEditClicked();
    void onDeleteClicked();
    void onDoubleClicked(int groupIndex, int spriteIndex, int frameIndex);
    void onDetailDoubleClicked(int spriteX, int spriteY);

private:
    void buildUi();
    void updateDetail(int collectionIndex);
    QImage renderComposite(const NormalizedCollection & norm);
    SpriteCollection buildFromNormalized(const NormalizedCollection & norm);

    SpriteSheetWidget  *theSpriteSheet;
    TileCanvasWidget   *theSpriteDetail;
    QLineEdit          *theNameEdit;
    QSpinBox           *theZoomSpin;
    QSpinBox           *theThumbZoomSpin;
    QCheckBox          *theBordersCheck;
    QPushButton        *theEditButton;
    QPushButton        *theDeleteButton;
    QLabel             *theInfoLabel;
    QSplitter          *theSplitter;

    RomDataService     *theDataService;
    GameDefinition     *theDef;

    int                 theSelectedIndex;
    bool                theShowBorders;
};

#endif // SPRITEVIEWERPANEL_H
