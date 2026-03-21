# Sprite Editor Refactoring — COMPLETED

## Result

The monolithic MainWindow (~3500 lines) has been broken into reusable QWidget panels.
MainWindow is now 683 lines (88-line header) — a thin shell that owns data and wires signals.

## Final Architecture

```
MainWindow (683 lines — thin shell)
  ├── RomFile, GameDefinition, CompressionHandler (owned data)
  ├── RomDataService (bridge: definition -> resolved TileBlockGroup)
  ├── QTabWidget
  │   ├── SpriteViewerPanel    (530 lines — grid + detail + reorder + delete)
  │   ├── RawTileBrowserPanel  (483 lines — range/palette/zoom/jump + browser)
  │   ├── ScreenCapturePanel   (416 lines — load/edit/save screen captures)
  │   ├── SpriteAnimationPanel (534 lines — recordings + capture workflow)
  │   └── SpriteEditorPanel    (543 lines — pixel editor + tool panel + save)
  └── Signal/slot wiring between panels

Shared Components:
  ├── GenesisTypes.h     (55 lines — TileBlock, TileBlockGroup, PaletteInfo)
  ├── RomDataService     (347 lines — definition -> TileBlockGroup resolution)
  └── PaintToolPanel     (184 lines — 2x2 icon tools + 4x4 palette grid)
```

## Shared Data Types (GenesisTypes.h)

- **TileBlock** — one resolved sprite piece (tile bytes + geometry + position + palette line)
- **TileBlockGroup** — multiple TileBlocks composited with 4 decoded palettes
- **PaletteInfo** — palette metadata for display

Widgets receive TileBlockGroups — they never see NormalizedCollection or SpriteCollection directly.

## Key Design Rules
- Panels emit signals, never call QFileDialog/QMessageBox directly
- MainWindow wires cross-panel communication via signals/slots
- RomDataService owns all definition->TileBlockGroup conversion
- PaintToolPanel is shared between ScreenCapturePanel and SpriteEditorPanel

## Binary Ninja Portability
- All panel widgets depend only on Qt6 and the shared types
- No dependency on MainWindow, QMainWindow, QSettings, or file dialogs
- C++ sprite viewer sidebar widget created in bn-genesis (cpp_ui/ directory)
- Same tile decoding code can be shared between projects

## Implementation History

| Phase | Panel | Lines | Date |
|-------|-------|-------|------|
| 0 | Foundation (GenesisTypes, RomDataService, PaintToolPanel) | 586 | 2026-03-18 |
| 1 | ScreenCapturePanel | 416 | 2026-03-18 |
| 2 | RawTileBrowserPanel | 483 | 2026-03-18 |
| 3 | SpriteEditorPanel | 543 | 2026-03-18 |
| 4 | SpriteAnimationPanel | 534 | 2026-03-19 |
| 5 | SpriteViewerPanel | 530 | 2026-03-19 |
| 6 | Dead code cleanup | -2700 | 2026-03-19 |
