#ifndef SPRITECOLLECTIONWIDGET_H
#define SPRITECOLLECTIONWIDGET_H

#include <QWidget>
#include <QImage>
#include <QSet>
#include "GameDefinition.h"
#include "TileDecoder.h"
#include "RomFile.h"

/**
 * Renders a composite sprite from multiple Genesis hardware sprites.
 * Each sprite is placed at its correct screen position, composited
 * with transparency into a single image.
 * Supports multi-select via Ctrl+click and rubber-band selection.
 */
class SpriteCollectionWidget : public QWidget
{
    Q_OBJECT
public:
    explicit SpriteCollectionWidget(QWidget *parent = nullptr);

    /** Load a sprite collection for display. romFile may be null if all tiles are embedded. */
    void setCollection(const SpriteCollection & collection, RomFile *romFile);

    void clearCollection();

    void setZoom(int factor);
    int zoom() const;

    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;

    /** Returns the currently loaded collection (for editor access) */
    const SpriteCollection & collection() const { return theCollection; }

    /** Returns the decoded palette for a given CRAM line (0-3) */
    const GenesisPalette & decodedPalette(int line) const { return thePalettes[qBound(0, line, 3)]; }

    void clearSelection();

    /** Multi-select accessors */
    const QSet<int> & selectedSpriteIndices() const { return theSelectedSpriteIndices; }

    /** Set which sprites are hidden (not rendered, shown as dashed outline on hover) */
    void setHiddenSprites(const QSet<int> & indices);
    void clearHiddenSprites();

signals:
    void spriteHovered(int spriteIndex, int x, int y);
    void spriteClicked(int spriteIndex);
    void selectionChanged(const QSet<int> & selectedIndices);

protected:
    void paintEvent(QPaintEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;

private:
    void rebuildImage();
    QByteArray tileDataForSprite(const CollectionSprite & sprite) const;
    int hitTestSprite(const QPoint & widgetPos) const;

    SpriteCollection     theCollection;
    GenesisPalette       thePalettes[4];  // decoded palette for each CRAM line
    RomFile             *theRomFile;
    QImage               theCompositeImage;  // pre-rendered composite
    int                  theZoom;
    bool                 theHasCollection;
    int                  theHoveredSpriteIndex;

    // Multi-select state
    QSet<int>            theSelectedSpriteIndices;
    QPoint               theRubberBandOrigin;
    QRect                theRubberBandRect;
    bool                 theRubberBanding;

    // Hidden sprites (not rendered, dashed outline on hover/select)
    QSet<int>            theHiddenSpriteIndices;
};

#endif // SPRITECOLLECTIONWIDGET_H
