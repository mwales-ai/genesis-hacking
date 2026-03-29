#ifndef SPRITEANIMATIONPANEL_H
#define SPRITEANIMATIONPANEL_H

#include <QWidget>
#include <QComboBox>
#include <QSpinBox>
#include <QPushButton>
#include <QLabel>
#include <QScrollArea>
#include <QSet>

#include "GenesisTypes.h"
#include "RomDataService.h"
#include "GameDefinition.h"
#include "RomFile.h"
#include "SpriteCollectionWidget.h"

/**
 * Self-contained panel for the Sprite Animations tab.
 * Handles sprite recording loading, frame navigation,
 * capture workflow, and hide/unhide.
 */
class SpriteAnimationPanel : public QWidget
{
    Q_OBJECT
public:
    explicit SpriteAnimationPanel(QWidget *parent = nullptr);

    void setDataService(RomDataService *service);
    void setRomFile(RomFile *rom);
    void setGameDefinition(GameDefinition *def);

    void addRecording(const SpriteRecording & rec);
    void populateCollections();

signals:
    void statusMessage(const QString & msg);
    void editSpriteRequested(int collectionIndex, const QMap<int, QString> & palLineToId);
    void editRecordingSpriteRequested(const SpriteCollection & col, int spriteIndex);
    void collectionsCaptured();

    /** Request host to show file dialog for recording. */
    void loadRecordingRequested();

private slots:
    void onCollectionSelected(int index);
    void onZoomChanged(int value);
    void onFrameChanged(int frameIndex);
    void onLoadRecordingClicked();
    void onSpriteClicked(int spriteIndex);
    void onSelectionChanged(const QSet<int> & selected);
    void onCaptureGroup();
    void onHideSelected();
    void onUnhideAll();
    void onUnhideSelectedOnly();

private:
    void buildUi();
    void displayRecordingFrame(int recIndex, int frameIndex);
    void applyPersistentHidden();

    QComboBox              *theCollectionCombo;
    QSpinBox               *theZoomSpin;
    QSpinBox               *theFrameSpin;
    QLabel                 *theFrameLabel;
    QLabel                 *theFrameCountLabel;
    QPushButton            *theLoadRecordingButton;
    QLabel                 *theSelectionLabel;
    QPushButton            *theCaptureButton;
    QPushButton            *theHideButton;
    QPushButton            *theUnhideAllButton;
    QPushButton            *theUnhideSelectedButton;
    QScrollArea            *theScrollArea;
    SpriteCollectionWidget *theSpriteColWidget;

    RomDataService         *theDataService;
    RomFile                *theRomFile;
    GameDefinition         *theDef;

    QVector<SpriteRecording> theRecordings;
    int                    theActiveRecIndex;
    QSet<QString>          theHiddenRomOffsets;
    int                    theCaptureCounter;
};

#endif // SPRITEANIMATIONPANEL_H
