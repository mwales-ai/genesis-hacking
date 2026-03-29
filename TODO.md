Tasks for Claude:

# Incomplete Tasks

* [ ] VRAM/CRAM address tracking for reverse engineering
    * [X] BlastEm sprite recorder: ensure VRAM address and CRAM address (palette_line * 32) are written to the .sprec JSON for every sprite
    * [X] BlastEm sprite recorder: for each palette line, record the CRAM byte address (line * 32) and any DMA source that wrote to that CRAM range
    * [X] BlastEm: record RAM-sourced DMA sprites separately (source="ram" with dma_source address)
    * [X] Sprite Editor: when middle-clicking a sprite in the editor, show VRAM address alongside ROM/RAM address and dimensions
    * [X] Sprite Editor: in the Sprite Viewer info panel, show VRAM addresses for each sprite in the group
    * [ ] Sprite Editor: in the Sprite Animations tab, show VRAM and CRAM addresses when hovering or selecting sprites
    * [X] Game definition JSON: preserve VRAM address when capturing sprite groups from recordings (NormalizedSprite now has vramAddr field)
    * [ ] Consider adding a "Copy VRAM address" context menu action so user can quickly paste into Binary Ninja search

* [ ] Sprite Editor Refactoring (remaining items)
    * [ ] Right click context menu on Raw Tile Browser (Export PNG, Copy hex) — future
    * [ ] Wire sprite click in Animations tab to open SpriteEditorPanel (pending full integration)

* [ ] BN-Genesis Plugin
    * [ ] Show a sprite in a custom Binja pane with palette selection (future task - don't do this one yet)

* [ ] Documentation updates
    * [X] Update USAGE.md to document the codetrace workflow (BlastEm codetrace → BN import)
    * [X] Update USAGE.md to reflect the refactored panel architecture (fixed stale palette strip reference)
    * [X] Add a section to USAGE.md about the VRAM/CRAM address display features
    * [X] Update bn-genesis README with codetrace import documentation
    * [ ] Review all screenshots in docs/ — some may be outdated after the refactoring (tool panel layout changed, palette strip removed, etc.)

# Completed Tasks

* [X] Sprite Editor Refactoring (Phases 0-6)
    * [X] GenesisTypes.h, RomDataService, PaintToolPanel (Phase 0)
    * [X] ScreenCapturePanel (Phase 1)
    * [X] RawTileBrowserPanel (Phase 2) — with Go-highlight and range fix
    * [X] SpriteEditorPanel (Phase 3)
    * [X] SpriteAnimationPanel (Phase 4)
    * [X] SpriteViewerPanel (Phase 5)
    * [X] Final cleanup — MainWindow 683 lines (Phase 6)
* [X] BlastEm updates
    * [X] Default .sprec extension
    * [X] Sprite recording code review + stability fixes
    * [X] Code trace recording (codetrace/codetracestop commands)
* [X] Sprite Editor features
    * [X] 2x2 icon tool buttons + 4x4 palette grid
    * [X] Screen capture editing with shared tile highlighting
    * [X] Drag-to-reorder, rename, delete sprite groups
    * [X] Border overlays (1px regardless of zoom)
    * [X] ROM overwrite protection
    * [X] Middle-click sprite info + delete from group
    * [X] RAM vs ROM address distinction
* [X] BN-Genesis Plugin
    * [X] Game definition loader (struct types + labels)
    * [X] C++ sprite viewer sidebar widget
    * [X] Code trace import command
    * [X] Build/install documentation
