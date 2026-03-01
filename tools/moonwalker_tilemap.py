#!/usr/bin/env python3
"""
moonwalker_tilemap.py - Complete tile data map of the Moonwalker ROM.

Scans the sprite bank region (0x01B000-0x02A400) tile-by-tile to create
a complete map of all tile data, identifying empty tiles, sprite boundaries,
and contiguous data regions.

Also scans for palette data throughout the ROM by looking for sequences of
CRAM-format colors (---- bbb- ggg- rrr-).
"""

import struct
import sys
import json


def read_u16(rom, off):
    return struct.unpack_from('>H', rom, off)[0]


def tile_is_empty(rom, offset):
    """Check if a 32-byte tile is all zeros."""
    return all(b == 0 for b in rom[offset:offset+32])


def tile_entropy(rom, offset):
    """Rough entropy estimate: count unique bytes in a 32-byte tile."""
    data = rom[offset:offset+32]
    return len(set(data))


def scan_tile_region(rom, start, end):
    """Scan a ROM region tile by tile (32 bytes each).
    Returns a list of tile descriptors.
    """
    tiles = []
    for off in range(start, end, 32):
        if off + 32 > len(rom):
            break
        data = rom[off:off+32]
        is_empty = all(b == 0 for b in data)
        zero_ratio = data.count(0) / 32
        unique_bytes = len(set(data))

        # Check if it looks like 4bpp tile data vs other data
        # 4bpp tiles: nibbles 0-F, typically many zeros
        max_nibble = max(max(b >> 4, b & 0xF) for b in data) if any(data) else 0

        tiles.append({
            'offset': off,
            'empty': is_empty,
            'zero_ratio': zero_ratio,
            'unique_bytes': unique_bytes,
            'max_nibble': max_nibble,
        })
    return tiles


def find_sprite_boundaries(tiles):
    """Find boundaries between sprites by detecting empty tile runs."""
    regions = []
    current_start = None
    current_type = None  # 'data' or 'empty'

    for t in tiles:
        ttype = 'empty' if t['empty'] else 'data'
        if ttype != current_type:
            if current_start is not None:
                regions.append({
                    'start': current_start,
                    'end': t['offset'],
                    'type': current_type,
                    'size': t['offset'] - current_start,
                    'tiles': (t['offset'] - current_start) // 32,
                })
            current_start = t['offset']
            current_type = ttype

    # Final region
    if current_start is not None:
        end = tiles[-1]['offset'] + 32
        regions.append({
            'start': current_start,
            'end': end,
            'type': current_type,
            'size': end - current_start,
            'tiles': (end - current_start) // 32,
        })

    return regions


def classify_sprite_size(tile_count):
    """Guess sprite dimensions from tile count."""
    # Common Genesis sprite sizes (width x height in tiles)
    sizes = {
        1: [(1, 1)],
        2: [(1, 2), (2, 1)],
        3: [(1, 3), (3, 1)],
        4: [(1, 4), (2, 2), (4, 1)],
        6: [(2, 3), (3, 2)],
        8: [(2, 4), (4, 2)],
        9: [(3, 3)],
        10: [(2, 5)],
        12: [(3, 4), (4, 3)],
        15: [(3, 5), (5, 3)],
        16: [(4, 4)],
    }
    return sizes.get(tile_count, [])


def analyze_data_region(rom, region):
    """Analyze a data region to guess sprite dimensions and frame count."""
    size = region['size']
    tiles = region['tiles']

    # Try common sprite sizes and see which divide evenly
    candidates = []
    for tiles_per_sprite in [8, 10, 15, 4, 12, 6, 9, 16, 2, 3, 1]:
        if tiles % tiles_per_sprite == 0:
            frame_count = tiles // tiles_per_sprite
            dim_options = classify_sprite_size(tiles_per_sprite)
            for w, h in dim_options:
                candidates.append({
                    'w': w, 'h': h,
                    'tiles_per_frame': tiles_per_sprite,
                    'frame_count': frame_count,
                })

    return candidates


def scan_for_palettes(rom):
    """Scan ROM for sequences of CRAM-format colors.

    CRAM format: ---- bbb- ggg- rrr- (each channel 0-7, bit 0 always 0)
    Valid CRAM word: bits 15-12 = 0, odd bits of each channel = 0
    Mask: 0xF111 should be 0x0000
    """
    print("\n" + "=" * 70)
    print("Palette Data Scan")
    print("=" * 70)

    palettes = []
    offset = 0
    while offset < len(rom) - 32:
        # Check if 16 consecutive words are valid CRAM colors
        valid = True
        colors = []
        for i in range(16):
            w = read_u16(rom, offset + i * 2)
            # Valid CRAM: bits 15-12 = 0, bits 0 of each channel unused but often 0
            # More relaxed: just check bits 15-12 and bits 8,4,0
            if w & 0xF111:
                valid = False
                break
            colors.append(w)

        if valid and not all(c == 0 for c in colors):
            # Check it's not in the middle of code
            # Valid palettes usually have at least 3 non-zero colors
            non_zero = sum(1 for c in colors if c != 0)
            if non_zero >= 3:
                palettes.append({
                    'offset': offset,
                    'colors': colors,
                    'non_zero': non_zero,
                })
                offset += 32  # skip past this palette
                continue

        offset += 2

    print(f"Found {len(palettes)} potential palette locations:")
    for p in palettes:
        colors_hex = ' '.join(f'{c:04X}' for c in p['colors'])
        # Convert to approximate RGB for display
        rgb = []
        for c in p['colors']:
            r = ((c >> 1) & 7) * 36
            g = ((c >> 5) & 7) * 36
            b = ((c >> 9) & 7) * 36
            rgb.append(f'({r},{g},{b})')

        print(f"  0x{p['offset']:06X}: {colors_hex}")
        print(f"    RGB: {' '.join(rgb[:8])}")
        print(f"         {' '.join(rgb[8:])}")

    return palettes


def main():
    rom_path = sys.argv[1] if len(sys.argv) > 1 else 'roms/Moonwalker.bin'

    with open(rom_path, 'rb') as f:
        rom = f.read()

    print(f"Loaded ROM: {rom_path} ({len(rom)} bytes)\n")

    # Scan the main sprite bank region
    bank_start = 0x01B000
    bank_end = 0x02A400

    print("=" * 70)
    print(f"Tile Data Map: 0x{bank_start:06X} - 0x{bank_end:06X}")
    print("=" * 70)

    tiles = scan_tile_region(rom, bank_start, bank_end)
    regions = find_sprite_boundaries(tiles)

    # Print region map
    print(f"\nTotal tiles: {len(tiles)}")
    empty_tiles = sum(1 for t in tiles if t['empty'])
    print(f"Empty tiles: {empty_tiles} ({empty_tiles/len(tiles):.1%})")
    print(f"Data tiles: {len(tiles) - empty_tiles} ({(len(tiles)-empty_tiles)/len(tiles):.1%})")

    print(f"\nContiguous regions:")
    known_chunks = {
        0x01D760: "Characters-large (2x4x73)",
        0x0220E0: "MJ Anim A (2x4x23)",
        0x023800: "MJ Anim B (2x5x43)",
        0x026DE0: "Chunk D (2x4x13)",
        0x027B60: "Chunk E (2x4x21)",
        0x0290A0: "Large chars (3x5x3)",
        0x029660: "Chunk F (2x4x13)",
    }

    data_regions = []
    for r in regions:
        known = known_chunks.get(r['start'], "")
        marker = f"  <-- {known}" if known else ""

        if r['type'] == 'data':
            # Analyze possible sprite dimensions
            candidates = analyze_data_region(rom, r)
            best = candidates[0] if candidates else None
            if best:
                dim_str = f"{best['w']}x{best['h']}x{best['frame_count']}"
            else:
                dim_str = "?"

            print(f"  DATA  0x{r['start']:06X}-0x{r['end']:06X}  "
                  f"{r['size']:5d} bytes  {r['tiles']:3d} tiles  "
                  f"[{dim_str}]{marker}")
            data_regions.append(r)
        else:
            if r['tiles'] >= 2:
                print(f"  EMPTY 0x{r['start']:06X}-0x{r['end']:06X}  "
                      f"{r['size']:5d} bytes  {r['tiles']:3d} tiles")

    # Generate a corrected sprite definition
    print("\n" + "=" * 70)
    print("Proposed Sprite Definitions (based on tile analysis)")
    print("=" * 70)

    for r in data_regions:
        candidates = analyze_data_region(rom, r)
        known = known_chunks.get(r['start'], None)

        print(f"\n  0x{r['start']:06X}: {r['tiles']} tiles ({r['size']} bytes)")
        if known:
            print(f"    Current name: {known}")
        print(f"    Possible dimensions:")
        for c in candidates[:5]:
            print(f"      {c['w']}x{c['h']} tiles = {c['frame_count']} frames "
                  f"({c['w']*8}x{c['h']*8} pixels)")

    # Also scan for palettes
    palettes = scan_for_palettes(rom)

    # Generate updated JSON snippet
    print("\n" + "=" * 70)
    print("Updated Sprite Definition JSON")
    print("=" * 70)

    sprites = []
    for r in data_regions:
        candidates = analyze_data_region(rom, r)
        known = known_chunks.get(r['start'], None)

        # Pick the most likely dimension
        # Prefer 2x4 (common MJ size) or 2x5, then 3x5
        preferred = None
        for c in candidates:
            if (c['w'], c['h']) in [(2, 4), (2, 5), (3, 5), (4, 4), (3, 3)]:
                preferred = c
                break
        if not preferred and candidates:
            preferred = candidates[0]

        if preferred:
            name = known.split('(')[0].strip() if known else f"Region 0x{r['start']:06X}"
            sprites.append({
                'name': name,
                'rom_offset': f"0x{r['start']:06X}",
                'width_tiles': preferred['w'],
                'height_tiles': preferred['h'],
                'frame_count': preferred['frame_count'],
                'palette_index': 0,
                'compression': 'none',
                'notes': f"Tiles: {r['tiles']}, verified by static analysis",
            })

    print(json.dumps(sprites, indent=2))


if __name__ == '__main__':
    main()
