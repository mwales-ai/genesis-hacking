# Sprite Editor Refactoring Plan

## Goal
Break the monolithic MainWindow (~3500 lines) into reusable QWidget panels
that can work in the standalone Sprite Editor AND in Binary Ninja as dock widgets.

## New Architecture

```
MainWindow (thin shell ~300 lines)
  ├── RomFile, GameDefinition, CompressionHandler (owned data)
  ├── RomDataService (bridge: definition -> resolved TileBlockGroup)
  ├── QTabWidget
  │   ├── SpriteViewerPanel    (grid + detail + reorder + delete)
  │   ├── RawTileBrowserPanel  (range/palette/zoom/jump + browser canvas)
  │   ├── ScreenCapturePanel   (load/edit/save screen captures)
  │   ├── SpriteAnimationPanel (recordings + capture workflow)
  │   └── SpriteEditorPanel    (pixel editor + tool panel + save)
  └── Signal/slot wiring between panels
```

## Shared Data Types (GenesisTypes.h)

- **TileBlock** — one resolved sprite piece (tile bytes + geometry + position + palette line)
- **TileBlockGroup** — multiple TileBlocks composited with 4 decoded palettes
- **PaletteInfo** — palette metadata for display

These are the widget-facing types. Widgets never see NormalizedCollection or SpriteCollection.

## Implementation Phases

### Phase 0: Foundation (no visible change)
1. GenesisTypes.h — shared value types
2. RomDataService — extract data resolution from MainWindow
3. PaintToolPanel — extract duplicated 2x2 tool + 4x4 palette panel
4. Build, verify everything still works

### Phase 1: ScreenCapturePanel (most self-contained)
### Phase 2: RawTileBrowserPanel
### Phase 3: SpriteEditorPanel
### Phase 4: SpriteAnimationPanel
### Phase 5: SpriteViewerPanel
### Phase 6: Simplify MainWindow.ui to minimal shell
### Phase 7: Binary Ninja portability check

## Key Design Rules
- Panels emit signals, never call QFileDialog/QMessageBox directly
- MainWindow wires cross-panel communication via signals/slots
- RomDataService owns all definition->TileBlockGroup conversion
- PaintToolPanel is shared between ScreenCapturePanel and SpriteEditorPanel
