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

### Moonwalker ROM Analysis (`sprite-editor/examples/moonwalker.json`)
Target ROM: `roms/Moonwalker.bin` (USA, JUE, checksum 0x38B2).
Also tested with `Moonwalker.md` (checksum 0x096F, alternate revision).
Sprite tile data is identical across revisions; palette offsets differ.

**ROM layout:**
- `0x200-0x15000`: Code + header
- `0x01B000-0x02A400`: **PRIMARY SPRITE BANK** — ~1952 tiles, density 0.37-0.65
- `0x02A800-0x052FFF`: Mixed compressed/other data
- `0x053000-0x05F400`: **SECONDARY SPRITE/TILE BANK** — ~1568 tiles, density 0.28-0.65
- `0x05F800+`: Audio samples / compressed (density 1.0)

**Confirmed sprite groups (identical across ROM revisions):**
- **0x0220E0**: 23 sprites of 2×4 tiles — MJ character animation set A ✓
- **0x023800**: 43 sprites of 2×5 tiles — MJ character animation set B ✓
- **0x0290A0**: 3 sprites of 3×5 tiles — large character sprites
- **0x054000+**: Level tile data (diagonal stripes, checkerboard floors) ✓

**Palettes (USA ROM, checksum 0x38B2):**
- **0x0082CA** — MJ gray suit (grayscale gradient + skin tones, HIGH confidence)
- **0x004416** — Stage 1 warm gradient (dark→light warm tones, MEDIUM)
- **0x002D30** — Earth tones / level environment (MEDIUM)

**Unresolved:**
- 587-tile run at 0x01D760-0x0220C0 — not cleanly divisible by common sprite sizes.
  Could be multiple sprite groups without zero-tile separators, or compressed data.

---

## Planned Work (Ordered by Priority)

### 1. ~~Implement Nemesis Decompressor~~ DONE

### 2. ~~Verify Moonwalker Sprite Offsets Visually~~ CONFIRMED

### 3. ~~Sprite Editor UX: assembly mode, export PNG, keyboard nav~~ DONE

### 3b. ~~Multi-frame sprite support~~ DONE
- Added `frame_count` field to JSON definition and `SpriteEntry` struct
- Sprite Viewer expands multi-frame entries into individual thumbnails ([1/N], [2/N], ...)
- Detail view shows per-frame ROM offset; replacement writes to correct frame offset
- Increased tile dimension bounds from 4→8 (Moonwalker uses 2×5 sprites)
- Updated moonwalker.json with frame counts: 23, 43, 73, 13, 21, 3, 13 frames

### 4. Identify and Document MJ Sprites in moonwalker.json
Using the sprite editor + BlastEm debugger, map out:
- MJ walk animation frames (in 2×4 group at 0x0220E0)
- MJ idle/standing pose
- MJ moonwalk move
- MJ hat throw attack
- Enemies (gangsters, zombies) — likely in the 587-tile run or elsewhere in main bank
- Level tiles for at least Stage 1

### 5. Sprite Replacement Test
Once sprites are identified:
1. Create a test PNG matching MJ's sprite dimensions
2. Import via "Replace Sprite" dialog
3. Verify the modified ROM plays correctly in BlastEm

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

## Known Issues / Blockers

| Issue | Severity | Notes |
|-------|----------|-------|
| 587-tile run at 0x01D760 unidentified | Medium | Try various W×H in sprite assembly mode |
| Kosinski decompressor is a stub | Low | Not needed for Moonwalker |

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
