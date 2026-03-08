# Sprite Editor JSON Format

The Sprite Editor reads game definition files in JSON format. These files describe
sprite locations, palettes, tile ranges, screen captures, sprite collections, and
sprite animations for Sega Genesis ROMs.

See `examples/moonwalker.json` for a complete example.

## Root Object

| Field | Type | Default | Description |
|-------|------|---------|-------------|
| `game_name` | string | "Unknown Game" | Display name of the game |
| `game_id` | string | "unknown" | Short identifier (no spaces) |
| `sprite_groups` | array | [] | Named groups of sprites with palettes |
| `tile_ranges` | array | auto | ROM regions to browse in Raw Tile Browser |
| `screen_captures` | array | [] | Full-screen tile map reconstructions |
| `sprite_collections` | array | [] | Hardware sprite snapshots (from BlastEm `spritecap`) |
| `sprite_animation` | object | absent | Frame-by-frame sprite recording (from BlastEm `spriterecord`) |

All top-level fields are optional. Unknown keys (e.g. `_note`, `notes`) are ignored
by the parser and can be used freely for documentation.

---

## sprite_groups

Array of sprite group objects. Each group organizes related sprites that share
a set of palettes — for example, all player character animations.

```json
{
  "name": "MJ - Player Character",
  "palettes": [ ... ],
  "sprites": [ ... ]
}
```

### Sprite Group Fields

| Field | Type | Default | Description |
|-------|------|---------|-------------|
| `name` | string | "Unnamed Group" | Display name shown in combo box |
| `palettes` | array | [] | Palette definitions for this group |
| `sprites` | array | [] | Sprite entries in this group |

### palettes[] — PaletteDefinition

Each palette is 16 colors (32 bytes) read directly from the ROM.

| Field | Type | Default | Description |
|-------|------|---------|-------------|
| `name` | string | "Palette" | Display name |
| `rom_offset` | hex string | "0x0" | Byte offset to 16 × 2-byte CRAM entries in ROM |

### sprites[] — SpriteEntry

Each entry defines a sprite (or sequence of animation frames) at a ROM location.

| Field | Type | Default | Validation | Description |
|-------|------|---------|------------|-------------|
| `name` | string | "Unnamed Sprite" | — | Display name |
| `rom_offset` | hex string | "0x0" | — | ROM byte offset to tile data |
| `width_tiles` | int | 1 | clamped [1, 8] | Width in 8px tiles |
| `height_tiles` | int | 1 | clamped [1, 8] | Height in 8px tiles |
| `frame_count` | int | 1 | min 1 | Number of consecutive frames at this offset |
| `palette_index` | int | 0 | clamped [0, 3] | Index into parent group's `palettes` array |
| `compression` | string | "none" | — | "none", "kosinski", or "nemesis" |
| `notes` | string | "" | — | Optional documentation |

Each frame is `width_tiles × height_tiles × 32` bytes. Frames are stored
consecutively starting at `rom_offset`.

---

## tile_ranges

Array of ROM address ranges for the Raw Tile Browser tab. If this array is
empty or absent, a single default range is created automatically:

```json
{ "label": "Full ROM (0x200 - end)", "start_offset": "0x200", "end_offset": "0x80000" }
```

### TileRange Fields

| Field | Type | Default | Description |
|-------|------|---------|-------------|
| `label` | string | "Range" | Display name in combo box |
| `start_offset` | hex string | "0x200" | Start of range in ROM |
| `end_offset` | hex string | "0x80000" | End of range in ROM |
| `default_palette_group` | int | -1 | Index into `sprite_groups` for default palette; -1 = none |

---

## screen_captures

Array of full-screen tile-map reconstructions. Each capture represents a
complete Genesis screen (typically 40×28 tiles = 320×224 pixels) with
per-tile metadata and embedded tile data.

```json
{
  "name": "Title Screen",
  "width_tiles": 40,
  "height_tiles": 28,
  "palettes": [ ... ],
  "tile_map": [ ... ],
  "embedded_tiles": { ... }
}
```

### ScreenCapture Fields

| Field | Type | Default | Description |
|-------|------|---------|-------------|
| `name` | string | "Unnamed Capture" | Display name |
| `width_tiles` | int | 40 | Screen width in 8px tiles |
| `height_tiles` | int | 28 | Screen height in 8px tiles |
| `palettes` | array | [] | CRAM palette lines (up to 4) |
| `tile_map` | array | [] | Per-tile entries covering the screen |
| `embedded_tiles` | object | {} | VRAM address → hex tile data |

### palettes[] — ScreenCapturePalette

Used by screen captures, sprite collections, and sprite animations.

| Field | Type | Default | Description |
|-------|------|---------|-------------|
| `line` | int | 0 | CRAM palette line (0–3) |
| `cram_values` | array of hex strings | [] | 16 Genesis CRAM color words (e.g. "0EEE") |
| `dma_source` | hex string or null | null | ROM offset where palette was DMA'd from |

Each CRAM value is a 16-bit word in Genesis format: `0x0BGR` where B, G, R
are 3-bit values (0–E in even steps).

### tile_map[] — TileMapEntry

Each entry describes one 8×8 tile position on the screen.

| Field | Type | Default | Description |
|-------|------|---------|-------------|
| `row` | int | 0 | Tile row (0-based) |
| `col` | int | 0 | Tile column (0-based) |
| `pattern` | int | 0 | VDP tile pattern index |
| `palette_line` | int | 0 | Which CRAM line (0–3) |
| `h_flip` | bool | false | Horizontal flip |
| `v_flip` | bool | false | Vertical flip |
| `priority` | bool | false | VDP priority bit |
| `rom_offset` | hex string or null | null | Source ROM offset (if known) |
| `source` | string | "blank" | How the tile was found: "dma", "search", "embedded", "blank" |

### embedded_tiles — Object

Key-value map where keys are VRAM addresses (hex strings like `"0xC000"`)
and values are raw tile data as hex strings (64 hex chars = 32 bytes = one
8×8 4bpp tile).

---

## sprite_collections

Array of hardware sprite snapshots. Generated by BlastEm's `spritecap`
debugger command. Each collection captures the sprite attribute table (SAT)
state at a single moment.

```json
{
  "name": "Player idle frame",
  "bounding_box": { "x": 100, "y": 80, "width": 48, "height": 64 },
  "palettes": [ ... ],
  "sprites": [ ... ]
}
```

### SpriteCollection Fields

| Field | Type | Default | Description |
|-------|------|---------|-------------|
| `name` | string | "Unnamed Collection" | Display name |
| `bounding_box` | object | {x:0, y:0, width:0, height:0} | Overall bounding rectangle |
| `palettes` | array | [] | ScreenCapturePalette objects (up to 4 lines) |
| `sprites` | array | [] | Individual hardware sprites |

### bounding_box

| Field | Type | Default |
|-------|------|---------|
| `x` | int | 0 |
| `y` | int | 0 |
| `width` | int | 0 |
| `height` | int | 0 |

### sprites[] — CollectionSprite

Each entry represents one Genesis hardware sprite from the SAT.

| Field | Type | Default | Validation | Description |
|-------|------|---------|------------|-------------|
| `index` | int | 0 | — | Hardware sprite index (0–79) |
| `x` | int | 0 | — | Screen X position (offset by -128) |
| `y` | int | 0 | — | Screen Y position (offset by -128) |
| `width_tiles` | int | 1 | clamped [1, 4] | Width in 8px tiles |
| `height_tiles` | int | 1 | clamped [1, 4] | Height in 8px tiles |
| `palette_line` | int | 0 | clamped [0, 3] | CRAM palette line |
| `priority` | bool | false | — | VDP priority bit |
| `h_flip` | bool | false | — | Horizontal flip |
| `v_flip` | bool | false | — | Vertical flip |
| `pattern` | int | 0 | — | Tile pattern index in VRAM |
| `vram_addr` | string | "" | — | VRAM address (hex string) |
| `rom_offset` | hex string or null | null | — | Source ROM offset (if resolved) |
| `source` | string | "dma" | — | How tile data was found: "dma", "search", "embedded" |
| `tile_data` | hex string | "" | — | Embedded tile data (used when source = "embedded") |

Note: Genesis hardware sprites are limited to 4×4 tiles (32×32 pixels) maximum.
The `width_tiles` and `height_tiles` fields are clamped to [1, 4] accordingly.
This differs from `sprite_groups` sprites which allow up to 8×8.

---

## sprite_animation

A single object (not an array) containing frame-by-frame sprite recordings.
Generated by BlastEm's `spriterecord` / `spriterecordstop` debugger commands.
Consecutive duplicate frames are omitted during recording.

```json
{
  "sprite_animation": {
    "game_name": "Michael Jackson's Moonwalker",
    "palettes": [
      { "line": 0, "cram_values": ["0EEE", "0CCC", ...], "dma_source": "0x0082CA" },
      { "line": 1, "cram_values": [...], "dma_source": null }
    ],
    "frames": [
      {
        "frame": 12345,
        "bounding_box": { "x": 96, "y": 72, "width": 48, "height": 64 },
        "sprites": [ ... ]
      }
    ]
  }
}
```

### SpriteAnimation Fields

| Field | Type | Default | Description |
|-------|------|---------|-------------|
| `game_name` | string | "Unknown" | Game name from BlastEm |
| `palettes` | array | [] | ScreenCapturePalette objects shared across all frames |
| `frames` | array | [] | Array of animation frames |

### frames[] — AnimationFrame

| Field | Type | Default | Description |
|-------|------|---------|-------------|
| `frame` | int | 0 | VDP frame number (monotonically increasing, gaps where duplicates were skipped) |
| `bounding_box` | object | {x:0, y:0, width:0, height:0} | Bounding box for sprites in this frame |
| `sprites` | array | [] | CollectionSprite objects (same format as sprite_collections) |

The `palettes` are defined once at the animation level (not per-frame) since
CRAM values are captured at recording start and assumed stable.

---

## Hex Offset Format

All hex offset fields (`rom_offset`, `start_offset`, `end_offset`, `vram_addr`,
`dma_source`) accept either format:

- With prefix: `"0x0082CA"` or `"0X0082CA"`
- Without prefix: `"0082CA"`

Leading zeros are optional. Parsing is case-insensitive. Invalid hex strings
fall back to a context-dependent default (usually 0).
