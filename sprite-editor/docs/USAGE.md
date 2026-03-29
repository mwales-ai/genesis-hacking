# Genesis Sprite Editor - User Guide

A Qt6/C++ tool for viewing, exporting, and replacing sprite artwork in Sega Genesis / Megadrive ROMs. Sprite locations are described by a JSON game definition file, making the tool usable with any Genesis game.

## Building

Requires Qt6 development libraries (`qt6-base-dev` or equivalent).

```bash
cd sprite-editor
bash build.sh
```

The binary is output to `sprite-editor/build/SpriteEditor`.

## Quick Start

Launch with command-line arguments to skip the file dialogs:

```bash
./build/SpriteEditor path/to/rom.bin path/to/definition.json
```

File type is auto-detected: `.json` files are loaded as game definitions, everything else as a ROM. Both arguments are optional and can be in any order.

You can also open files from within the app using **File > Open ROM** (`Ctrl+O`) and **File > Open Game Definition** (`Ctrl+D`).

![File menu](screenshots/07_file_menu.png)

## Sprite Viewer Tab

The Sprite Viewer displays all sprites defined in the JSON game definition, organized by groups.

### Browsing Sprites

1. Load a ROM and game definition (via File menu or command line)
2. All sprite groups from the game definition appear as thumbnails in the left panel
3. Use the **Thumb Zoom** spinbox to adjust thumbnail size independently
4. Drag and drop thumbnails to reorder sprite groups (saved to the definition file)

### Viewing Sprite Details

Click any thumbnail to select it. The right panel shows:

- **Name** field (editable — press Enter to rename the group)
- **Zoomed composite preview** of all sprites in the group
- **Borders** toggle — when enabled, shows 1px palette-colored outlines around
  each hardware sprite (borders stay 1px regardless of zoom level)
- **Info** showing sprite count, pixel dimensions, ROM offset coverage, and palette details
- **Edit** button to open the group in the Sprite Editor tab
- **Delete** button to remove the group from the game definition (with confirmation)

Use the **Zoom** spinbox to adjust the detail view.

**Double-click** a sprite in the detail view to jump to the **Raw Tile Browser** tab
with the correct palette, tile W/H, and ROM address pre-filled.

### Exporting Sprites

- **Export PNG** - Save the currently selected sprite as a PNG file
- **Export All** - Batch-export every frame in the current sprite group to a directory. Multi-frame entries get numbered suffixes (`_000.png`, `_001.png`, etc.)

### Replacing Sprites

1. Select a sprite in the grid
2. Click **Replace Sprite** (or **Edit > Replace Sprite...**)
3. Browse for a PNG image. It will be scaled to match the sprite dimensions if needed
4. The preview shows exactly what will be written to the ROM (quantized to the 16-color palette)
5. Click OK to write the replacement into the ROM
6. Use **File > Save ROM** (`Ctrl+S`) to save changes

Only uncompressed sprites can be replaced directly. For compressed sprites, locate the uncompressed tile data in the Raw Tile Browser and update the JSON definition accordingly.

## Raw Tile Browser Tab

The Raw Tile Browser lets you explore raw 8x8 tile data anywhere in the ROM. This is the primary tool for discovering sprite locations in games that don't have a definition file yet.

### Basic Browsing

![Raw tile browser showing individual tiles in greyscale](screenshots/03_raw_browser_default.png)

- **Range** - Select a ROM address range to browse. If a game definition is loaded, it provides named ranges; otherwise a default "Full ROM" range is used.
- **Palette** - Choose between greyscale (default) or any palette from the game definition.
- **Zoom** - Adjust tile display size.

### Applying a Palette

Selecting a palette from the dropdown immediately re-renders all tiles with those colors, making sprite data much easier to identify visually.

![Raw tiles with MJ Gray Suit palette applied - character data becomes visible](screenshots/04_raw_browser_palette.png)

### Assembling Tiles into Sprites

Genesis sprites are composed of multiple 8x8 tiles arranged in a grid. Use the **Tiles W x H** spinboxes to group tiles into assembled sprites:

1. Set **W** and **H** to the sprite dimensions in tiles (e.g., 2x4 for a 16x32 pixel sprite)
2. Enter the starting ROM offset in the **Jump** field (hex, e.g., `0x0220E0`)
3. Click **Start** to set that offset as the assembly origin

The tiles are assembled in Genesis column-major order: tile at (col, row) = col * height + row.

![Assembled 2x4 sprites showing MJ character animation frames](screenshots/05_raw_browser_assembled.png)

### Getting Tile Information

Click any tile or assembled sprite to see its ROM offset in the status bar. This is essential for building game definition files.

![Clicking a sprite shows its tile range and ROM offset](screenshots/06_raw_tile_clicked.png)

### Exporting from Raw Browser

Click a tile or sprite, then click **Export PNG** to save it as a PNG file using the currently selected palette.

## Screen Captures Tab

The Screen Captures tab displays full-screen tile maps captured from BlastEm using the `screencap` debugger command. This lets you see exactly what the Genesis is rendering and trace every tile back to its ROM location.

### Capturing a Screen

1. Run your ROM in BlastEm with the debugger: `blastem -d rom.bin`
2. Play until the desired screen (e.g., title screen)
3. Enter the debugger (press backtick)
4. Run: `screencap title.json` (or `screencap title.json b` for Plane B)
5. Load the resulting JSON in the sprite editor as a game definition

### Viewing Screen Captures

1. Load the captured JSON file via **File > Open Game Definition**
2. Switch to the **Screen Captures** tab
3. Select a capture from the dropdown (if the file contains multiple)
4. Use the **Zoom** spinbox to adjust the display scale (1x-8x)
5. Hover over any tile to see its details: pattern index, palette line, flip flags, ROM offset, and source type

### Tile Source Types

Each tile in the capture has a source indicating how its ROM location was found:

| Source | Meaning |
|--------|---------|
| `dma` | ROM offset found via DMA transfer history (most reliable) |
| `search` | ROM offset found by brute-force search of tile data in ROM |
| `embedded` | Tile not found in ROM; raw data is embedded in the JSON |
| `blank` | Tile is all zeros (empty/transparent) |

### Loading Screen Captures

Screen captures can be loaded in two ways:
- Via **File > Open Game Definition** (accepts both `.json` definitions and standalone screen capture files)
- Via the **Load Capture...** button on the Screen Captures tab

External captures appear with an `[ext]` prefix. Use **Add to Definition** to save them
into the game definition, or **Remove** to delete captures from the definition.

### Editing Screen Capture Tiles

Click the **Edit** button to enable pixel-level tile painting:

1. A tool panel appears on the right with pencil, fill, eyedropper, and brush tools
   in a 2x2 icon grid, plus a 4x4 palette selector
2. All tiles sharing the same ROM offset are highlighted in yellow when you hover
3. The current tile under the cursor is outlined in green
4. Painting on a tile with a ROM address writes directly to the ROM buffer
5. Click **Save Changes to ROM** to write modifications to disk
6. Click **Revert to Original** to discard all painting and reload from ROM

The status bar at the bottom shows tile info (position, pattern, palette, ROM offset)
and capture statistics (unique patterns, ROM address recovery, palette info).

**Note:** Only tiles with recovered ROM addresses can be saved. Embedded tiles
(captured VRAM data without a known ROM location) can be painted but cannot be
persisted to the ROM file.

### Optionally Loading a ROM

If you also load the corresponding ROM file, the editor reads tile data directly from ROM offsets. Without a ROM, only embedded tiles and blank tiles will display correctly.

## Game Definition JSON Format

A game definition file tells the editor where sprites and palettes are located in a specific ROM. Here is the schema:

```json
{
  "game_name": "Game Title",
  "game_id": "game_id",

  "sprite_groups": [
    {
      "name": "Group Name",
      "palettes": [
        {
          "name": "Palette Name",
          "rom_offset": "0x008000"
        }
      ],
      "sprites": [
        {
          "name": "Sprite Name",
          "rom_offset": "0x020000",
          "width_tiles": 2,
          "height_tiles": 4,
          "frame_count": 23,
          "palette_index": 0,
          "compression": "none",
          "notes": "Optional description"
        }
      ]
    }
  ],

  "tile_ranges": [
    {
      "label": "Main Sprite Bank (0x1B000-0x2A400)",
      "start_offset": "0x01B000",
      "end_offset": "0x02A400",
      "default_palette_group": 0
    }
  ]
}
```

### Field Reference

**sprite_groups[]**

| Field | Description |
|-------|-------------|
| `name` | Display name for the group |
| `palettes[]` | Array of palette definitions (16-color, 32 bytes each at `rom_offset`) |
| `sprites[]` | Array of sprite entries |

**sprites[]**

| Field | Type | Default | Description |
|-------|------|---------|-------------|
| `name` | string | required | Display name |
| `rom_offset` | hex string | required | ROM file offset (e.g., `"0x0220E0"`) |
| `width_tiles` | int | 1 | Width in 8px tiles (1-8) |
| `height_tiles` | int | 1 | Height in 8px tiles (1-8) |
| `frame_count` | int | 1 | Number of consecutive frames at this offset |
| `palette_index` | int | 0 | Index into the parent group's `palettes` array |
| `compression` | string | `"none"` | `"none"`, `"nemesis"`, or `"kosinski"` |
| `notes` | string | `""` | Optional notes (not displayed in UI) |

When `frame_count` > 1, the editor treats the data at `rom_offset` as N consecutive sprites of the same dimensions, each `width_tiles * height_tiles * 32` bytes apart.

**tile_ranges[]**

| Field | Type | Default | Description |
|-------|------|---------|-------------|
| `label` | string | required | Display name in the Range dropdown |
| `start_offset` | hex string | `"0x200"` | Start of browsable range |
| `end_offset` | hex string | `"0x80000"` | End of browsable range |
| `default_palette_group` | int | -1 | Sprite group index for auto-palette selection |

**screen_captures[]** (optional, generated by BlastEm `screencap` command)

```json
"screen_captures": [
  {
    "name": "title_screen",
    "width_tiles": 40,
    "height_tiles": 28,
    "palettes": [
      { "line": 0, "cram_values": ["0EEE", "0E80", ...], "dma_source": "0x1A200" },
      { "line": 1, "cram_values": [...], "dma_source": null }
    ],
    "tile_map": [
      {
        "row": 0, "col": 0, "pattern": 145, "palette_line": 2,
        "h_flip": false, "v_flip": false, "priority": false,
        "rom_offset": "0x1B240", "source": "dma"
      }
    ],
    "embedded_tiles": {
      "0x1234": "AABBCCDD..."
    }
  }
]
```

| Field | Description |
|-------|-------------|
| `name` | Capture identifier |
| `width_tiles` / `height_tiles` | Visible dimensions (typically 40x28 NTSC, 40x30 PAL) |
| `palettes[]` | All 4 CRAM palette lines: `line` (0-3), `cram_values` (16 hex color words), `dma_source` (ROM offset or null) |
| `tile_map[]` | One entry per visible tile: `row`, `col`, `pattern` (VDP tile index), `palette_line`, `h_flip`, `v_flip`, `priority`, `rom_offset` (hex or null), `source` (`dma`/`search`/`embedded`/`blank`) |
| `embedded_tiles` | Hex-encoded 32-byte tile data keyed by VRAM address, for tiles not found in ROM |

See `examples/moonwalker.json` for a complete working example.

### Normalized Format

The normalized format uses shared palette and pattern pools referenced by ID,
rather than duplicating data in each sprite group:

```json
{
  "game_name": "Game Title",
  "game_id": "game_id",
  "palettes": {
    "pal_id": {
      "name": "Palette Name",
      "rom_offset": "0x8000",
      "cram_values": ["0x0000", "0x0eee", ...]
    }
  },
  "patterns": {
    "pat_id": {
      "name": "Pattern Name",
      "rom_offset": "0x20000",
      "width_tiles": 2,
      "height_tiles": 4,
      "frame_count": 1,
      "compression": "none"
    }
  },
  "sprite_collections": {
    "col_id": {
      "name": "Collection Name",
      "sprites": [
        {
          "pattern": "pat_id",
          "palette": "pal_id",
          "x": 0, "y": 0,
          "h_flip": false, "v_flip": false
        }
      ]
    }
  }
}
```

## Sprite Recording Workflow (.sprec files)

Sprite recordings capture hardware sprite data across multiple frames from
BlastEm's debugger, allowing frame-by-frame analysis of game animations.

### Recording Sprites in BlastEm

1. Run your ROM in BlastEm with the debugger: `blastem -d rom.bin`
2. Play until the sprites you want to capture are on screen
3. Enter the debugger (press backtick)
4. Run: `spritecap output.sprec` to start recording sprite data
5. Resume the game to capture frames, then stop recording in the debugger

### Loading Recordings

```bash
./build/SpriteEditor rom.bin definition.json recording.sprec
```

Or use **File > Open Game Definition** and select a `.sprec` file.
Recordings appear in the **Sprite Animations** tab. Use the **Load Recording...**
button on that tab to load additional `.sprec` files at any time.

### Aladdin Example Walkthrough

The `examples/` directory includes Aladdin sample files:
- `aladdin_sprite_def.json` — game definition with sprite groups, palettes, and pattern pools
- `ali_animations.sprec` — multi-frame sprite animation recording
- `aladdin_spriterecord.sprec` — additional sprite recording
- `aladdin_market_screencap.json` — market scene screen capture
- `aladdin_title_screencap.json` — title screen capture

To explore all features:
```bash
./build/SpriteEditor roms/Aladdin_Beta.bin examples/aladdin_sprite_def.json examples/ali_animations.sprec
```

This loads the ROM, game definition (sprite groups visible in Sprite Viewer), and
animation recording (visible in Sprite Animations tab). You can then:
1. Browse captured sprite groups in the **Sprite Viewer** tab
2. Load screen captures via **Screen Captures > Load Capture...**
3. Browse animation frames in the **Sprite Animations** tab
4. Edit sprites in the **Sprite Editor** tab with painting tools

## Multi-Select and Capture Workflow

The Sprite Animations tab supports multi-select for identifying and
capturing sprite groups from recordings.

### Multi-Select

- **Click** a sprite to select it (opens in pixel editor)
- **Ctrl+Click** to toggle additional sprites in/out of the selection
- **Rubber-band drag** on empty space to select all sprites in a rectangle
- **Ctrl+rubber-band** adds to existing selection

The selection label below the controls shows how many sprites are selected.

### Capturing Sprite Groups

After selecting sprites that form a character or object:

1. Click **Capture Sprite Group**
2. Enter a name for the group (e.g., "Aladdin Walk Frame 1")
3. The editor creates palette pool entries, pattern pool entries, and a
   normalized sprite collection in the game definition JSON
4. The game definition is auto-saved

Sprite positions are normalized relative to the group's bounding box origin.

### Hide/Unhide Sprites

After capturing a group, you can hide those sprites to reduce visual
clutter when analyzing subsequent frames:

- **Hide Selected** — removes selected sprites from rendering (they still
  appear as dashed outlines when hovered or selected)
- **Unhide Selected** — unhide only the selected hidden sprites
- **Unhide All** — restores all hidden sprites

Hidden sprites persist across frame changes via ROM offset tracking.

## Composite Sprite Editor

When clicking a multi-sprite normalized collection in the Sprite Animations tab
or using the Edit button in the Sprite Viewer, the pixel editor opens in
**group mode**:

![Sprite Editor tab showing Aladdin with painting tools and 4x4 palette](screenshots/09_sprite_editor.png)

- All sprites are composited at their correct positions
- Painting works across sprite boundaries — the editor automatically
  determines which sprite owns each pixel
- Dashed yellow outlines show individual sprite boundaries
- Middle-click a sprite to switch the active palette line
- **Save Tiles to ROM** writes each sprite's modified tile data to its
  own ROM offset

### Painting Tools

The tool panel on the right side of the editor provides:

- **2x2 icon buttons** — Pencil (single pixel), Fill (flood fill), Eyedropper
  (pick color), and Brush (adjustable size square)
- **Brush Size** spinner — appears below the tools only when the Brush is active
- **4x4 palette grid** — quick color selection showing all 16 palette colors
- **Delete Sprite** button — remove a sprite from a group (visible after middle-click)

**Right-click** anywhere to eyedrop (pick color) regardless of the active tool.

### ROM Overwrite Protection

When saving ROM changes, if the target file is the original ROM, you will be
prompted to save to a different file instead. This prevents accidentally
overwriting original ROM data.

This is essential for editing characters that span multiple hardware sprites.

## Genesis Tile Format Reference

- **Tile size**: 8x8 pixels, 4 bits per pixel = 32 bytes per tile
- **Pixel encoding**: Each byte contains 2 pixels (high nibble = left pixel, low nibble = right pixel)
- **Palette index 0**: Transparent in sprites
- **CRAM palette format**: 16 colors per palette, 2 bytes each (big-endian): `---- bbb- ggg- rrr-` (3 bits per channel, values 0-7 mapped to 0-252 brightness)
- **Sprite tile order**: Column-major. For a W x H sprite, tile at column `c`, row `r` is stored at index `c * H + r`
- **ROM storage**: Tiles are stored as a contiguous byte stream. A 2x4 sprite = 8 tiles = 256 bytes

## VRAM/CRAM Address Tracking

When sprites are captured from BlastEm, their VDP memory addresses are preserved
for reverse engineering:

- **VRAM address** — where the tile data was stored in VDP memory at capture time
- **CRAM address** — where the palette data was in color RAM (palette line x 32)
- **DMA source** — the 68K address that DMA transferred data from (ROM or RAM)

These addresses appear in:
- **Sprite Viewer** info panel — shows VRAM address for each sprite in a group
- **Sprite Editor** middle-click — shows VRAM address, DMA source, and dimensions
- **Game definition JSON** — `vram_addr` field preserved during capture normalization

### Why VRAM/CRAM Addresses Matter

When a sprite's tile data was DMA'd from RAM (not ROM), the ROM offset is unknown.
The VRAM address tells you where the VDP stored the data, and the DMA source tells
you which RAM address the data came from. You can then search Binary Ninja for code
that writes to that RAM address to trace the data's origin.

## BlastEm Code Trace Workflow

BlastEm can record branch and jump targets during emulation, which Binary Ninja
can import to improve its auto-analysis of the ROM.

### Recording Branch Targets

1. Run your ROM in BlastEm with the debugger: `blastem -d rom.bin`
2. Enter the debugger (press backtick)
3. Start recording: `codetrace output.json`
4. Resume emulation and play through the sections you want to analyze
5. Re-enter the debugger and stop: `codetracestop`

The output JSON contains every unique JSR, JMP, BSR, and Bcc target address
encountered during execution.

### Importing into Binary Ninja

1. Open the ROM in Binary Ninja with the bn-genesis plugin
2. Run **Plugins > genesis: import code trace**
3. Select the `.json` file from BlastEm
4. JSR/BSR targets become new functions, JMP targets become functions,
   Bcc targets get branch labels

This is especially valuable for:
- Indirect jumps through registers or memory (jump tables)
- Computed branch targets that static analysis can't resolve
- Code that's only reached through specific game state

## Keyboard Shortcuts

| Shortcut | Action |
|----------|--------|
| `Ctrl+O` | Open ROM |
| `Ctrl+D` | Open Game Definition |
| `Ctrl+S` | Save ROM |
| `Ctrl+Q` | Quit |
| Arrow keys | Navigate sprite grid (Sprite Viewer) |
| Home / End | Jump to first / last sprite |
