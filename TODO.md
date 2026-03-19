Tasks for Claude:

# Incomplete Tasks

* [ ] Sprite Editor Refactoring
    * [ ] We have too much code in a single super class MainWindow.  I want to break this up into smaller classes / QWidgets to allow us to use them in other applications.  So we want to make sure the functions are easy to interface with for for each of them, minimal complexity, minimal cross-dependencies.
    * [X] Raw Tile Browser → RawTileBrowserPanel (Phase 2 complete)
        * [X] Self-contained QWidget with all controls + canvas
        * [X] jumpToAddress() slot for cross-tab navigation
        * [X] populateRanges()/populatePalettes()/refresh() slots
        * [X] tileSelected signal for click events
        * [X] exportPngRequested signal (host provides file dialog)
        * [ ] Right click context menu (Export PNG, Copy hex) — future
    * [X] Sprite Editing Tools → PaintToolPanel (Phase 0 complete)
        * [X] 2x2 icon tool buttons + 4x4 palette grid
        * [X] toolChanged, brushSizeChanged, colorSelected signals
        * [X] deleteRequested signal for sprite removal
    * [X] Sprite Editor → SpriteEditorPanel (Phase 3 complete)
        * [X] TileBlockGroup shared structure (GenesisTypes.h)
        * [X] editNormalizedCollection() / editLegacySprite() entry points
        * [X] Save tiles/palette signals, colorEditRequested signal
        * [X] Zoom, grid, tool panel all self-contained
    * [ ] Sprite Animations → SpriteAnimationPanel (Phase 4)
        * [ ] Extract populateSpriteCollections, recording loading, frame navigation
        * [ ] Extract capture workflow (capture group, hide/unhide)
        * [ ] Wire sprite click to open SpriteEditorPanel
    * [ ] Sprite Viewer → SpriteViewerPanel (Phase 5)
        * [ ] Extract collection grid, detail view, reorder, rename, delete
        * [ ] Wire edit button to SpriteEditorPanel
        * [ ] Wire double-click to RawTileBrowserPanel
    * [ ] Final cleanup (Phase 6)
        * [ ] Remove all #if 0 dead code blocks from MainWindow.cpp
        * [ ] Remove unused tabs from MainWindow.ui
        * [ ] MainWindow should be under 400 lines
    * [X] When I click on a spirte and go to the Raw Viewer, there are often sprites that are addressed out of range (but the range should go to the end?)
    

* [X] BlastEm updates
    * [X] If I don't specify a file extenstion, put a default extension of .sprec on sprite recordings
    * [X] Do a code review of the sprite recording logic, particularly if a sprite isn't DMA-ed from ROM, have we correctly copied the sprite and palette data from VRAM into the sprec file

* [X] Sprite Editor
    * [X] Sprite Viewer
        * [X] Sprites captured in RAM indicate they are from ROM but at the RAM address, change the text to say ROM as well
    * [X] Sprite Editor tab
        * [X] Remove the palette at the bottom of the screen since we have one on the right side
        * [X] Want middle click to select a sprite in the sprite group (would be important if sprites in the group had different palettes)
        * [X] When a sprite from a group is middle clicked, display sprite info (ROM address, dimensions, palette addr)
        * [X] Provide a button to delete the selected sprite from the sprite group (like accidentally capturing sprite from a neighboring character)

* [ ]  BN-Genesis Plugin

    * [X] Load a game definition file when a ROM is loaded and label all sprites at their ROM addresses
    * [X] Give each sprite a proper structure at its memory location (you will need to define a structure for sprite data and make sure it is loaded into the bndb file)
    * [X] Label each palette in the ROM with an appropriate structure
    * [ ] Show a sprite in a custom Binja pane with palette selection (future task - don't do this one yet)
    * [X] Write a small document about how we can get code flow info from blastem into binary ninja

# Completed Tasks

