#ifndef ROMDATASERVICE_H
#define ROMDATASERVICE_H

#include "GenesisTypes.h"
#include "RomFile.h"
#include "GameDefinition.h"
#include "CompressionHandler.h"

#define DataSvcDebug if(0) std::cout

/**
 * Bridge between GameDefinition (serialization) and widgets (rendering).
 * Resolves normalized/legacy collections and recordings into TileBlockGroups
 * that widgets can render directly.  No UI dependency — pure data service.
 */
class RomDataService
{
public:
    RomDataService();

    void setRom(RomFile *rom);
    void setDefinition(GameDefinition *def);
    void setCompressor(CompressionHandler *comp);

    RomFile *rom() const { return theRom; }
    GameDefinition *definition() const { return theDef; }

    /** Resolve a normalized collection to a renderable group. */
    TileBlockGroup resolveNormalized(const NormalizedCollection & norm);

    /** Resolve a legacy SpriteCollection to a renderable group. */
    TileBlockGroup resolveLegacy(const SpriteCollection & col);

    /** Resolve a recording frame to a renderable group. */
    TileBlockGroup resolveRecordingFrame(const SpriteRecording & rec, int frameIndex);

    /** Render a composite QImage from a TileBlockGroup. */
    QImage renderComposite(const TileBlockGroup & group);

    /** Resolve a single palette by pool ID. */
    GenesisPalette resolvePalette(const QString & paletteId);

    /** Read raw tile bytes from ROM. */
    QByteArray readTileRange(uint32_t startOffset, uint32_t length);

    /** Resolve tile data for a sprite entry (legacy format). */
    QByteArray fetchTileData(const SpriteEntry & entry);

    /** Resolve tile data for a pool pattern. */
    QByteArray fetchPatternTileData(const PoolPattern & pat);

    /** Build a list of PaletteInfo from the current definition. */
    QVector<PaletteInfo> availablePalettes();

private:
    RomFile            *theRom;
    GameDefinition     *theDef;
    CompressionHandler *theCompressor;
};

#endif // ROMDATASERVICE_H
