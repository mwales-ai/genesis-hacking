# Genesis Hacking — Project Planning

## What Has Been Done

### Repository Setup
- CLAUDE.md created with project structure, build commands, and architecture notes
- `.gitignore` excludes `roms/` (Genesis ROMs not committed)
- Homebrew ROMs downloaded: Titan Overdrive 1 & 2 (`roms/`)
- BlastEm Genesis emulator installed via apt

### Documentation (`docs/`)
- `genvdp-charles-macdonald.txt` — authoritative VDP/tile/CRAM reference
- `kabuto-hardware-notes.html` — Titan demo group hardware notes
- `GenesisTechnicalOverview.pdf` — official Sega overview

### bn-genesis Plugin Improvements (committed + pushed)
- `assemble.py`: replaced hardcoded toolchain paths with `shutil.which()`, added clear
  error dialogs with `apt install` hints, try/finally temp dir cleanup
- `vdp_analysis.py`: fixed 32-bit write word order bug (big-endian M68K high word first),
  added VDP command word decoder, Mode1-4 register bit-level comments,
  replaced `print()` with `log.log_debug()`
- `loader.py`: added `GenesisRomHeader` struct at 0x100, expanded vector table to all
  64 vectors, added YM2612 segment and register labels, consistent naming prefixes
- `loader.py`: fixed modern Binary Ninja API compatibility — `self.raw.length` instead of
  `len(self.raw)`, `.read(offset, n)` instead of slice indexing for struct.unpack,
  removed broken `get_load_settings_for_data` classmethod, added `perform_get_address_size`

### Sprite Editor (`sprite-editor/`) — BUILT AND WORKING
Qt6/C++ application for viewing and replacing sprite artwork in Genesis ROMs.
Binary: `sprite-editor/build/SpriteEditor`

**Files:** `main.cpp`, `MainWindow.{h,cpp,ui}`, `RomFile.{h,cpp}`, `GameDefinition.{h,cpp}`,
`TileDecoder.{h,cpp}`, `PaletteWidget.{h,cpp}`, `TileCanvasWidget.{h,cpp}`,
`SpriteSheetWidget.{h,cpp}`, `RawTileBrowserWidget.{h,cpp}`,
`CompressionHandler.{h,cpp}`, `NoneHandler.{h,cpp}`,
`NemesisDecompressor.{h,cpp}`, `KosinskiDecompressor.{h,cpp}` (stub),
`SpriteReplaceDialog.{h,cpp,ui}`, `examples/moonwalker.json`

**Build:** `cd sprite-editor && bash build.sh` (uses Qt6/qmake6)

**Features implemented:**
- ~~Jump to offset~~ DONE
- ~~Export PNG~~ DONE
- ~~Keyboard navigation (arrow keys, Home/End)~~ DONE
- ~~Sprite assembly mode (W×H tiles)~~ DONE — assembles tiles into sprites using
  Genesis column-major order. Adjustable via spinboxes in Raw Tile Browser.
- ~~Assembly start offset~~ DONE — "Start" button sets sprite assembly origin to
  a specific ROM address so sprites align correctly.
- ~~Multi-frame sprites~~ DONE — `frame_count` in JSON expands to N thumbnails
  in Sprite Viewer; per-frame offset for detail view, replace, and export.

---

## Planned Work (Ordered by Priority)

### 1. ~~Implement Nemesis Decompressor~~ DONE

### 3. ~~Sprite Editor UX: assembly mode, export PNG, keyboard nav~~ DONE

### 3b. ~~Multi-frame sprite support~~ DONE
- Added `frame_count` field to JSON definition and `SpriteEntry` struct
- Sprite Viewer expands multi-frame entries into individual thumbnails ([1/N], [2/N], ...)
- Detail view shows per-frame ROM offset; replacement writes to correct frame offset

### 3c. ~~BlastEm Enhancements: CRAM DMA Tracking & Debugger~~ DONE
Forked BlastEm to add ROM hacking support features:
- **DMA history ring buffer** (`vdp.h`/`vdp.c`): Tracks all DMA transfers (source address,
  destination, length, frame, type). Configurable size via `--dma-history N`.
- **CRAM DMA source lookup** (`vdp_dma_lookup_cram_source()`): Traces palette data in CRAM
  back to its ROM source address via DMA history.
- **VRAM DMA source lookup** (`vdp_dma_lookup_source()`): Traces tile data in VRAM back
  to its ROM source address.
- **`dmatrace` debugger command**: Logs all DMA transfers to a file or stderr, including
  source address, destination type/address, and length. `dmatrace off` to stop.
- **Read watchpoints**: Break when emulated code reads from specific addresses.
- **Updated `-h` help text**: Documents all command-line switches including `-b`, `-D`,
  `-t`, `--dma-log`, and `--dma-history`.

### 3d. ~~Full Screen Graphic Capture~~ DONE
BlastEm `screencap` debugger command + sprite editor Screen Captures tab:
- **BlastEm side** (`vdp.c`, `debug.c`):
  - VDP helpers: `vdp_get_plane_a_base()`, `vdp_get_plane_b_base()`,
    `vdp_get_nametable_stride()`, `vdp_get_visible_dimensions()`
  - Brute-force ROM tile search: `vdp_find_tile_in_rom()` — finds tiles in ROM when
    DMA history doesn't cover them
  - `screencap [FILE] [a|b]` command: Exports full VDP plane state to JSON with tile map
    entries (pattern, palette, flip, priority, ROM offset), 4 CRAM palette lines with
    DMA sources, and embedded tile data for VRAM-only tiles
  - Source resolution chain: DMA history → brute-force ROM search → embedded data
- **Sprite editor side** (`GameDefinition.h/cpp`, `TileDecoder.h/cpp`, `TileMapWidget.h/cpp`,
  `MainWindow.h/cpp`):
  - New data structures: `ScreenCapture`, `TileMapEntry`, `ScreenCapturePalette`
  - JSON parsing for `screen_captures` array in game definitions
  - `decodeTileFlipped()` — decode tiles with h/v flip support
  - `decodePaletteFromCram()` — build palette from raw CRAM values
  - `TileMapWidget` — renders full tile maps with zoom (1-8x) and hover tooltips
  - New "Screen Captures" tab in MainWindow with capture selector and zoom control
- **JSON format**: Designed for forward compatibility with sprite collections and
  animations (future `sprite_collections` and `animations` arrays)

### 6. Implement Kosinski Decompressor (Lower Priority)
**File:** `sprite-editor/KosinskiDecompressor.cpp`
**Why:** Sonic games use Kosinski; less relevant for Moonwalker but needed for portability.
**Effort:** Medium — LZ-based, ~200 lines C++

### 7. bn-genesis Plugin: Implement Nemesis Analysis
Add a Binary Ninja plugin command that:
- Detects Nemesis-compressed blocks in ROM
- Annotates them in the disassembly view
- Shows decompressed size in comments
- Works alongside the existing VDP DMA analysis

### 8. Sprite Editor: Remaining UX
- Add tile grid overlay toggle to PaletteWidget
- Improve SpriteReplaceDialog preview and error handling

---

## Architecture Notes

### Sprite Editor Key Paths
- **Open ROM** → `RomFile::openRom()` → validates Genesis header ("SEGA" @ 0x100)
- **Open def file** → `GameDefinition::loadFromFile()` → populates group combo box
- **View sprite** → `CompressionHandler::decompress()` → `TileDecoder::decodeSprite()` → displayed in `TileCanvasWidget`
- **Replace sprite** → `SpriteReplaceDialog` → `TileDecoder::encodeSprite()` → `RomFile::writeBytes()`
- **Raw Tile Browser** → decodes tiles at ROM offset, optionally assembled into W×H sprites
  with adjustable assembly start offset. Column-major Genesis tile order.

### Genesis Tile Format (quick reference)
- 8×8 pixels, 4bpp, 32 bytes per tile
- Row = 4 bytes: high nibble = left pixel, low nibble = right pixel
- Pixel index 0 = transparent
- Sprite tile order: COLUMN-MAJOR (col * height_tiles + row)
- CRAM color: `---- bbb- ggg- rrr-` (3 bits each, × 36 → 0-252)

### Nemesis Quick Reference
Used by: Moonwalker, Sonic 1, other early Sega titles
Header: 2 bytes — tile count (bits 15-1) and XOR flag (bit 0)
XOR mode: each output nibble is XORed with the previous nibble
Output: raw 4bpp tile data

---

## User Instructions / Preferences
- Commit locally often; get approval before pushing
- Background agents approved for research and builds
- ROM files stay in `roms/` (gitignored)
- Build command: `cd sprite-editor && bash build.sh`
