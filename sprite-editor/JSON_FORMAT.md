# Sprite Editor JSON Format

The Sprite Editor uses two file formats:

1. **Game Definition** (`.json`) — describes sprite locations, palettes, tile ranges, screen captures, and sprite collections for a Sega Genesis ROM
2. **Sprite Recording** (`.sprec`) — frame-by-frame sprite captures from BlastEm's debugger

See `examples/moonwalker.json` for a complete game definition example.

---

## Game Definition Format (Normalized)

The current format uses root-level **palette** and **pattern pools** referenced by string IDs. Sprite collections compose sprites from these pools.

### Root Object

| Field | Type | Default | Description |
|-------|------|---------|-------------|
| `game_name` | string | "Unknown Game" | Display name of the game |
| `game_id` | string | "unknown" | Short identifier (no spaces) |
| `palettes` | object | {} | Keyed palette pool (ID → palette definition) |
| `patterns` | object | {} | Keyed pattern pool (ID → pattern definition) |
| `sprite_collections` | object | {} | Keyed collection pool (ID → collection definition) |
| `screen_captures` | array | [] | Full-screen tile map reconstructions |
| `tile_ranges` | array | auto | ROM regions to browse in Raw Tile Browser |

All top-level fields are optional. Unknown keys (e.g. `_note`, `notes`) are ignored
by the parser and can be used freely for documentation.

### palettes — Palette Pool

Object where each key is a palette ID and the value defines the palette.
A palette can specify either a ROM offset (to read 32 bytes of CRAM data) or
inline CRAM values.

```json
{
  "palettes": {
    "mj_gray_suit": {
      "name": "MJ Gray Suit Palette",
      "rom_offset": "0x0082CA"
    },
    "captured_pal": {
      "name": "Captured from emulator",
      "cram_values": ["0EEE", "0CCC", "0AAA", "0888", "0666",
                       "0EC8", "0CA6", "0A84", "0AAA", "0888",
                       "0CCC", "0EEE", "0840", "0620", "0800", "0E8E"]
    }
  }
}
```

| Field | Type | Default | Description |
|-------|------|---------|-------------|
| `name` | string | (key) | Display name |
| `rom_offset` | hex string | "0x0" | ROM offset to 16 x 2-byte CRAM entries (32 bytes) |
| `cram_values` | array of hex strings | [] | 16 Genesis CRAM color words (alternative to rom_offset) |

### patterns — Pattern Pool

Object where each key is a pattern ID and the value defines a sprite pattern
(tile layout and ROM location).

```json
{
  "patterns": {
    "mj_anim_a_2x4": {
      "name": "MJ Anim Set A (2x4)",
      "rom_offset": "0x0220E0",
      "width_tiles": 2,
      "height_tiles": 4,
      "frame_count": 23,
      "compression": "none"
    }
  }
}
```

| Field | Type | Default | Validation | Description |
|-------|------|---------|------------|-------------|
| `name` | string | (key) | — | Display name |
| `rom_offset` | hex string | "0x0" | — | ROM byte offset to tile data |
| `width_tiles` | int | 1 | clamped [1, 8] | Width in 8px tiles |
| `height_tiles` | int | 1 | clamped [1, 8] | Height in 8px tiles |
| `frame_count` | int | 1 | min 1 | Number of consecutive frames at this offset |
| `compression` | string | "none" | — | "none", "kosinski", or "nemesis" |
| `tile_data` | hex string | "" | — | Optional embedded tile data (alternative to rom_offset) |

Each frame is `width_tiles x height_tiles x 32` bytes. Frames are stored
consecutively starting at `rom_offset`.

### sprite_collections — Collections

Object where each key is a collection ID. Each collection composes sprites
from the pattern and palette pools.

```json
{
  "sprite_collections": {
    "mj_idle": {
      "name": "MJ Idle Pose",
      "sprites": [
        {
          "pattern": "mj_anim_a_2x4",
          "frame": 0,
          "palette": "mj_gray_suit",
          "x": 0, "y": 0,
          "h_flip": false, "v_flip": false,
          "priority": false
        }
      ]
    }
  }
}
```

#### Sprite Entry (in collection)

| Field | Type | Default | Description |
|-------|------|---------|-------------|
| `pattern` | string | "" | Pattern pool ID |
| `frame` | int | 0 | Frame index within the pattern (0-based) |
| `palette` | string | "" | Palette pool ID |
| `x` | int | 0 | X position relative to collection origin |
| `y` | int | 0 | Y position relative to collection origin |
| `h_flip` | bool | false | Horizontal flip |
| `v_flip` | bool | false | Vertical flip |
| `priority` | bool | false | VDP priority bit |

---

## tile_ranges

Array of ROM address ranges for the Raw Tile Browser tab. If this array is
empty or absent, a single default range is created automatically.

| Field | Type | Default | Description |
|-------|------|---------|-------------|
| `label` | string | "Range" | Display name in combo box |
| `start_offset` | hex string | "0x200" | Start of range in ROM |
| `end_offset` | hex string | "0x80000" | End of range in ROM |
| `default_palette` | string | "" | Palette pool ID for default rendering |
| `default_palette_group` | int | -1 | (Legacy) Index into `sprite_groups` array |

---

## screen_captures

Array of full-screen tile-map reconstructions. Each capture represents a
complete Genesis screen (typically 40x28 tiles = 320x224 pixels) with
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

| Field | Type | Default | Description |
|-------|------|---------|-------------|
| `name` | string | "Unnamed Capture" | Display name |
| `width_tiles` | int | 40 | Screen width in 8px tiles |
| `height_tiles` | int | 28 | Screen height in 8px tiles |
| `palettes` | array | [] | CRAM palette lines (up to 4) — same format as .sprec |
| `tile_map` | array | [] | Per-tile entries covering the screen |
| `embedded_tiles` | object | {} | VRAM address hex -> hex tile data |

---

## .sprec File Format (Sprite Recording)

Produced by BlastEm's `spritecap` and `spriterecord` debugger commands.
A `.sprec` file is a standalone JSON file containing captured sprite data.

```json
{
  "game_name": "Michael Jackson's Moonwalker",
  "palettes": [
    {
      "line": 0,
      "cram_values": ["0EEE", "0CCC", "0AAA", ...],
      "dma_source": "0x0082CA"
    },
    { "line": 1, "cram_values": [...], "dma_source": null },
    { "line": 2, "cram_values": [...], "dma_source": null },
    { "line": 3, "cram_values": [...], "dma_source": null }
  ],
  "frames": [
    {
      "frame": 12345,
      "bounding_box": { "x": 96, "y": 72, "width": 48, "height": 64 },
      "sprites": [
        {
          "index": 0,
          "x": 128, "y": 96,
          "width_tiles": 2, "height_tiles": 4,
          "palette_line": 0,
          "priority": false,
          "h_flip": false, "v_flip": false,
          "pattern": 512,
          "vram_addr": "0x4000",
          "rom_offset": "0x0220E0",
          "source": "dma"
        }
      ]
    }
  ]
}
```

### .sprec Root Fields

| Field | Type | Description |
|-------|------|-------------|
| `game_name` | string | Game name from BlastEm ROM header |
| `palettes` | array | All 4 CRAM palette lines at capture time |
| `frames` | array | One or more captured frames (spritecap = 1, spriterecord = many) |

### palettes[] — ScreenCapturePalette

| Field | Type | Default | Description |
|-------|------|---------|-------------|
| `line` | int | 0 | CRAM palette line (0-3) |
| `cram_values` | array of hex strings | [] | 16 Genesis CRAM color words (e.g. "0EEE") |
| `dma_source` | hex string or null | null | ROM offset where palette was DMA'd from |

Each CRAM value is a 16-bit word in Genesis format: `0x0BGR` where B, G, R
are 3-bit values (0-E in even steps).

### frames[] — AnimationFrame

| Field | Type | Default | Description |
|-------|------|---------|-------------|
| `frame` | int | 0 | VDP frame number |
| `bounding_box` | object | {x:0, y:0, width:0, height:0} | Overall bounding rectangle |
| `sprites` | array | [] | Hardware sprites in this frame |

### sprites[] — CollectionSprite

Each entry represents one Genesis hardware sprite from the Sprite Attribute Table.

| Field | Type | Default | Description |
|-------|------|---------|-------------|
| `index` | int | 0 | Hardware sprite index (0-79) |
| `x` | int | 0 | Screen X position (offset by -128) |
| `y` | int | 0 | Screen Y position (offset by -128) |
| `width_tiles` | int | 1 | Width in 8px tiles (1-4) |
| `height_tiles` | int | 1 | Height in 8px tiles (1-4) |
| `palette_line` | int | 0 | CRAM palette line (0-3) |
| `priority` | bool | false | VDP priority bit |
| `h_flip` | bool | false | Horizontal flip |
| `v_flip` | bool | false | Vertical flip |
| `pattern` | int | 0 | Tile pattern index in VRAM |
| `vram_addr` | string | "" | VRAM address (hex string) |
| `rom_offset` | hex string or null | null | Source ROM offset (if resolved) |
| `source` | string | "dma" | How tile data was found: "dma", "search", "embedded" |
| `tile_data` | hex string | "" | Embedded tile data (when source = "embedded") |

---

## Backward Compatibility

The parser auto-detects the format:

- **Normalized format**: `"palettes"` is a JSON object (keyed by ID)
- **Legacy format**: `"sprite_groups"` array is present, or `"palettes"` is absent/array

Old `.json` files with `sprite_groups` continue to work. Old `.json` and `.sprec`
files with `sprite_animation` or `sprite_collections` wrapper are also supported
when loaded as `.sprec` recordings.

### Legacy-only fields (deprecated)

| Field | Description |
|-------|-------------|
| `sprite_groups` | Array of `{name, palettes[], sprites[]}` — replaced by `palettes` + `patterns` pools |
| `sprite_animation` | Object wrapper `{game_name, palettes, frames}` — replaced by .sprec format |

---

## Hex Offset Format

All hex offset fields (`rom_offset`, `start_offset`, `end_offset`, `vram_addr`,
`dma_source`) accept either format:

- With prefix: `"0x0082CA"` or `"0X0082CA"`
- Without prefix: `"0082CA"`

Leading zeros are optional. Parsing is case-insensitive. Invalid hex strings
fall back to a context-dependent default (usually 0).
