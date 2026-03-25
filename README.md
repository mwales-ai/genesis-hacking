# genesis-hacking

AI-assisted tools for Sega Genesis / Megadrive ROM hacking. The main deliverables are a sprite editor for viewing and replacing artwork, and a customized BlastEm emulator with enhanced debugging for reverse engineering.

## Tools

### Sprite Editor (`sprite-editor/`)

A Qt6/C++ application for viewing, editing, and replacing sprite artwork in Genesis ROMs. Sprite locations are described by a JSON game definition file, making the tool usable with any game.

![Sprite Editor Screenshot](sprite-editor/docs/screenshots/01_sprite_viewer_overview.png)

**Features:**
- **Sprite Viewer** — browse sprite groups with drag-to-reorder, independent thumbnail zoom, palette-colored border overlays, double-click to jump to Raw Tile Browser
- **Raw Tile Browser** — explore raw tile data anywhere in ROM, assemble tiles into sprites, jump to offset
- **Screen Captures** — display full-screen tile maps captured from BlastEm, with hover tile info, status bar showing pattern/palette recovery stats, and pixel-level tile editing with shared tile highlighting
- **Sprite Editor** — pixel-level painting with pencil, bucket fill, eyedropper, and adjustable brush tools in a 2x2 icon grid, with 4x4 palette selector and 1x16 palette strip
- **Sprite Animations** — load and browse `.sprec` sprite recording files captured from BlastEm, capture sprite groups from animation frames
- Multi-palette support, Nemesis/Kosinski decompression, PNG export, original ROM overwrite protection

**Build:** Requires `qt6-base-dev`. Run `cd sprite-editor && bash build.sh`.

**Usage:**
```
./build/SpriteEditor [rom.bin] [definition.json] [recording.sprec ...]
```

**Example (Aladdin):**
```
./build/SpriteEditor roms/Aladdin_Beta.bin sprite-editor/examples/aladdin_sprite_def.json sprite-editor/examples/ali_animations.sprec
```

See [sprite-editor/docs/USAGE.md](sprite-editor/docs/USAGE.md) for the full user guide.

### BlastEm (forked, at `../blastem/`)

Forked BlastEm Genesis emulator with ROM hacking enhancements:

- **DMA history tracking** — ring buffer records all VDP DMA transfers with source/destination addresses, enabling tracing of tile and palette data back to ROM
- **`dmatrace` command** — log all DMA transfers to a file for analysis
- **`screencap` command** — capture the full VDP tile map to JSON for the sprite editor, with ROM offset resolution via DMA history and brute-force ROM search
- **Read watchpoints** — break when emulated code reads from specific addresses
- **`--dma-history N`** — configure DMA history buffer size

Enter the debugger during emulation by pressing backtick. Run `help` to see all commands.

### bn-genesis (`../bn-genesis/`)

Binary Ninja plugin for Genesis ROM analysis. Provides two components:

**Python plugin** (master branch) — ROM loader, VDP analysis, and game definition support:
- Parses ROM headers, maps memory segments (ROM, RAM, Z80, VDP, I/O)
- `genesis: load game definition` — labels all sprites and palettes at their ROM addresses with proper struct types
- `genesis: comment VDP inst` — annotates VDP register writes in disassembly
- `genesis: assemble and patch` — compile M68K assembly and patch into ROM
- `genesis: fixup ROM checksum` — recalculate and write ROM checksum

**C++ sprite viewer** (sprite-viewer-cpp branch) — native sidebar widget:
- Visual tile rendering at cursor address using Genesis 4bpp format
- Configurable sprite grid (W x H tiles) with column-major ordering
- Palette loading from game definition JSON or directly from ROM
- Adjustable zoom (1-16x)

#### Installing the Python Plugin

```bash
# Symlink the plugin directory into Binary Ninja's plugin folder
ln -s /path/to/bn-genesis ~/.binaryninja/plugins/genesis

# Dependencies
sudo apt install gcc-m68k-linux-gnu    # for assemble and patch
```

The plugin requires the `binaryninja-m68k` processor module for M68K disassembly.

#### Building the C++ Sprite Viewer

The C++ sidebar widget requires the Binary Ninja API headers and Qt6.

```bash
# Clone the Binary Ninja API (if not already done)
git clone https://github.com/Vector35/binaryninja-api.git bn-api
cd bn-api && git submodule update --init vendor/fmt && cd ..

# Checkout the C++ branch
cd bn-genesis
git checkout sprite-viewer-cpp

# Build
cd cpp_ui
mkdir -p build && cd build
cmake .. \
    -DBN_API_PATH=/path/to/bn-api \
    -DBN_INSTALL_DIR=/path/to/binaryninja
make

# Install — copy the built library to Binary Ninja's plugin directory
cp libgenesis_sprite_viewer.so ~/.binaryninja/plugins/
```

**Build requirements:**
- Qt6 development libraries (`qt6-base-dev`)
- CMake 3.13+
- Binary Ninja API headers (cloned from GitHub)
- Binary Ninja installation (for linking against `libbinaryninjacore.so` and `libbinaryninjaui.so`)

After installation, the "Sprite Viewer" sidebar appears in Binary Ninja when viewing a Genesis ROM.

See the [bn-genesis README](https://github.com/mwales-ai/bn-genesis) for full documentation.

## ROM Analysis

### Aladdin (`sprite-editor/examples/aladdin_sprite_def.json`)

Target ROM: Aladdin Beta (Aladdin_Beta.bin).

- Game definition file with sprite groups, palettes, and pattern pools
- Animation recording (`ali_animations.sprec`) — captured sprite animation frames from BlastEm
- Screen captures of title screen and market level (`aladdin_title_screencap.json`, `aladdin_market_screencap.json`)

### Moonwalker (`sprite-editor/examples/moonwalker.json`)

Target ROM: Michael Jackson's Moonwalker (USA, JUE, checksum 0x38B2).

- Primary sprite bank: 0x01B000-0x02A400 (~1952 tiles)
- Confirmed sprite groups: MJ animations at 0x0220E0 (2x4), 0x023800 (2x5), 0x0290A0 (3x5)
- MJ Gray Suit palette at 0x0082CA
- Tile data is identical across ROM revisions; palette offsets differ

## Project Structure

```
genesis-hacking/           Main repository
  sprite-editor/           Qt6/C++ sprite editor application
    SpriteViewerPanel.*    Collection browser with drag-reorder
    RawTileBrowserPanel.*  ROM tile data exploration
    ScreenCapturePanel.*   BlastEm screen capture viewer/editor
    SpriteAnimationPanel.* Recording playback and capture workflow
    SpriteEditorPanel.*    Pixel-level painting editor
    PaintToolPanel.*       Reusable 2x2 tool buttons + 4x4 palette
    RomDataService.*       Data resolution bridge (definition -> display)
    GenesisTypes.h         Shared TileBlock/TileBlockGroup types
    MainWindow.*           Thin shell (~680 lines) wiring panels together
    examples/              Game definition JSON files + recordings
    docs/                  User guide and screenshots
  docs/                    Genesis hardware documentation
  tools/                   BlastEm trace analysis scripts
  roms/                    ROM files (gitignored)

bn-genesis/                Binary Ninja plugin (separate repo)
  genesis/                 Python plugin modules
    loader.py              ROM loader, memory mapping, header parsing
    game_definition.py     Label sprites/palettes from JSON definitions
    vdp_analysis.py        VDP register write annotations
    assemble.py            M68K assembly and ROM patching
    checksum.py            ROM checksum calculator
  cpp_ui/                  C++ sprite viewer sidebar widget
    GenesisSpriteViewer.*  Native Qt6 sidebar for tile visualization

blastem/                   Forked Genesis emulator (separate repo)
  debug.c                  Debugger: spritecap, spriterecord, screencap
  vdp.c                    VDP with DMA history tracking
```

## Documentation

- [Sprite Editor User Guide](sprite-editor/docs/USAGE.md) — full guide with screenshots
- [PLANNING.md](PLANNING.md) — development history and roadmap
- `docs/genvdp-charles-macdonald.txt` — authoritative VDP/tile/CRAM reference
