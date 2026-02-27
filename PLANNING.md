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

### Sprite Editor (`sprite-editor/`) — BUILT AND WORKING
Qt6/C++ application for viewing and replacing sprite artwork in Genesis ROMs.
Binary: `sprite-editor/build/SpriteEditor`

**Files:** `main.cpp`, `MainWindow.{h,cpp,ui}`, `RomFile.{h,cpp}`, `GameDefinition.{h,cpp}`,
`TileDecoder.{h,cpp}`, `PaletteWidget.{h,cpp}`, `TileCanvasWidget.{h,cpp}`,
`SpriteSheetWidget.{h,cpp}`, `RawTileBrowserWidget.{h,cpp}`,
`CompressionHandler.{h,cpp}`, `NoneHandler.{h,cpp}`,
`NemesisDecompressor.{h,cpp}` (stub), `KosinskiDecompressor.{h,cpp}` (stub),
`SpriteReplaceDialog.{h,cpp,ui}`, `examples/moonwalker.json`

**Build:** `cd sprite-editor && bash build.sh` (uses Qt6/qmake6)

### Moonwalker ROM Analysis (`sprite-editor/examples/moonwalker.json`)
Performed density-based tile analysis of `roms/Moonwalker.md` (512 KB US version).
Sprite-like data = density 0.1-0.65 (lots of transparent index-0 pixels).
High-density 0.7-1.0 = compressed data, audio, or code.

**ROM layout:**
- `0x200-0x15000`: Code + header (moderate-high density, compression/data)
- `0x01B000-0x02A400`: **PRIMARY SPRITE BANK** — ~1952 tiles, density 0.37-0.65. HIGH confidence sprite data.
- `0x02A800-0x052FFF`: Mixed compressed/other data (high density interspersed)
- `0x053000-0x05F400`: **SECONDARY SPRITE/TILE BANK** — ~1568 tiles, density 0.28-0.65
- `0x05F800+`: Audio samples / compressed (density 1.0)

**Palettes:**
- **0x0082DA** — MJ gray suit (grayscale gradient + skin tones, HIGH confidence)
- **0x00C770** — Fire/explosion animation (rotating 4-palette block, HIGH confidence)
- **0x002D3C** — Earth tones / level environment (MEDIUM confidence)
- **0x0033E4** — Vivid colors, possibly cutscene/UI (MEDIUM confidence)

---

## Current Work In Progress

### Nemesis Decompressor (`sprite-editor/NemesisDecompressor.cpp`)
**Status:** IMPLEMENTED. Full decompressor including XOR mode, inline literals, and
code table parsing. Tested to build cleanly.

**Moonwalker note:** Further ROM analysis shows Moonwalker appears to use UNCOMPRESSED
tile data stored directly in ROM (ROM-to-VRAM DMA without decompression). The code area
extends from 0x200 through approximately 0x15000. Tile data found at 0x02D6CC,
0x032D8A, and 0x05A000-0x05E800. The Nemesis decompressor will be useful for
Sonic and other Sega titles that do use Nemesis compression.

---

## Planned Work (Ordered by Priority)

### 1. ~~Implement Nemesis Decompressor~~ DONE

### 2. ~~Verify Moonwalker Sprite Offsets Visually~~ CONFIRMED via Python analysis

Zero-tile boundary analysis + visual pixel inspection confirmed:
- **0x0220E0**: 23 sprites of 2×4 tiles — dark jacket + gold skin (MJ palette) ✓
- **0x023800**: 43 sprites of 2×5 tiles — humanoid figures in motion ✓
- **0x0290A0**: 3 sprites of 3×5 tiles
- **0x054000+**: Level tile data (diagonal stripes, checkerboard floor patterns) ✓

Large run 0x01D760-0x0220C0 (587 tiles) still unresolved — could be more sprites
(not cleanly divisible by common sprite sizes) or Nemesis-compressed data.

**Next step:** In sprite editor, open 0x0220E0 and 0x023800 as sprite entries (already
in moonwalker.json) to view them with proper transparency and correct palette.

### 3. Implement Kosinski Decompressor (Lower Priority)
**File:** `sprite-editor/KosinskiDecompressor.cpp`
**Why:** Sonic games use Kosinski; less relevant for Moonwalker but needed for portability.
**Effort:** Medium — LZ-based, ~200 lines C++

### 4. Sprite Editor UX Improvements
Things to improve once the core functionality is verified:
- ~~Add a "jump to offset" field in Raw Tile Browser~~ DONE
- Add "Export PNG" button to dump the current sprite as a PNG
- Add tile grid overlay toggle to PaletteWidget
- Keyboard navigation in SpriteSheetWidget (arrow keys)

### 5. Identify and Document MJ Sprites in moonwalker.json
Using the sprite editor + BlastEm debugger, map out:
- MJ walk animation frames (likely 2×4 tiles each, 6-8 frames)
- MJ idle/standing pose
- MJ moonwalk move
- MJ hat throw attack
- Enemies (gangsters, zombies)
- Level tiles for at least Stage 1

### 6. Sprite Replacement Test
Once sprites are identified and Nemesis is implemented:
1. Create a test PNG matching MJ's sprite dimensions
2. Import via "Replace Sprite" dialog
3. Verify the modified ROM plays correctly in BlastEm

### 7. bn-genesis Plugin: Implement Nemesis Analysis
Add a Binary Ninja plugin command that:
- Detects Nemesis-compressed blocks in ROM
- Annotates them in the disassembly view
- Shows decompressed size in comments
- Works alongside the existing VDP DMA analysis

---

## Known Issues / Blockers

| Issue | Severity | Notes |
|-------|----------|-------|
| moonwalker.json palette/sprite offsets unverified | Medium | Need visual confirmation via app |
| Kosinski decompressor is a stub | Low | Not needed for Moonwalker |

---

## Architecture Notes

### Sprite Editor Key Paths
- **Open ROM** → `RomFile::openRom()` → validates Genesis header ("SEGA" @ 0x100)
- **Open def file** → `GameDefinition::loadFromFile()` → populates group combo box
- **View sprite** → `CompressionHandler::decompress()` → `TileDecoder::decodeSprite()` → displayed in `TileCanvasWidget`
- **Replace sprite** → `SpriteReplaceDialog` → `TileDecoder::encodeSprite()` → `RomFile::writeBytes()`
- **Raw Tile Browser** → decodes every 32-byte chunk at ROM offset as a tile, displayed as grid

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
